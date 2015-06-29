// Lambgine.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Lambgine.h"
#include "lua\LambLua.h"

#include <vector>
#include <deque>
#include <functional>

#include <future>
#include <thread>
#include <mutex>
#include <atomic>
#include <list>

struct Lambgine::LambgineImpl
{
	LambLua luaEngine;

	std::list<std::shared_ptr<LambWindow>> mWindows;
	std::mutex mWindowsMutex;

	std::atomic<bool> mRunning;
	std::thread mGuiThread;

	LambgineImpl()
	{
		mGuiThread = std::thread([this](){
			if (!glfwInit())
				exit(EXIT_FAILURE);

			MainLoop();

			mWindows.clear();
		});
	}

	~LambgineImpl()
	{
		mRunning = false;
		mGuiThread.join();

		glfwTerminate();
	}

	void MainLoop()
	{
		double lastTime = glfwGetTime();
		mRunning = true;

		while (mRunning)
		{
			double now = glfwGetTime();
			double delta = now - lastTime;

			{
				std::unique_lock<std::mutex> lock(tasksMutex);
				while (!tasks.empty())
				{
					auto task(tasks.front());
					tasks.pop_front();
					lock.unlock();
					(*task)();
					lock.lock();
				}
			}

			if (mWindows.empty())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
			else
			{
				std::unique_lock<std::mutex> lock(mWindowsMutex);
				for (auto &it = mWindows.begin(); it != mWindows.end();)
				{
					(*it)->Refresh();
					if ((*it)->ShouldClose())
					{
						mWindows.erase(it++);
					}
					else
					{
						++it;
					}
				}
			}

			glfwPollEvents();

			lastTime = now;
		}

	}

	std::shared_ptr<LambWindow> NewWindow(const char *title, int width, int height, int monitor)
	{
		Invoke([&]() {
			std::unique_lock<std::mutex> lock(mWindowsMutex);
			mWindows.emplace_back(std::make_shared<LambWindow>(title, width, height, monitor));
			lock.unlock();
		});
		return mWindows.back();
	}

	void Invoke(std::function<void()> function)
	{
		std::packaged_task<void()> task(function);
		std::future<void> result = task.get_future();
		{
			std::lock_guard<std::mutex> lock(tasksMutex);
			tasks.push_back(&task);
		}
		result.get();
	}

	std::deque<std::packaged_task<void()> *> tasks;
	std::mutex tasksMutex;
};

LAMBGINE_API Lambgine::Lambgine() :
mImpl(std::make_unique<Lambgine::LambgineImpl>())
{
}

LAMBGINE_API Lambgine::~Lambgine()
{
}

LAMBGINE_API LambLua& Lambgine::Lua() const
{
	return mImpl->luaEngine;
}

LAMBGINE_API std::shared_ptr<LambWindow> Lambgine::NewWindow(const char *title, int width, int height, int monitor)
{
	return mImpl->NewWindow(title, width, height, monitor);
}
