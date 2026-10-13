#pragma once

#include <effects/AnimationBase.h>

#define ANIM_BOOT_RADAR "Boot radar"

class Animation_BootRadar : public AnimationBase
{
public:
	Animation_BootRadar();

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

	static constexpr float SWEEP_TIME   = 1.6f;  // forward sweep duration
	static constexpr float RETURN_TIME  = 1.0f;  // faster return sweep
	static constexpr float STANDBY_TIME = 1.0f;
};