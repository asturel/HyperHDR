#pragma once

#include <effects/AnimationBase.h>

#define ANIM_BOOT_SEQUENCE "Boot sequence"

class Animation_BootSequence : public AnimationBase
{
public:
	Animation_BootSequence();

	void Init(HyperImage& hyperImage, int hyperLatchTime) override;
	bool Play(HyperImage& painter) override;
	bool hasLedData(QVector<ColorRgb>& buffer) override;

	static EffectDefinition getDefinition();

private:
	static bool isRegistered;

	int       stage;        // 0..3 (power-on, loading, complete, ready)
	float     stageTimer;   // seconds elapsed in current stage
	int       ledCount;
	int64_t   prevMs;
	QVector<ColorRgb> ledData;
};