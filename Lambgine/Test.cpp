#include "stdafx.h"
#include "Test.h"
#include <iostream>

Test::Test(std::string name) :
name(name)
{
	std::cout << "Created Test " << name << std::endl;
}


Test::~Test()
{
	std::cout << "Destroyed Test " << name << std::endl;
}

int Test::Add(int x, int y)
{
	std::cout << (x + y) << std::endl;
	return x + y;
}

void Test::Identify()
{
	std::cout << "Identity " << name << std::endl;
}