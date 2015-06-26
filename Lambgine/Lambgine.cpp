// Lambgine.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Lambgine.h"
#include "lua\LambLua.h"

struct Lambgine::LambgineImpl
{
	LambLua luaEngine;
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
