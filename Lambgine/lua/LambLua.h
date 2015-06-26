#pragma once
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <memory>

class LambLua
{
public:
	LAMBGINE_API LambLua();
	LAMBGINE_API ~LambLua();

	LAMBGINE_API lua_State* GetLuaState();
	LAMBGINE_API int DoTerminal();

private:
	struct LambLuaImpl;
	std::unique_ptr<LambLuaImpl> mImpl;
};

