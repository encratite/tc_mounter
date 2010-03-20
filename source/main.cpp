#define _WIN32_DCOM

#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>

struct security_handler
{
	security_handler();
};

int main(int argc, char ** argv)
{
	HRESULT result = CoInitializeEx(0, COINIT_MULTITHREADED); 
	if(FAILED(result))
	{
		std::cout << "Failed to initialise COM library. Error code = 0x" << std::hex << result << std::endl;
		return 1;
	}

	result = CoInitializeSecurity
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
	{
		std::cout << "Failed to initialise security. Error code = 0x" << std::hex << result << std::endl;
		CoUninitialize();
		return 1;
	}

	IWbemLocator * locator = 0;

	result = CoCreateInstance
	(
		CLSID_WbemLocator,			 
		0, 
		CLSCTX_INPROC_SERVER, 
		IID_IWbemLocator,
		reinterpret_cast<LPVOID *>(&locator)
	);
 
	if(FAILED(result))
	{
		std::cout << "Failed to create IWbemLocator object. Err code = 0x" << std::hex << result << std::endl;
		CoUninitialize();
		return 1;
	}

	IWbemServices * services = 0;
	
	result = locator->ConnectServer
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
	{
		std::cout << "Could not connect. Error code = 0x" << std::hex << result << std::endl;
		locator->Release();	 
		CoUninitialize();
		return 1;
	}

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
	{
		std::cout << "Could not set proxy blanket. Error code = 0x" << std::hex << result << std::endl;
		services->Release();
		locator->Release();	 
		CoUninitialize();
		return 1;
	}

	IEnumWbemClassObject * enumerator = 0;
	result = services->ExecQuery
	(
		bstr_t("WQL"), 
		bstr_t("select * from Win32_OperatingSystem"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
		0,
		&enumerator
	);
	
	if(FAILED(result))
	{
		std::cout << "Query for operating system name failed. Error code = 0x" << std::hex << result << std::endl;
		services->Release();
		locator->Release();
		CoUninitialize();
		return 1;
	}

	IWbemClassObject * class_object;
	ULONG return_value = 0;
 
	while(enumerator)
	{
		HRESULT next_result = enumerator->Next(WBEM_INFINITE, 1, &class_object, &return_value);

		if(return_value == 0)
			break;

		VARIANT variant_property;

		next_result = class_object->Get(L"Name", 0, &variant_property, 0, 0);
		std::wcout << "OS Name: " << variant_property.bstrVal << std::endl;
		VariantClear(&variant_property);

		class_object->Release();
	}

	services->Release();
	locator->Release();
	enumerator->Release();
	CoUninitialize();

	return 0;	
}
