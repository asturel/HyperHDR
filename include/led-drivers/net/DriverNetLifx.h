#pragma once

#include "ProviderUdp.h"
#include <array>
#include <vector>
#include <utility>
#include <tuple>
#include <QHostAddress>
#include <QtGlobal>
#include <QtCore/qendian.h>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

template<typename T>
static inline T qToLittleEndian(T v)
{
    return qToLittleEndian(static_cast<typename QtPrivate::qSwapType<T>::Type>(v));
}

template<typename T>
static inline T qFromLittleEndian(T v)
{
    return qFromLittleEndian(static_cast<typename QtPrivate::qSwapType<T>::Type>(v));
}

#endif

class DriverNetLifx : public ProviderUdp
{
public:
	explicit DriverNetLifx(const QJsonObject& deviceConfig);
	static LedDevice* construct(const QJsonObject& deviceConfig);
	QJsonObject discover(const QJsonObject& params) override;
	void identify(const QJsonObject& params) override;

	using MacAddress = std::array<uint8_t, 6>;
	using IpMacAddress = std::tuple<QString, QHostAddress, MacAddress>;

protected:
	bool init(QJsonObject deviceConfig) override;
	std::pair<bool, int> writeInfiniteColors(SharedOutputColors nonlinearRgbColors) override;
	bool powerOn() override;
	bool powerOff() override;
	void setPower(uint16_t power);

	std::vector<IpMacAddress> lamps;
	int transition;
	static bool isRegistered;
};
