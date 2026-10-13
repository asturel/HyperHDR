#include <effects/Animation_IdleDormant.h>
#include <utils/InternalClock.h>
#include <cmath>
#include <algorithm>

Animation_IdleDormant::Animation_IdleDormant() :
	AnimationBase(),
	ledCount(-1), prevMs(0), elapsed(0.0f),
	starTimer(5.0f), starActive(0.0f), starHead(0.0f), starHue(200)
{}

EffectDefinition Animation_IdleDormant::getDefinition()
{
	// Low refresh rate and strong smoothing so nothing ever jumps
	EffectDefinition ed(EffectFactory<Animation_IdleDormant>, 600, 10);
	ed.name = ANIM_IDLE_DORMANT;
	return ed;
}

bool Animation_IdleDormant::isRegistered =
	hyperhdr::effects::REGISTER_EFFECT(Animation_IdleDormant::getDefinition());

void Animation_IdleDormant::Init(HyperImage&, int)
{
	SetSleepTime(60);          // about 17 fps is enough
	ledCount   = -1;
	prevMs     = InternalClock::now();
	elapsed    = 0.0f;
	starTimer  = 4.0f + (rand() % 1500) / 100.0f;
	starActive = 0.0f;
	starHead   = 0.0f;
	starHue    = 180 + (rand() % 80);
}

bool Animation_IdleDormant::Play(HyperImage&) { return true; }

bool Animation_IdleDormant::hasLedData(QVector<ColorRgb>& buffer)
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
	if (dt <= 0.0f || dt > 0.5f) dt = 0.06f;
	prevMs = now;
	elapsed += dt;

	// Two very slowly drifting aurora blobs
	float b1 = std::fmod(elapsed * 0.045f, 1.0f) * n;              // ~22 s per lap
	float b2 = std::fmod(elapsed * 0.030f + 0.5f, 1.0f) * n;        // ~33 s per lap
	float blobWidth = std::max(3.0f, n * 0.10f);
	int   baseHue   = static_cast<int>(std::fmod(elapsed * 4.0f, 360.0f));

	for (int i = 0; i < n; i++)
	{
		// Shortest distance on the circular strip
		float d1 = std::fabs(i - b1); d1 = std::min(d1, n - d1);
		float d2 = std::fabs(i - b2); d2 = std::min(d2, n - d2);

		float i1 = std::exp(-(d1 * d1) / (blobWidth * blobWidth));
		float i2 = std::exp(-(d2 * d2) / (blobWidth * blobWidth * 1.6f));

		float intensity = i1 * 0.75f + i2 * 0.45f;
		float val       = intensity * 55.0f + 3.0f;   // 3 floor + up to ~58
		int   hue       = (baseHue + static_cast<int>(i * 1.2f)) % 360;

		ColorRgb c;
		c.fromHsv(hue, 220, static_cast<int>(val));
		ledData[i] = c;
	}

	// Rare, faint shooting star
	starTimer -= dt;
	if (starActive <= 0.0f && starTimer <= 0.0f)
	{
		starActive = 1.2f;
		starHead   = 0.0f;
		starHue    = 180 + (rand() % 100);
		starTimer  = 15.0f + (rand() % 2000) / 100.0f;   // 15-35 s
	}

	if (starActive > 0.0f)
	{
		starActive -= dt;
		starHead   += dt * (n / 1.2f);

		int hi   = static_cast<int>(starHead) % n;
		int tail = std::max(2, n / 25);

		for (int k = 0; k < tail; k++)
		{
			int idx = hi - k;
			if (idx < 0) idx += n;

			float f = 1.0f - k / static_cast<float>(tail);
			float v = f * f * 140.0f;

			ColorRgb c;
			c.fromHsv(starHue, (k == 0) ? 40 : 200, static_cast<int>(v));

			// Composite with max() so we don't overwrite the aurora
			uint8_t r = std::max<uint8_t>(ledData[idx].red,   c.red);
			uint8_t g = std::max<uint8_t>(ledData[idx].green, c.green);
			uint8_t b = std::max<uint8_t>(ledData[idx].blue,  c.blue);
			ledData[idx] = ColorRgb(r, g, b);
		}
	}

	ledData.swap(buffer);
	return true;
}