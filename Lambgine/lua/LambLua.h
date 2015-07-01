#pragma once

#include <Lambgine\LambgineExports.h>
#include <memory>
struct lua_State;

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

