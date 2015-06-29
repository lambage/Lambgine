// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LAMBGINE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LAMBGINE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef LAMBGINE_H
#define LAMBGINE_H

#ifdef LAMBGINE_EXPORTS
#define LAMBGINE_API __declspec(dllexport)
#else
#define LAMBGINE_API __declspec(dllimport)
#endif

#include <Lambgine\Log.h>
#include <Lambgine\lua\LambLua.h>  //for external reference
#include <Lambgine\LambWindow.h>   //for external reference

#include <functional>
#include <memory>

class LambLua;
class LambWindow;

class Lambgine
{
public:
	LAMBGINE_API Lambgine();
	LAMBGINE_API ~Lambgine();

	LAMBGINE_API LambLua& Lua() const;

	LAMBGINE_API std::shared_ptr<LambWindow> NewWindow(const char *title, int width, int height, int monitor);

private:
	struct LambgineImpl;
	std::unique_ptr<LambgineImpl> mImpl;
};

#endif