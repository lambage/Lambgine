// LambgineTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Lambgine/Lambgine.h>
#include <thread>

int _tmain(int argc, _TCHAR* argv[])
{
	int retVal = 0;
	try
	{
		Lambgine engine;
		//engine.NewWindow("one", 640, 480, 0);
		//engine.NewWindow("two", 640, 480, 0);

		retVal = engine.Lua().DoTerminal();
	}
	catch (std::runtime_error ex)
	{
		LogError("%s\n", ex.what());
		return -1;
	}

	return retVal;
}

