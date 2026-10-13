#pragma once

#include <effects/AnimationBase.h>

#define ANIM_IDLE_DORMANT "Idle dormant"

class Animation_IdleDormant : public AnimationBase
{
public:
	Animation_IdleDormant();

	void Init(HyperImage& hyperImage, int hyperLatchTime) override;
	bool Play(HyperImage& painter) override;
	bool hasLedData(QVector<ColorRgb>& buffer) override;

	static EffectDefinition getDefinition();

private:
	static bool isRegistered;

	int     ledCount;
	int64_t prevMs;
	float   elapsed;
	float   starTimer;    // seconds until next shooting star
	float   starActive;   // remaining life of the current star
	float   starHead;     // position of the star head along the strip
	int     starHue;
	QVector<ColorRgb> ledData;
};