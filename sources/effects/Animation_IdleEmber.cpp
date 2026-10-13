#include <effects/Animation_IdleEmber.h>
#include <utils/InternalClock.h>
#include <cmath>
#include <algorithm>

Animation_IdleEmber::Animation_IdleEmber() :
	AnimationBase(), ledCount(-1), prevMs(0), elapsed(0.0f)
{}

EffectDefinition Animation_IdleEmber::getDefinition()
{
	// Slow, heavy smoothing to eliminate any visible stepping
	EffectDefinition ed(EffectFactory<Animation_IdleEmber>, 900, 8);
	ed.name = ANIM_IDLE_EMBER;
	return ed;
}

bool Animation_IdleEmber::isRegistered =
	hyperhdr::effects::REGISTER_EFFECT(Animation_IdleEmber::getDefinition());

void Animation_IdleEmber::Init(HyperImage&, int)
{
	SetSleepTime(80);          // ~12 fps is plenty for a slow glow
	ledCount = -1;
	prevMs   = InternalClock::now();
	elapsed  = 0.0f;
}

bool Animation_IdleEmber::Play(HyperImage&) { return true; }

bool Animation_IdleEmber::hasLedData(QVector<ColorRgb>& buffer)
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
	if (dt <= 0.0f || dt > 0.5f) dt = 0.08f;
	prevMs = now;
	elapsed += dt;

	// Two overlapping slow breath layers -> organic, non-repeating feel
	const float breathe1 = 0.5f + 0.5f * std::sin(elapsed * 0.30f);
	const float breathe2 = 0.5f + 0.5f * std::sin(elapsed * 0.13f + 1.7f);
	const float breathe  = 0.55f * breathe1 + 0.45f * breathe2;

	// Peak output intentionally capped low to protect LEDs and eyes
	const int peakValue = 38;
	int       baseValue = static_cast<int>(4.0f + peakValue * breathe);

	// Gentle hue drift between deep amber and warm orange
	int hue = 20 + static_cast<int>(6.0f * breathe1);

	const float cx = (n - 1) * 0.5f;

	for (int i = 0; i < n; i++)
	{
		// Distance from centre, normalised 0..1
		const float dist = std::fabs(i - cx) / std::max(1.0f, cx);
		const float window = std::exp(-dist * dist * 2.2f);

		const int v = static_cast<int>(baseValue * window);

		ColorRgb c;
		c.fromHsv(hue, 235, std::min(255, std::max(0, v)));
		ledData[i] = c;
	}

	ledData.swap(buffer);
	return true;
}