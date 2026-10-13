#include <effects/Animation_IdleFireflies.h>
#include <utils/InternalClock.h>
#include <cmath>
#include <algorithm>

Animation_IdleFireflies::Animation_IdleFireflies() :
	AnimationBase(), ledCount(-1), prevMs(0), elapsed(0.0f), spawnTimer(0.0f)
{}

EffectDefinition Animation_IdleFireflies::getDefinition()
{
	// Slow smoothing keeps the sparkles soft, not jarring
	EffectDefinition ed(EffectFactory<Animation_IdleFireflies>, 350, 12);
	ed.name = ANIM_IDLE_FIREFLIES;
	return ed;
}

bool Animation_IdleFireflies::isRegistered =
	hyperhdr::effects::REGISTER_EFFECT(Animation_IdleFireflies::getDefinition());

void Animation_IdleFireflies::Init(HyperImage&, int)
{
	SetSleepTime(50);          // ~20 fps
	ledCount   = -1;
	prevMs     = InternalClock::now();
	elapsed    = 0.0f;
	spawnTimer = 0.5f;
	fireflies.clear();
}

bool Animation_IdleFireflies::Play(HyperImage&) { return true; }

bool Animation_IdleFireflies::hasLedData(QVector<ColorRgb>& buffer)
{
	const int n = static_cast<int>(buffer.size());
	if (n < 2)
		return true;

	if (n != ledCount)
	{
		ledCount = n;
		ledData.resize(n);
		prevMs     = InternalClock::now();
		elapsed    = 0.0f;
		spawnTimer = 0.5f;
		fireflies.clear();
	}

	int64_t now = InternalClock::now();
	float dt = static_cast<float>(now - prevMs) / 1000.0f;
	if (dt <= 0.0f || dt > 0.5f) dt = 0.05f;
	prevMs = now;
	elapsed += dt;

	// Spawn a new firefly occasionally, if we have room
	spawnTimer -= dt;
	if (spawnTimer <= 0.0f && fireflies.size() < MAX_FIREFLIES)
	{
		Firefly f;
		f.index   = rand() % n;
		f.maxLife = 0.9f + (rand() % 160) / 100.0f;   // 0.9 - 2.5 s
		f.life    = f.maxLife;
		f.hue     = 30 + (rand() % 40);               // warm amber / orange
		fireflies.push_back(f);

		float r = (rand() % 1000) / 1000.0f;
		spawnTimer = MIN_SPAWN + r * (MAX_SPAWN - MIN_SPAWN);
	}

	// Start from black every frame
	for (int i = 0; i < n; i++)
		ledData[i] = ColorRgb(0, 0, 0);

	// Update and render each firefly
	for (int k = static_cast<int>(fireflies.size()) - 1; k >= 0; k--)
	{
		Firefly& f = fireflies[k];
		f.life -= dt;

		if (f.life <= 0.0f)
		{
			fireflies.removeAt(k);
			continue;
		}

		// Triangular fade: 0 -> peak -> 0 across its life
		float t     = 1.0f - f.life / f.maxLife;      // 0..1
		float env   = (t < 0.5f) ? (t * 2.0f) : (1.0f - t) * 2.0f;
		float v     = env * 160.0f;                   // peak brightness
		int   sat   = static_cast<int>(200.0f * env + 55.0f);

		ColorRgb c;
		c.fromHsv(f.hue, sat, static_cast<int>(v));

		// Soft bleed onto immediate neighbours (1 LED either side)
		const int idx = f.index;
		auto addSoft = [&](int i, float scale)
		{
			if (i < 0 || i >= n) return;
			uint8_t r = static_cast<uint8_t>(c.red   * scale);
			uint8_t g = static_cast<uint8_t>(c.green * scale);
			uint8_t b = static_cast<uint8_t>(c.blue  * scale);

			ledData[i] = ColorRgb(
				std::max<uint8_t>(ledData[i].red,   r),
				std::max<uint8_t>(ledData[i].green, g),
				std::max<uint8_t>(ledData[i].blue,  b));
		};

		addSoft(idx - 1, 0.30f);
		addSoft(idx,     1.00f);
		addSoft(idx + 1, 0.30f);
	}

	ledData.swap(buffer);
	return true;
}