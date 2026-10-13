#pragma once

#include <effects/AnimationBase.h>

#define ANIM_BOOT_CHASE "Boot chase"

class Animation_BootChase : public AnimationBase
{
public:
	Animation_BootChase();

	void Init(HyperImage& hyperImage, int hyperLatchTime) override;
	bool Play(HyperImage& painter) override;
	bool hasLedData(QVector<ColorRgb>& buffer) override;

	static EffectDefinition getDefinition();

private:
	static bool isRegistered;

	int     ledCount;
	int64_t prevMs;
	float   elapsed;
	float   headPos;
	QVector<ColorRgb> ledData;

	static constexpr int   NUM_HEADS  = 4;
	static constexpr float SPIN_TIME  = 3.0f;  // accelerating spin duration
	static constexpr float FLASH_TIME = 0.20f; // white flash duration
};