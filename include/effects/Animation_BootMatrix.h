#pragma once

#include <effects/AnimationBase.h>
#include <QVector>

#define ANIM_BOOT_MATRIX "Boot matrix"

class Animation_BootMatrix : public AnimationBase
{
public:
	Animation_BootMatrix();

	void Init(HyperImage& hyperImage, int hyperLatchTime) override;
	bool Play(HyperImage& painter) override;
	bool hasLedData(QVector<ColorRgb>& buffer) override;

	static EffectDefinition getDefinition();

private:
	static bool isRegistered;

	int     ledCount;
	int64_t prevMs;
	float   elapsed;
	QVector<uint8_t> cellValue;   // per-LED brightness, eased toward target
	QVector<uint8_t> cellTarget;  // 0 or 255
	float   nextHitDelay;
	QVector<ColorRgb> ledData;

	static constexpr float CHECK_TIME  = 2.2f;  // random test phase
	static constexpr float FILL_TIME   = 0.5f;  // everything turns on
	static constexpr float FLASH_TIME  = 0.25f; // brief full-white flash
	static constexpr float FADE_TIME   = 0.8f;  // fade to black
};