#include "com.hpp"

com_handler::com_handler()
{
	HRESULT result = CoInitializeEx(0, COINIT_MULTITHREADED); 
	if(FAILED(result))
		throw ail::exception("Failed to initialise COM library");
}

com_handler::~com_handler()
{
	CoUninitialize();
}

security_handler::security_handler()
{
	HRESULT result = CoInitializeSecurity
	(
		0, 
		//COM authentication
		-1,
		//Authentication services
		0,
		//Reserved
		0,
		//Default authentication
		RPC_C_AUTHN_LEVEL_DEFAULT,
		//Default Impersonation 
		RPC_C_IMP_LEVEL_IMPERSONATE,
		//Authentication info
		0,
		//Additional capabilities 
		EOAC_NONE,
		//Reserved
		0
	);

	if(FAILED(result))
		throw ail::exception("Failed to initialise COM security");
}

locator_handler::locator_handler():
	locator(0)
{
	HRESULT result = CoCreateInstance
	(
		CLSID_WbemLocator,			 
		0, 
		CLSCTX_INPROC_SERVER, 
		IID_IWbemLocator,
		reinterpret_cast<LPVOID *>(&locator)
	);

	if(FAILED(result))
		throw ail::exception("Failed to create COM locator");
}

locator_handler::~locator_handler()
{
	locator->Release();
}

server_handler::server_handler(locator_handler & locator):
	services(0)
{
	HRESULT result = locator.locator->ConnectServer
	(
		//Object path of WMI namespace
		 _bstr_t(L"ROOT\\CIMV2"),
		 //User name. 0 = current user
		 0,
		 //User password. 0 = current
		 0,
		 //Locale. 0 indicates current
		 0,
		 //Security flags.
		 0,
		 //Authority (e.g. Kerberos)
		 0,
		 //Context object 
		 0,
		 //pointer to IWbemServices proxy
		 &services
	);

	if(FAILED(result))
		throw ail::exception("Failed to create connect to WMI server");

	result = CoSetProxyBlanket
	(
		//Indicates the proxy to set
		services,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		0,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		//client identity
		0,
		//proxy capabilities 
		EOAC_NONE
	);

	if(FAILED(result))
		throw ail::exception("Failed to set proxy");
}

server_handler::~server_handler()
{
	services->Release();
}
