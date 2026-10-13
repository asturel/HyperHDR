#pragma once

#include <effects/AnimationBase.h>

#define ANIM_BOOT_COMET "Boot comet"

class Animation_BootComet : public AnimationBase
{
public:
	Animation_BootComet();

	void Init(HyperImage& hyperImage, int hyperLatchTime) override;
	bool Play(HyperImage& painter) override;
	bool hasLedData(QVector<ColorRgb>& buffer) override;

	static EffectDefinition getDefinition();

private:
	static bool isRegistered;

	int     ledCount;
	int64_t prevMs;
	float   elapsed;
	QVector<ColorRgb> ledData;

	static constexpr int   NUM_PASSES  = 3;    // orange, cyan, violet
	static constexpr float PASS_TIME   = 1.4f; // seconds per full lap
	static constexpr float SETTLE_TIME = 1.2f; // idle tail after last lap
};