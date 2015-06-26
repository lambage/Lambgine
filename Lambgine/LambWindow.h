#pragma once
#include <Lambgine/Lambgine.h>
#include <memory>
#include <string>

#include <memory>
#include <functional>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 
#include <winsock2.h>
#endif
#include <Windows.h>
#include <GLFW/glfw3.h>

class LambWindow
{
public:
	LAMBGINE_API LambWindow(const char *title, int monitor = 0);
	LAMBGINE_API virtual ~LambWindow();

	LAMBGINE_API virtual bool KeyHandler(int key, int scancode, int action, int mods);
	LAMBGINE_API virtual bool CharHandler(unsigned int codepoint);
	LAMBGINE_API virtual bool CursorHandler(double xpos, double ypos);
	LAMBGINE_API virtual bool MouseButtonHandler(int button, int action, int mods);
	LAMBGINE_API virtual bool ScrollHandler(double xoffset, double yoffset);
	LAMBGINE_API virtual bool SizeHandler(int width, int height);

	LAMBGINE_API virtual void Refresh();

	LAMBGINE_API void Close();

	LAMBGINE_API virtual void Setup();
	LAMBGINE_API virtual void Render(double ms);

	LAMBGINE_API void MainLoop();

	LAMBGINE_API GLFWwindow* GetWindow();

	LAMBGINE_API void ListMonitors();

	LAMBGINE_API void Invoke(std::function<void()> function);

private:
	struct LambWindowImpl;
	std::unique_ptr<LambWindowImpl> mImpl;

	void _internalSetMonitor(int monitor);
	void _internalSetup();
};
