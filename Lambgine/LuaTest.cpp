#include "stdafx.h"
#include "LuaTest.h"

#include "Test.h"

#include <luacppinterface.h>

LuaUserdata<Test> testConstructor(Lua lua, std::string str)
{
	auto test = new Test(str);
	auto userData = lua.CreateUserdata<Test>(test);

	userData.Bind("Add", &Test::Add);
	userData.Bind("Identify", &Test::Identify);

	return userData;
}

void OpenTest(Lua* lua)
{
	auto global = lua->GetGlobalEnvironment();

	auto newTest = lua->CreateFunction<LuaUserdata<Test>(std::string)>(std::bind(&testConstructor, *lua, std::placeholders::_1));
	auto testtable = lua->CreateTable();
	testtable.Set("new", newTest);
	global.Set("Test", testtable);
}
