#include <ail/exception.hpp>

#define _WIN32_DCOM

#include <comdef.h>
#include <Wbemidl.h>

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
	IWbemLocator * locator;

	locator_handler();
	~locator_handler();
};

struct server_handler
{
	IWbemServices * services;

	server_handler(locator_handler & locator);
	~server_handler();
};