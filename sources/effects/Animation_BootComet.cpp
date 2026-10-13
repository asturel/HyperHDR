#include <effects/Animation_BootComet.h>
#include <utils/InternalClock.h>
#include <cmath>

Animation_BootComet::Animation_BootComet() :
	AnimationBase(), ledCount(-1), prevMs(0), elapsed(0.0f)
{}

EffectDefinition Animation_BootComet::getDefinition()
{
	EffectDefinition ed(EffectFactory<Animation_BootComet>);
	ed.name = ANIM_BOOT_COMET;
	return ed;
}

bool Animation_BootComet::isRegistered =
	hyperhdr::effects::REGISTER_EFFECT(Animation_BootComet::getDefinition());

void Animation_BootComet::Init(HyperImage&, int)
{
	SetSleepTime(33);          // ~30 fps
	ledCount = -1;
	prevMs   = InternalClock::now();
	elapsed  = 0.0f;
}

bool Animation_BootComet::Play(HyperImage&) { return true; }

bool Animation_BootComet::hasLedData(QVector<ColorRgb>& buffer)
{
	const int n = static_cast<int>(buffer.size());
	if (n < 2)
		return true;

	// Re-init when LED count changes
	if (n != ledCount)
	{
		ledCount = n;
		ledData.resize(n);
		prevMs  = InternalClock::now();
		elapsed = 0.0f;
	}

	// Delta time with sanity clamping for first frame / lag spikes
	int64_t now = InternalClock::now();
	float dt = static_cast<float>(now - prevMs) / 1000.0f;
	if (dt <= 0.0f || dt > 0.5f) dt = 0.033f;
	prevMs = now;
	elapsed += dt;

	// One hue per pass
	static const int passHue[NUM_PASSES] = { 15, 190, 280 };
	const float totalSpin = PASS_TIME * NUM_PASSES;
	const float tailLen   = std::max(6.0f, n * 0.18f);

	if (elapsed < totalSpin)
	{
		// Which pass are we in?
		int pass = static_cast<int>(elapsed / PASS_TIME);
		if (pass >= NUM_PASSES) pass = NUM_PASSES - 1;

		// Head position along the strip for this pass
		float localT  = (elapsed - pass * PASS_TIME) / PASS_TIME;
		float headPos = localT * n;
		int   hue     = passHue[pass];

		for (int i = 0; i < n; i++)
		{
			// Distance behind the head, wrapped around the strip
			float d = std::fmod(headPos - i + n, static_cast<float>(n));
			float val = 0.0f;
			int   sat = 230;

			if (d < tailLen)
			{
				// Exponential falloff -> sharp comet tail
				float lin     = d / tailLen;
				float falloff = std::exp(-lin * 3.5f);
				val = falloff * 255.0f;

				// Whitish core right at the head
				if (lin < 0.06f)
				{
					val = 255.0f;
					sat = 80;
				}
			}

			ColorRgb c;
			c.fromHsv(hue, sat, static_cast<int>(val));
			ledData[i] = c;
		}
	}
	else
	{
		// Soft "ready" idle glow, then quit
		float st      = elapsed - totalSpin;
		float breathe = 0.5f + 0.5f * std::sin(st * 3.0f);
		int   val     = static_cast<int>(18 + 40 * breathe);

		ColorRgb c;
		c.fromHsv(210, 220, val);
		for (int i = 0; i < n; i++)
			ledData[i] = c;

		if (st > SETTLE_TIME)
			setStopMe(true);
	}

	ledData.swap(buffer);
	return true;
}