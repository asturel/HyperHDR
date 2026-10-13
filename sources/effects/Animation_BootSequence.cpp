#include <effects/Animation_BootSequence.h>
#include <utils/InternalClock.h>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Animation_BootSequence::Animation_BootSequence() :
	AnimationBase(),
	stage(0),
	stageTimer(0.0f),
	ledCount(-1),
	prevMs(0)
{
}

EffectDefinition Animation_BootSequence::getDefinition()
{
	EffectDefinition ed(EffectFactory<Animation_BootSequence>);
	ed.name = ANIM_BOOT_SEQUENCE;
	return ed;
}

bool Animation_BootSequence::isRegistered =
	hyperhdr::effects::REGISTER_EFFECT(Animation_BootSequence::getDefinition());

void Animation_BootSequence::Init(HyperImage& /*hyperImage*/, int /*hyperLatchTime*/)
{
	SetSleepTime(33);          // about 30 fps
	stage = 0;
	stageTimer = 0.0f;
	ledCount = -1;
	prevMs = InternalClock::now();
}

bool Animation_BootSequence::Play(HyperImage& /*painter*/)
{
	return true;
}

// Helper: HSV -> ColorRgb with input clamping
static inline ColorRgb makeHsv(float h, float s, float v)
{
	int hi = static_cast<int>(h) % 360;
	if (hi < 0) hi += 360;

	const uint8_t sat = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, s)));
	const uint8_t val = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, v)));

	uint8_t r = 0, g = 0, b = 0;
	ColorRgb::hsv2rgb(hi, sat, val, r, g, b);
	return ColorRgb(r, g, b);
}

bool Animation_BootSequence::hasLedData(QVector<ColorRgb>& buffer)
{
	const int n = static_cast<int>(buffer.size());
	if (n < 2)
		return true;

	// Re-initialise when the LED count changes
	if (n != ledCount)
	{
		ledCount = n;
		ledData.resize(n);
		stage = 0;
		stageTimer = 0.0f;
		prevMs = InternalClock::now();
	}

	// ---- timing ----
	int64_t now = InternalClock::now();
	float dt = static_cast<float>(now - prevMs) / 1000.0f;
	if (dt <= 0.0f || dt > 0.5f) dt = 0.033f;   // first frame / lag guard
	prevMs = now;
	stageTimer += dt;

	// ---- stage transitions ----
	if (stage == 0 && stageTimer >= 0.9f) { stage = 1; stageTimer = 0.0f; }
	if (stage == 1 && stageTimer >= 2.5f) { stage = 2; stageTimer = 0.0f; }
	if (stage == 2 && stageTimer >= 0.6f) { stage = 3; stageTimer = 0.0f; }
	if (stage == 3 && stageTimer >= 2.5f) { setStopMe(true); }

	const float cx   = (n - 1) * 0.5f;
	const float maxR = std::max(1.0f, cx);

	// ---- per-LED rendering ----
	for (int i = 0; i < n; i++)
	{
		// 0 = centre, 1 = edges
		const float dist = std::fabs(static_cast<float>(i) - cx) / maxR;

		ColorRgb c(0, 0, 0);

		if (stage == 0)
		{
			// Power-on: wave expanding outward from the centre
			const float t      = stageTimer / 0.9f;    // 0..1
			const float front  = t * 1.15f;            // slightly overshoot edges
			const float behind = front - dist;

			if (behind >= 0.0f)
			{
				const float ring  = std::exp(-behind * 9.0f);        // sharp edge
				const float trail = 0.35f * std::exp(-behind * 2.0f); // soft tail
				const float val   = std::min(1.0f, ring + trail) * 255.0f;

				// Whitish-cyan at the front, deep cyan behind
				const float hue = 185.0f + 35.0f * (1.0f - ring);
				const float sat = 220.0f - 180.0f * ring;

				c = makeHsv(hue, sat, val);
			}
		}
		else if (stage == 1)
		{
			// Loading: fills outward from the centre
			const float progress = stageTimer / 2.5f;   // 0..1
			if (dist <= progress)
			{
				const float edge = progress - dist;     // 0 at the edge, grows inward
				const float head = std::exp(-edge * 30.0f);
				const float base = 0.35f
				                 + 0.15f * std::sin(edge * 40.0f - stageTimer * 6.0f);
				const float val  = std::min(1.0f, base + head) * 255.0f;
				const float hue  = 195.0f - 25.0f * head; // whiter at the front
				const float sat  = 255.0f - 150.0f * head;

				c = makeHsv(hue, sat, val);
			}
		}
		else if (stage == 2)
		{
			// Complete flash: three quick pulses
			const float t     = stageTimer / 0.6f;      // 0..1
			const float pulse = std::max(0.0f, std::cos(t * 4.0f * static_cast<float>(M_PI)));
			const float val   = pulse * 255.0f;
			const float sat   = 40.0f * (1.0f - pulse); // low saturation near white

			c = makeHsv(120.0f, sat, val);
		}
		else
		{
			// Ready: green-cyan breathing
			const float breathe = 0.5f + 0.5f * std::sin(stageTimer * 2.5f);
			const float val     = 60.0f + 120.0f * breathe;
			const float hue     = 140.0f - 20.0f * breathe;

			c = makeHsv(hue, 255.0f, val);
		}

		ledData[i] = c;
	}

	ledData.swap(buffer);
	return true;
}