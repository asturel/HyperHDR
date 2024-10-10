/* SuspendHandlerLinux.cpp
*
*  MIT License
*
*  Copyright (c) 2020-2024 awawa-dev
*
*  Project homesite: https://github.com/awawa-dev/HyperHDR
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.

*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
 */



#include <algorithm>
#include <cstdint>
#include <limits>
#include <cmath>
#include <cassert>
#include <csignal>
#include <stdlib.h>
#include <suspend-handler/SuspendHandlerWebOS.h>
#include <utils/Components.h>
#include <image/Image.h>
#include <base/HyperHdrManager.h>
#include <iostream>



SuspendHandler::SuspendHandler(bool sessionLocker)
{
	me = this;
	try
	{
		signal(SIGCHLD, SuspendHandler::signal_handler);
		signal(SIGUSR1, SuspendHandler::signal_handler);
		signal(SIGUSR2, SuspendHandler::signal_handler);
		std::cout << "THE SLEEP HANDLER IS REGISTERED!" << std::endl;
	}
	catch (std::exception& ex)
	{
		std::cout << "COULD NOT REGISTER THE SLEEP HANDLER: " << ex.what() << std::endl;
	}
}

SuspendHandler::~SuspendHandler()
{
	try
	{
		std::cout << "THE SLEEP HANDLER IS DEREGISTERED!" << std::endl;
	}
	catch (std::exception& ex)
	{
		std::cout << "COULD NOT DEREGISTER THE SLEEP HANDLER: " << ex.what() << std::endl;
	}
}

void SuspendHandler::sleeping(bool sleep)
{
	if (sleep)
	{
		std::cout << "OS event: going to sleep" << std::endl;
		emit SignalHibernate(false, hyperhdr::SystemComponent::SUSPEND);
	}
	else
	{
		std::cout << "OS event: waking up" << std::endl;
		emit SignalHibernate(true, hyperhdr::SystemComponent::SUSPEND);
	}
}

void SuspendHandler::signal_handler(int signum)
{
	/*
	if (signum == SIGCHLD)
	{

	}
	else if (signum == SIGUSR1)
	{
		std::cout << "OS event: going to sleep" << std::endl;
		emit SignalHibernate(false, hyperhdr::SystemComponent::SUSPEND);
	}
	else if (signum == SIGUSR2)
	{
		std::cout << "OS event: waking up" << std::endl;
		emit SignalHibernate(true, hyperhdr::SystemComponent::SUSPEND);
	}
	*/
	if (signum == SIGCHLD)
	{

	}
	else if (signum == SIGUSR1)
	{
		std::cout << "OS event: going to sleep" << std::endl;
		me->sleeping(true);
	}
	else if (signum == SIGUSR2)
	{
		std::cout << "OS event: waking up" << std::endl;
		me->sleeping(false);
	}
}
