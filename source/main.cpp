#include <iostream>
#include <string>
#include <ail/configuration.hpp>
#include <ail/array.hpp>
#include <ail/types.hpp>
#include "com.hpp"

std::string wchar_to_string(wchar_t * input)
{
	char const * default_char = "_";
	int size = WideCharToMultiByte(CP_ACP, 0, input, -1, 0, 0, default_char, 0);
	char * buffer = new char[size];
	WideCharToMultiByte(CP_ACP, 0, input, -1, buffer, size, default_char, 0);
	std::string output(buffer, static_cast<std::size_t>(size - 1));
	delete buffer;
	return output;
}

void perform_query(bool list_devices_mode)
{
	com_handler com_object;
	security_handler security_object;
	locator_handler locator_object;
	server_handler server_object(locator_object);

	IEnumWbemClassObject * enumerator = 0;
	HRESULT result = server_object.services->ExecQuery
	(
		bstr_t("WQL"), 
		bstr_t("select * from Win32_DiskDrive"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
		0,
		&enumerator
	);
	
	if(FAILED(result))
		throw ail::exception("Query failed");

	IWbemClassObject * class_object;
	ULONG return_value = 0;

	uword counter = 1;
 
	while(enumerator)
	{
		result = enumerator->Next(WBEM_INFINITE, 1, &class_object, &return_value);

		if(return_value == 0)
			break;

		std::string
			name,
			serial_number;

		std::string * string_pointers[] =
		{
			&name,
			&serial_number
		};

		LPCWSTR property_strings[] =
		{
			L"Name",
			L"SerialNumber"
		};

		for(std::size_t i = 0; i < ail::countof(string_pointers); i++)
		{
			VARIANT variant_property;
			result = class_object->Get(property_strings[i], 0, &variant_property, 0, 0);
			*string_pointers[i] = wchar_to_string(variant_property.bstrVal);
			VariantClear(&variant_property);
		}

		class_object->Release();

		if(list_devices_mode)
			std::cout << counter << ". " << name << ": " << serial_number << std::endl;

		counter++;
	}

	enumerator->Release();
}

int main(int argc, char ** argv)
{
	try
	{
		perform_query(true);
	}
	catch(ail::exception & exception)
	{
		std::cout << "An exception occured: " << exception.get_message() << std::endl;
	}
	return 0;	
}
