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
#include <Lambgine\LambWindow.h>
#include <Lambgine\lua\LambLua.h>

class Lambgine
{
public:
	LAMBGINE_API Lambgine();
	LAMBGINE_API ~Lambgine();

	LAMBGINE_API LambLua& Lua() const;

private:
	struct LambgineImpl;
	std::unique_ptr<LambgineImpl> mImpl;
};

#endif