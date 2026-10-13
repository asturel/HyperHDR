#include <effects/Animation_BootRadar.h>
#include <utils/InternalClock.h>
#include <cmath>
#include <algorithm>

Animation_BootRadar::Animation_BootRadar() :
	AnimationBase(), ledCount(-1), prevMs(0), elapsed(0.0f)
{}

EffectDefinition Animation_BootRadar::getDefinition()
{
	EffectDefinition ed(EffectFactory<Animation_BootRadar>);
	ed.name = ANIM_BOOT_RADAR;
	return ed;
}

bool Animation_BootRadar::isRegistered =
	hyperhdr::effects::REGISTER_EFFECT(Animation_BootRadar::getDefinition());

void Animation_BootRadar::Init(HyperImage&, int)
{
	SetSleepTime(30);
	ledCount = -1;
	prevMs   = InternalClock::now();
	elapsed  = 0.0f;
}

bool Animation_BootRadar::Play(HyperImage&) { return true; }

bool Animation_BootRadar::hasLedData(QVector<ColorRgb>& buffer)
{
	const int n = static_cast<int>(buffer.size());
	if (n < 2)
		return true;

	if (n != ledCount)
	{
		ledCount = n;
		ledData.resize(n);
		prevMs  = InternalClock::now();
		elapsed = 0.0f;
	}

	int64_t now = InternalClock::now();
	float dt = static_cast<float>(now - prevMs) / 1000.0f;
	if (dt <= 0.0f || dt > 0.5f) dt = 0.03f;
	prevMs = now;
	elapsed += dt;

	const float forwardEnd = SWEEP_TIME;
	const float returnEnd  = SWEEP_TIME + RETURN_TIME;
	const float standbyEnd = returnEnd + STANDBY_TIME;

	// Long, softly fading tail behind the sweep head
	const float tailLen = std::max(8.0f, n * 0.45f);

	if (elapsed < forwardEnd)
	{
		// Forward sweep: 0 -> n-1
		float progress = elapsed / SWEEP_TIME;
		float head     = progress * (n - 1);

		for (int i = 0; i < n; i++)
		{
			float d   = head - i;              // pixels behind head
			float val = 0.0f;

			if (d >= 0.0f && d < tailLen)
			{
				float lin     = d / tailLen;
				float falloff = std::exp(-lin * 2.5f);
				val = falloff * 240.0f;
				if (d < 1.0f) val = 255.0f;    // crisp leading edge
			}

			// Slight hue shift as we look "outward" in the strip
			int hue = 150 + static_cast<int>(40.0f * (i / static_cast<float>(n)));

			ColorRgb c;
			c.fromHsv(hue, (d < 1.0f) ? 60 : 220, static_cast<int>(val));
			ledData[i] = c;
		}
	}
	else if (elapsed < returnEnd)
	{
		// Return sweep: n-1 -> 0, slightly different colour
		float progress = (elapsed - forwardEnd) / RETURN_TIME;
		float head     = (1.0f - progress) * (n - 1);

		for (int i = 0; i < n; i++)
		{
			float d   = i - head;              // pixels behind head (reverse dir)
			float val = 0.0f;

			if (d >= 0.0f && d < tailLen)
			{
				float lin     = d / tailLen;
				float falloff = std::exp(-lin * 3.0f);
				val = falloff * 200.0f;
				if (d < 1.0f) val = 255.0f;
			}

			ColorRgb c;
			c.fromHsv(200, (d < 1.0f) ? 60 : 220, static_cast<int>(val));
			ledData[i] = c;
		}
	}
	else if (elapsed < standbyEnd)
	{
		// Fade-in idle standby glow
		float t = (elapsed - returnEnd) / STANDBY_TIME;
		int   v = static_cast<int>(90.0f * (1.0f - t) + 12.0f * t);

		ColorRgb c;
		c.fromHsv(210, 220, v);
		for (int i = 0; i < n; i++)
			ledData[i] = c;
	}
	else
	{
		setStopMe(true);
	}

	ledData.swap(buffer);
	return true;
}