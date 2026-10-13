#include <effects/Animation_BootMatrix.h>
#include <utils/InternalClock.h>
#include <cmath>
#include <algorithm>

Animation_BootMatrix::Animation_BootMatrix() :
	AnimationBase(), ledCount(-1), prevMs(0), elapsed(0.0f), nextHitDelay(0.0f)
{}

EffectDefinition Animation_BootMatrix::getDefinition()
{
	EffectDefinition ed(EffectFactory<Animation_BootMatrix>);
	ed.name = ANIM_BOOT_MATRIX;
	return ed;
}

bool Animation_BootMatrix::isRegistered =
	hyperhdr::effects::REGISTER_EFFECT(Animation_BootMatrix::getDefinition());

void Animation_BootMatrix::Init(HyperImage&, int)
{
	SetSleepTime(30);
	ledCount     = -1;
	prevMs       = InternalClock::now();
	elapsed      = 0.0f;
	nextHitDelay = 0.0f;
}

bool Animation_BootMatrix::Play(HyperImage&) { return true; }

bool Animation_BootMatrix::hasLedData(QVector<ColorRgb>& buffer)
{
	const int n = static_cast<int>(buffer.size());
	if (n < 2)
		return true;

	if (n != ledCount)
	{
		ledCount = n;
		ledData.resize(n);
		cellValue.resize(n);
		cellTarget.resize(n);

		for (int i = 0; i < n; i++)
		{
			cellValue[i]  = 0;
			cellTarget[i] = 0;
		}

		prevMs       = InternalClock::now();
		elapsed      = 0.0f;
		nextHitDelay = 0.0f;
	}

	int64_t now = InternalClock::now();
	float dt = static_cast<float>(now - prevMs) / 1000.0f;
	if (dt <= 0.0f || dt > 0.5f) dt = 0.03f;
	prevMs = now;
	elapsed += dt;

	const float checkEnd = CHECK_TIME;
	const float fillEnd  = checkEnd + FILL_TIME;
	const float flashEnd = fillEnd + FLASH_TIME;
	const float fadeEnd  = flashEnd + FADE_TIME;

	if (elapsed < checkEnd)
	{
		// Randomly toggle individual LEDs on and off
		nextHitDelay -= dt;
		if (nextHitDelay <= 0.0f)
		{
			int   toggles = 3 + rand() % 6;
			for (int k = 0; k < toggles; k++)
			{
				int idx = rand() % n;
				cellTarget[idx] = (cellTarget[idx] == 0) ? 255 : 0;
			}
			nextHitDelay = 0.03f + (rand() % 30) / 1000.0f;
		}
	}
	else if (elapsed < fillEnd)
	{
		// Light everything up
		for (int i = 0; i < n; i++)
			cellTarget[i] = 255;
	}
	else if (elapsed < flashEnd)
	{
		// Full white flash (handled by cellValue below)
		for (int i = 0; i < n; i++)
			cellTarget[i] = 255;
	}
	else if (elapsed < fadeEnd)
	{
		for (int i = 0; i < n; i++)
			cellTarget[i] = 0;
	}
	else
	{
		setStopMe(true);
	}

	// Smoothly ease cellValue toward cellTarget (simple exponential smoothing)
	const float ease = std::min(1.0f, dt * 12.0f);

	for (int i = 0; i < n; i++)
	{
		float v = cellValue[i];
		v += (cellTarget[i] - v) * ease;
		v = std::max(0.0f, std::min(255.0f, v));
		cellValue[i] = static_cast<uint8_t>(v);

		// Choose colour depending on phase
		ColorRgb c;
		if (elapsed < checkEnd)
		{
			// Cool blue-white "test" colour
			c = ColorRgb(
				static_cast<uint8_t>(cellValue[i] * 0.4f),
				static_cast<uint8_t>(cellValue[i] * 0.85f),
				cellValue[i]);
		}
		else if (elapsed < fillEnd)
		{
			// Warm amber fill
			c = ColorRgb(
				cellValue[i],
				static_cast<uint8_t>(cellValue[i] * 0.7f),
				static_cast<uint8_t>(cellValue[i] * 0.15f));
		}
		else if (elapsed < flashEnd)
		{
			// Full white
			c = ColorRgb(255, 255, 255);
		}
		else
		{
			// Fading out toward black (slight blue tint)
			c = ColorRgb(
				static_cast<uint8_t>(cellValue[i] * 0.3f),
				static_cast<uint8_t>(cellValue[i] * 0.5f),
				cellValue[i]);
		}

		ledData[i] = c;
	}

	ledData.swap(buffer);
	return true;
}