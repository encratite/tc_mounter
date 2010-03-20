#include <ail/exception.hpp>
#include <comdef.h>

struct com_handler
{
	com_handler();
	~com_handler();
};

struct security_handler
{
	security_handler();
};

struct locator_handler
{
	IWbemLocator * locator = 0;

	locator_handler();
	~locator_handler();
};

struct server_handler
{
	IWbemServices * services;

	server_handler();
	~server_handler();
};