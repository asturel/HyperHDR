#include <effects/Animation_BootChase.h>
#include <utils/InternalClock.h>
#include <cmath>
#include <algorithm>

Animation_BootChase::Animation_BootChase() :
	AnimationBase(), ledCount(-1), prevMs(0), elapsed(0.0f), headPos(0.0f)
{}

EffectDefinition Animation_BootChase::getDefinition()
{
	EffectDefinition ed(EffectFactory<Animation_BootChase>);
	ed.name = ANIM_BOOT_CHASE;
	return ed;
}

bool Animation_BootChase::isRegistered =
	hyperhdr::effects::REGISTER_EFFECT(Animation_BootChase::getDefinition());

void Animation_BootChase::Init(HyperImage&, int)
{
	SetSleepTime(28);          // ~35 fps
	ledCount = -1;
	prevMs   = InternalClock::now();
	elapsed  = 0.0f;
	headPos  = 0.0f;
}

bool Animation_BootChase::Play(HyperImage&) { return true; }

bool Animation_BootChase::hasLedData(QVector<ColorRgb>& buffer)
{
	const int n = static_cast<int>(buffer.size());
	if (n < 4)
		return true;

	if (n != ledCount)
	{
		ledCount = n;
		ledData.resize(n);
		prevMs  = InternalClock::now();
		elapsed = 0.0f;
		headPos = 0.0f;
	}

	int64_t now = InternalClock::now();
	float dt = static_cast<float>(now - prevMs) / 1000.0f;
	if (dt <= 0.0f || dt > 0.5f) dt = 0.028f;
	prevMs = now;
	elapsed += dt;

	const float tailLen = std::max(4.0f, n * 0.12f);

	if (elapsed < SPIN_TIME)
	{
		// Accelerating spin phase
		float t     = elapsed / SPIN_TIME;            // 0..1
		float speed = 18.0f + 80.0f * t;              // LEDs per second
		headPos += dt * speed;

		float baseHead = std::fmod(headPos, static_cast<float>(n));
		float offset   = n / static_cast<float>(NUM_HEADS);

		// Clear previous frame
		for (int i = 0; i < n; i++)
			ledData[i] = ColorRgb(0, 0, 0);

		// Composite NUM_HEADS comet tails
		for (int h = 0; h < NUM_HEADS; h++)
		{
			float hp  = std::fmod(baseHead + h * offset, static_cast<float>(n));
			int   hue = 200 + h * 40;                 // cyan -> magenta

			for (int i = 0; i < n; i++)
			{
				float d = std::fmod(hp - i + n, static_cast<float>(n));
				if (d >= tailLen)
					continue;

				float falloff = std::exp(-d / tailLen * 4.0f);
				float v       = falloff * 255.0f;
				int   sat     = (d < 0.5f) ? 60 : 230;

				ColorRgb c;
				c.fromHsv(hue, sat, static_cast<int>(v));

				// Additive-ish combine with existing pixel
				ledData[i] = ColorRgb(
					std::max<uint8_t>(ledData[i].red,   c.red),
					std::max<uint8_t>(ledData[i].green, c.green),
					std::max<uint8_t>(ledData[i].blue,  c.blue));
			}
		}
	}
	else if (elapsed < SPIN_TIME + FLASH_TIME)
	{
		// Victory flash
		for (int i = 0; i < n; i++)
			ledData[i] = ColorRgb(255, 255, 255);
	}
	else if (elapsed < SPIN_TIME + FLASH_TIME + 0.9f)
	{
		// Dim blue "ready" glow
		for (int i = 0; i < n; i++)
			ledData[i] = ColorRgb(0, 25, 70);
	}
	else
	{
		setStopMe(true);
	}

	ledData.swap(buffer);
	return true;
}