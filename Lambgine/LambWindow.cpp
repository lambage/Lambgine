#include "stdafx.h"
#include "LambWindow.h"

#include <stdexcept>
#include <vector>

#include "Log.h"

#include <deque>
#include <mutex>
#include <future>
#include <thread>

static void KeyHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
static void CharHandler(GLFWwindow* window, unsigned int codepoint);
static void CursorHandler(GLFWwindow* window, double xpos, double ypos);
static void MouseButtonHandler(GLFWwindow* window, int button, int action, int mods);
static void ScrollHandler(GLFWwindow* window, double xoffset, double yoffset);
static void SizeHandler(GLFWwindow* window, int width, int height);

static void WindowRefreshHandler(GLFWwindow *window);

static const int defaultWindowWidth = 640;
static const int defaultWindowHeight = 480;

/******************************************************/
/* LambWindow private implementation                */
/******************************************************/

struct LambWindow::LambWindowImpl
{
	LambWindowImpl(const char *title, int monitor) :
		mWindowTitle(title),
		mWindow(nullptr),
		mLastMonitor(-1)
	{
		if (!glfwInit())
		{
			throw std::runtime_error("failed to initialize glfw");
		}

		SetMonitor(monitor);
	}

	~LambWindowImpl()
	{
		if (mWindow != nullptr)
		{
			glfwDestroyWindow(mWindow);
		}
		mWindow = nullptr;
	}

	void SetSize(int width, int height)
	{
		mWindowWidth = width;
		mWindowHeight = height;
	}

	void SetUserPointer(void *pointer)
	{
		glfwSetWindowUserPointer(mWindow, pointer);
	}

	bool SetMonitor(int monitor)
	{
		if (monitor == mLastMonitor) return false;

		QueryMonitors();

		//assigning to a new window first allows us to share the context so gl objects are not destroyed between switching windows
		GLFWwindow* newWindow = nullptr;

		if (monitor > 0 && monitor <= mMonitorCount)
		{
			const GLFWvidmode* mode = glfwGetVideoMode(mMonitors[monitor - 1]);
			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
			newWindow = glfwCreateWindow(mode->width, mode->height, mWindowTitle, mMonitors[monitor - 1], mWindow);
			SetSize(mode->width, mode->height);
		}
		else
		{
			newWindow = glfwCreateWindow(defaultWindowWidth, defaultWindowHeight, mWindowTitle, NULL, mWindow);
			SetSize(defaultWindowWidth, defaultWindowHeight);
		}

		glfwDestroyWindow(mWindow);
		mWindow = newWindow;

		if (mWindow == nullptr)
		{
			throw std::runtime_error("failed to create glfw window");
		}
		mLastMonitor = monitor;

		return true;
	}

	GLFWwindow* GetWindow()
	{
		return mWindow;
	}

	void Close()
	{
		glfwSetWindowShouldClose(mWindow, GL_TRUE);
	}

	void QueryMonitors()
	{
		mMonitors = glfwGetMonitors(&mMonitorCount);
	}

	void ListMonitors()
	{
		QueryMonitors();

		Log("Displaying monitor information:\n");
		for (int i = 0; i < mMonitorCount; i++)
		{
			const GLFWvidmode* mode = glfwGetVideoMode(mMonitors[i]);
			const char * name = glfwGetMonitorName(mMonitors[i]);
			Log("%d. %s w=%d h=%d rgb=%d,%d,%d refresh=%dhz\n", i + 1, name, mode->width, mode->height, mode->redBits, mode->greenBits, mode->blueBits, mode->refreshRate);
		}
	}

	std::vector<GLFWmonitor *> GetMonitors()
	{
		QueryMonitors();
		std::vector<GLFWmonitor *> monitors;
		for (int i = 0; i < mMonitorCount; i++)
		{
			monitors.push_back(mMonitors[i]);
		}
		return monitors;
	}

	void Invoke(std::function<void()> function)
	{
		std::packaged_task<void()> task(function);
		std::future<void> result = task.get_future();
		{
			std::lock_guard<std::mutex> lock(tasks_mutex);
			tasks.push_back(&task);
		}
		result.get();
	}

	GLFWwindow* mWindow;

	const char * mWindowTitle;

	GLFWmonitor ** mMonitors;
	int mMonitorCount;
	int mLastMonitor;
	int mWindowWidth;
	int mWindowHeight;

	std::deque<std::packaged_task<void()> *> tasks;
	std::mutex tasks_mutex;
};


/******************************************************/
/* LambWindow public interface implementation       */
/******************************************************/

LAMBGINE_API LambWindow::LambWindow(const char *title, int monitor) :
mImpl(std::make_unique<LambWindow::LambWindowImpl>(title, monitor))
{
	glfwMakeContextCurrent(mImpl->GetWindow());
	glfwSwapInterval(1);
}

LAMBGINE_API LambWindow::~LambWindow()
{
}


LAMBGINE_API void LambWindow::MainLoop()
{
	//sets up context and function callback pointers
	_internalSetup();

	//pure virtual setup method
	Setup();

	while (!glfwWindowShouldClose(mImpl->GetWindow()))
	{
		Refresh();

		glfwSwapBuffers(mImpl->GetWindow());
		glfwPollEvents();
	}
}

LAMBGINE_API void LambWindow::Refresh()
{
	{
		std::unique_lock<std::mutex> lock(mImpl->tasks_mutex);
		while (!mImpl->tasks.empty())
		{
			auto task(mImpl->tasks.front());
			mImpl->tasks.pop_front();
			lock.unlock();
			(*task)();
			lock.lock();
		}
	}

	//pure virtual render method
	Render(glfwGetTime());
}

LAMBGINE_API bool LambWindow::KeyHandler(int key, int scancode, int action, int mods)
{
	bool retVal = false;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(GetWindow(), GL_TRUE);
		retVal = true;
	}

	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9 && mods == (GLFW_MOD_CONTROL | GLFW_MOD_ALT) && action == GLFW_PRESS)
	{
		int display = key - GLFW_KEY_1;
		_internalSetMonitor(display + 1);
	}

	if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
	{
		if (mods == 0)
		{
			_internalSetMonitor(0);
		}
		else if (mods == GLFW_MOD_CONTROL)
		{
			_internalSetMonitor(1);
		}
		else if (mods == GLFW_MOD_ALT)
		{
			_internalSetMonitor(2);
		}
		else if (mods == GLFW_MOD_SHIFT)
		{
			_internalSetMonitor(3);
		}
		else if (mods == (GLFW_MOD_CONTROL | GLFW_MOD_ALT))
		{
			_internalSetMonitor(4);
		}
		else if (mods == (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT))
		{
			_internalSetMonitor(5);
		}
		else if (mods == (GLFW_MOD_ALT | GLFW_MOD_SHIFT))
		{
			_internalSetMonitor(6);
		}
		else if (mods == (GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SHIFT))
		{
			_internalSetMonitor(7);
		}
	}

	return retVal;
}

LAMBGINE_API bool LambWindow::CharHandler(unsigned int codepoint)
{
	return false;
}

LAMBGINE_API bool LambWindow::CursorHandler(double xpos, double ypos)
{
	return false;
}

LAMBGINE_API bool LambWindow::MouseButtonHandler(int button, int action, int mods)
{
	return false;
}

LAMBGINE_API bool LambWindow::ScrollHandler(double xoffset, double yoffset)
{
	return false;
}

LAMBGINE_API bool LambWindow::SizeHandler(int width, int height)
{
	mImpl->SetSize(width, height);
	return false;
}

LAMBGINE_API void LambWindow::Close()
{
	mImpl->Close();
}

LAMBGINE_API void LambWindow::Setup()
{

}

LAMBGINE_API void LambWindow::Render(double ms)
{

}

LAMBGINE_API GLFWwindow* LambWindow::GetWindow()
{
	return mImpl->GetWindow();
}

LAMBGINE_API void LambWindow::ListMonitors()
{
	mImpl->ListMonitors();
}

LAMBGINE_API void LambWindow::Invoke(std::function<void()> function)
{
	mImpl->Invoke(function);
}

void LambWindow::_internalSetup()
{
	glfwMakeContextCurrent(mImpl->GetWindow());
	glfwSwapInterval(1);

	glfwSetWindowUserPointer(mImpl->GetWindow(), this);
	glfwSetKeyCallback(mImpl->GetWindow(), ::KeyHandler);
	glfwSetCharCallback(mImpl->GetWindow(), ::CharHandler);
	glfwSetCursorPosCallback(mImpl->GetWindow(), ::CursorHandler);
	glfwSetMouseButtonCallback(mImpl->GetWindow(), ::MouseButtonHandler);
	glfwSetScrollCallback(mImpl->GetWindow(), ::ScrollHandler);
	glfwSetWindowSizeCallback(mImpl->GetWindow(), ::SizeHandler);

	glfwSetWindowRefreshCallback(mImpl->GetWindow(), ::WindowRefreshHandler);
}

void LambWindow::_internalSetMonitor(int monitor)
{
	auto monitors = mImpl->GetMonitors();
	if (monitor >= 0 && monitor <= (int)monitors.size())
	{
		if (mImpl->SetMonitor(monitor))
		{
			Log("Selecting monitor %d\n", monitor);
			_internalSetup();
		}
	}
	else
	{
		LogError("%d is not a valid display\n", monitor);
	}
}



/******************************************************/
/* static handlers                                    */
/******************************************************/

static void KeyHandler(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto handler = static_cast<LambWindow *>(glfwGetWindowUserPointer(window));
	handler->KeyHandler(key, scancode, action, mods);
}

static void CharHandler(GLFWwindow* window, unsigned int codepoint)
{
	auto handler = static_cast<LambWindow *>(glfwGetWindowUserPointer(window));
	handler->CharHandler(codepoint);
}

static void CursorHandler(GLFWwindow* window, double xpos, double ypos)
{
	auto handler = static_cast<LambWindow *>(glfwGetWindowUserPointer(window));
	handler->CursorHandler(xpos, ypos);
}

static void MouseButtonHandler(GLFWwindow* window, int button, int action, int mods)
{
	auto handler = static_cast<LambWindow *>(glfwGetWindowUserPointer(window));
	handler->MouseButtonHandler(button, action, mods);
}

static void ScrollHandler(GLFWwindow* window, double xoffset, double yoffset)
{
	auto handler = static_cast<LambWindow *>(glfwGetWindowUserPointer(window));
	handler->ScrollHandler(xoffset, yoffset);
}

static void SizeHandler(GLFWwindow* window, int width, int height)
{
	auto handler = static_cast<LambWindow *>(glfwGetWindowUserPointer(window));
	handler->SizeHandler(width, height);
}

static void WindowRefreshHandler(GLFWwindow *window)
{
	auto handler = static_cast<LambWindow *>(glfwGetWindowUserPointer(window));
	handler->Refresh();
}