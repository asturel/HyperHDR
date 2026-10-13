#include <effects/Animation_GalacticVortex.h>
#include <algorithm>
#include <cmath>

Animation_GalacticVortex::Animation_GalacticVortex() : AnimationBase()
{
	time = 0;
	phaseOffset = 0;
	glitchTimer = 0;

	// Precompute a 360-entry RGB palette from HSV hues
	for (int i = 0; i < GV_PAL; i++)
	{
		ColorRgb c;
		c.fromHsv(i, 255, 255);
		palette[i * 3]     = c.red;
		palette[i * 3 + 1] = c.green;
		palette[i * 3 + 2] = c.blue;
	}

	spawnStars();
}

EffectDefinition Animation_GalacticVortex::getDefinition()
{
	EffectDefinition ed(EffectFactory<Animation_GalacticVortex>, 100, 30);
	ed.name = ANIM_GALACTIC_VORTEX;
	return ed;
}

bool Animation_GalacticVortex::isRegistered =
	hyperhdr::effects::REGISTER_EFFECT(Animation_GalacticVortex::getDefinition());

void Animation_GalacticVortex::spawnStars()
{
	for (int i = 0; i < STAR_COUNT; i++)
	{
		stars[i].x     = (rand() % 1000) / 1000.0;
		stars[i].y     = (rand() % 1000) / 1000.0;
		stars[i].speed = 0.1 + (rand() % 100) / 200.0;
		stars[i].hue   = rand() % 360;
	}
}

void Animation_GalacticVortex::Init(HyperImage& hyperImage, int /*hyperLatchTime*/)
{
	hyperImage.resize(GV_WIDTH, GV_HEIGHT);
	SetSleepTime(50);
	time = 0;
	phaseOffset = 0;
	glitchTimer = 0;
	spawnStars();
}

bool Animation_GalacticVortex::Play(HyperImage& painter)
{
	time += 0.03;

	// --- hyperspace glitch handling ---
	if (glitchTimer > 0)
	{
		glitchTimer--;
		phaseOffset = ((rand() % 200) - 100) / 200.0;
	}
	else if ((rand() % 250) == 0)
	{
		glitchTimer = 6;
	}
	else
	{
		phaseOffset *= 0.85;
	}

	const double cx   = GV_WIDTH  * 0.5;
	const double cy   = GV_HEIGHT * 0.5;
	const double maxR = std::sqrt(cx * cx + cy * cy);

	// --- 1) vortex layer ---
	for (int y = 0; y < GV_HEIGHT; y++)
	{
		for (int x = 0; x < GV_WIDTH; x++)
		{
			const double dx = x - cx;
			const double dy = y - cy;
			const double r  = std::sqrt(dx * dx + dy * dy);
			const double theta = std::atan2(dy, dx);

			// Logarithmic spiral coordinate
			const double s = theta * 3.0
			               + std::log(r + 1.0) * 4.0
			               - time * 2.5
			               + phaseOffset * 6.0;

			// Spiral arm sharpness
			double band = 0.5 + 0.5 * std::sin(s);
			band = band * band * band;

			// Pulsing rings emanating from the centre
			const double pulse = 0.65 + 0.35 * std::sin(r * 0.55 - time * 4.0);

			// Radial falloff toward the edges
			double falloff = std::max(0.0, 1.0 - r / maxR);
			falloff = 0.25 + 0.75 * falloff;

			const double intensity = band * pulse * falloff;

			// Rainbow hue that rotates over time and along the spiral
			int hue = static_cast<int>(std::fmod(s * 35.0 + time * 50.0, 360.0));
			if (hue < 0) hue += 360;

			const uint8_t val = static_cast<uint8_t>(
				std::min(255.0, intensity * 255.0));

			ColorRgb c;
			c.fromHsv(hue, 255, val);
			painter.setPen(c);
			painter.drawPoint(x, y);
		}
	}

	// --- 2) draw stars ---
	for (int i = 0; i < STAR_COUNT; i++)
	{
		const int sx = static_cast<int>(stars[i].x * GV_WIDTH);
		const int sy = static_cast<int>(stars[i].y * GV_HEIGHT);

		if (sx >= 0 && sx < GV_WIDTH && sy >= 0 && sy < GV_HEIGHT)
		{
			ColorRgb c;
			c.fromHsv(static_cast<int>(stars[i].hue), 255, 255);
			painter.setPen(c);
			painter.drawPoint(sx, sy);
		}
	}

	// --- 3) drift stars outward (warp effect) ---
	for (int i = 0; i < STAR_COUNT; i++)
	{
		const double dx = stars[i].x - 0.5;
		const double dy = stars[i].y - 0.5;
		stars[i].x += dx * stars[i].speed * 0.02;
		stars[i].y += dy * stars[i].speed * 0.02;

		// Respawn near the centre when leaving the frame
		if (stars[i].x < -0.1 || stars[i].x > 1.1 ||
			stars[i].y < -0.1 || stars[i].y > 1.1)
		{
			stars[i].x = 0.5 + ((rand() % 100) - 50) / 200.0;
			stars[i].y = 0.5 + ((rand() % 100) - 50) / 200.0;
			stars[i].hue = rand() % 360;
		}
	}

	return true;
}