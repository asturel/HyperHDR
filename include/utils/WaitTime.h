#pragma once

#include <QEventLoop>
#include <QTimer>

#include <chrono>

inline void wait(int millisecondsWait)
{
	QEventLoop loop;
	QTimer t;
	t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
	t.start(millisecondsWait);
	loop.exec();
}

