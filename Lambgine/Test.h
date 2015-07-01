#pragma once
#include <string>
class Test
{
public:
	Test(std::string name);
	~Test();

	int Add(int x, int y);
	void Identify();
private:
	std::string name;
};

