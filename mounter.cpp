#define _WIN32_DCOM
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>

int main(int argc, char ** argv)
{
	HRESULT result = CoInitializeEx(0, COINIT_MULTITHREADED); 
	if(FAILED(result))
	{
		std::cout << "Failed to initialize COM library. Error code = 0x" << std::hex << result << std::endl;
		return 1;
	}

	result = CoInitializeSecurity
	(
		0, 
		-1, //COM authentication
		0, //Authentication services
		0, //Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT, //Default authentication 
		RPC_C_IMP_LEVEL_IMPERSONATE, //Default Impersonation 
		0, //Authentication info
		EOAC_NONE, //Additional capabilities 
		0 //Reserved
	);

					 
	if(FAILED(result))
	{
		std::cout << "Failed to initialize security. Error code = 0x" << std::hex << result << std::endl;
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

	IWbemServices * pSvc = 0;
	
	result = locator->ConnectServer(
		 _bstr_t(L"ROOT\\CIMV2"), //Object path of WMI namespace
		 0,					//User name. 0 = current user
		 0,					//User password. 0 = current
		 0,					 //Locale. 0 indicates current
		 0,					//Security flags.
		 0,					 //Authority (e.g. Kerberos)
		 0,					 //Context object 
		 &pSvc					//pointer to IWbemServices proxy
		 );
	
	if(FAILED(result))
	{
		std::cout << "Could not connect. Error code = 0x" 
			 << std::hex << result << std::endl;
		locator->Release();	 
		CoUninitialize();
		return 1;				//Program has failed.
	}

	std::cout << "Connected to ROOT\\CIMV2 WMI namespace" << std::endl;


	//Step 5: --------------------------------------------------
	//Set security levels on the proxy -------------------------

	result = CoSetProxyBlanket(
	 pSvc,						//Indicates the proxy to set
	 RPC_C_AUTHN_WINNT,		 //RPC_C_AUTHN_xxx
	 RPC_C_AUTHZ_NONE,			//RPC_C_AUTHZ_xxx
	 0,						//Server principal name 
	 RPC_C_AUTHN_LEVEL_CALL,	 //RPC_C_AUTHN_LEVEL_xxx 
	 RPC_C_IMP_LEVEL_IMPERSONATE, //RPC_C_IMP_LEVEL_xxx
	 0,						//client identity
	 EOAC_NONE					//proxy capabilities 
	);

	if(FAILED(result))
	{
		std::cout << "Could not set proxy blanket. Error code = 0x" 
			<< std::hex << result << std::endl;
		pSvc->Release();
		locator->Release();	 
		CoUninitialize();
		return 1;			 //Program has failed.
	}

	//Step 6: --------------------------------------------------
	//Use the IWbemServices pointer to make requests of WMI ----

	//For example, get the name of the operating system
	IEnumWbemClassObject* pEnumerator = 0;
	result = pSvc->ExecQuery(
		bstr_t("WQL"), 
		bstr_t("SELECT * FROM Win32_OperatingSystem"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
		0,
		&pEnumerator);
	
	if(FAILED(result))
	{
		std::cout << "Query for operating system name failed."
			<< " Error code = 0x" 
			<< std::hex << result << std::endl;
		pSvc->Release();
		locator->Release();
		CoUninitialize();
		return 1;			 //Program has failed.
	}

	//Step 7: -------------------------------------------------
	//Get the data from the query in step 6 -------------------
 
	IWbemClassObject *pclsObj;
	ULONG uReturn = 0;
 
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
			&pclsObj, &uReturn);

		if(0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;

		//Get the value of the Name property
		hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
		std::wcout << " OS Name : " << vtProp.bstrVal << std::endl;
		VariantClear(&vtProp);

		pclsObj->Release();
	}

	pSvc->Release();
	locator->Release();
	pEnumerator->Release();
	CoUninitialize();

	return 0;	
}
