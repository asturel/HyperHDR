#pragma once

#include <effects/AnimationBase.h>

#define ANIM_IDLE_EMBER "Idle ember"

class Animation_IdleEmber : public AnimationBase
{
public:
	Animation_IdleEmber();

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
};