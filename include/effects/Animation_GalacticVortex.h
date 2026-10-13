#pragma once

#include <effects/AnimationBase.h>

#define ANIM_GALACTIC_VORTEX "Galactic vortex"

class Animation_GalacticVortex : public AnimationBase
{
public:
	Animation_GalacticVortex();

	void Init(HyperImage& hyperImage, int hyperLatchTime) override;
	bool Play(HyperImage& painter) override;

	static EffectDefinition getDefinition();

private:
	static bool isRegistered;

	static constexpr int GV_WIDTH   = 80;
	static constexpr int GV_HEIGHT  = 45;
	static constexpr int GV_PAL     = 360;
	static constexpr int STAR_COUNT = 60;

	uint8_t palette[GV_PAL * 3];

	double time;
	double phaseOffset;
	int    glitchTimer;

	struct Star { double x, y, speed, hue; };
	Star stars[STAR_COUNT];

	void spawnStars();
};