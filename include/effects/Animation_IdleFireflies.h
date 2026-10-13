#pragma once

#include <effects/AnimationBase.h>
#include <QVector>

#define ANIM_IDLE_FIREFLIES "Idle fireflies"

class Animation_IdleFireflies : public AnimationBase
{
public:
	Animation_IdleFireflies();

	void Init(HyperImage& hyperImage, int hyperLatchTime) override;
	bool Play(HyperImage& painter) override;
	bool hasLedData(QVector<ColorRgb>& buffer) override;

	static EffectDefinition getDefinition();

private:
	static bool isRegistered;

	struct Firefly
	{
		int   index;        // which LED
		float life;         // remaining life in seconds
		float maxLife;
		int   hue;
	};

	int     ledCount;
	int64_t prevMs;
	float   elapsed;
	float   spawnTimer;
	QVector<Firefly> fireflies;
	QVector<ColorRgb> ledData;

	static constexpr int   MAX_FIREFLIES = 3;
	static constexpr float MIN_SPAWN     = 0.8f;
	static constexpr float MAX_SPAWN     = 2.5f;
};