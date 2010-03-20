#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <ail/configuration.hpp>
#include <ail/file.hpp>
#include <ail/array.hpp>
#include <ail/types.hpp>
#include "com.hpp"

struct serial_number_entry
{
	std::string
		serial_number,
		disk_identifier;

	serial_number_entry();
	serial_number_entry(std::string const & name, std::string const & serial_number);
	bool operator==(std::string const & input) const;
};

typedef std::vector<serial_number_entry> serial_number_vector;

serial_number_entry::serial_number_entry()
{
}

serial_number_entry::serial_number_entry(std::string const & name, std::string const & serial_number):
	serial_number(serial_number)
{
	std::string const target = "\\\\.\\PHYSICALDRIVE";
	if(name.size() <= target.size() || name.substr(0, target.size()) != target)
		throw ail::exception("Encountered an invalid physical drive identifier: " + name);
	disk_identifier = name.substr(target.size());
}

bool serial_number_entry::operator==(std::string const & input) const
{
	return serial_number == input;
}

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

std::string convert_serial_number(std::string const & input)
{
	std::string output;
	std::size_t const group_size = 2;
	for(std::size_t i = 0; i < input.size(); i += group_size)
	{
		std::string group = input.substr(i, group_size);
		std::istringstream stream(group);
		int letter;
		if(!(stream >> std::hex >> letter))
			throw ail::exception("Failed to convert serial number data " + input);
		output.push_back(static_cast<char>(letter));
	}
	output = ail::right_trim(output);
	return output;
}

void perform_query(string_vector & names, string_vector & serial_numbers)
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

		serial_number = convert_serial_number(serial_number);

		names.push_back(name);
		serial_numbers.push_back(serial_number);
	}

	enumerator->Release();
}

void print_help(char ** argv)
{
	std::cout << "Usage: " << std::endl;
	std::cout << argv[0] << " list: List devices and their serial numbers" << std::endl;
	std::cout << argv[0] << " mount <configuration file>: Mount devices as specified in the configuration file and the disk file" << std::endl;
}

int main(int argc, char ** argv)
{
	if(argc < 2)
	{
		print_help(argv);
		return 1;
	}

	std::string operation(argv[1]);

	try
	{
		string_vector
			names,
			serial_numbers;

		if(operation == "list")
		{
			perform_query(names, serial_numbers);

			uword counter = 1;
			std::cout << "List of hard disks and their serial numbers:" << std::endl;
			for(std::size_t i = 0; i < names.size(); i++, counter++)
				std::cout << counter << ". " << names[i] << ": " << serial_numbers[i] << std::endl;
		}
		else if(operation == "mount")
		{
			if(argc != 3)
			{
				std::cout << "Invalid argument count for mounting." << std::endl;
				print_help(argv);
				return 1;
			}

			std::string configuration_file(argv[2]);
			ail::configuration configuration;
			if(!configuration.load(configuration_file))
			{
				std::cout << "Failed to load configuration file " << configuration_file << std::endl;
				return 1;
			}

			std::string
				disk_file = configuration.string("disk_file"),
				command_line = configuration.string("command_line");

			string_vector lines;
			if(!ail::read_lines(disk_file, lines))
			{
				std::cout << "Unable to read disk file " << disk_file << std::endl;
				return 1;
			}

			perform_query(names, serial_numbers);

			serial_number_vector serial_number_entries;
			for(std::size_t i = 0; i < names.size(); i++)
				serial_number_entries.push_back(serial_number_entry(names[i], serial_numbers[i]));

			for(string_vector::iterator i = lines.begin(); i != lines.end(); i++)
			{
				string_vector tokens = ail::tokenise(*i, " ");
				std::string const
					& drive_letter = tokens[0],
					& serial_number = tokens[1];

				serial_number_vector::iterator iterator = std::find(serial_number_entries.begin(), serial_number_entries.end(), serial_number);
				if(iterator == serial_number_entries.end())
					throw ail::exception("Unable to find a disk entry for serial number " + serial_number);

				std::string const targets[] =
				{
					"$DISK_IDENTIFIER$",
					"$DRIVE_LETTER$"
				};

				std::string replacements[] =
				{
					iterator->disk_identifier,
					drive_letter
				};

				std::string command = command_line;
				for(std::size_t i = 0; i < ail::countof(targets); i++)
					command = ail::replace_string(command, targets[i], replacements[i]);

				std::cout << "Executing " << command << std::endl;
			}
		}
		else
		{
			std::cout << "Invalid operation specified." << std::endl;
			print_help(argv);
			return 1;
		}
	}
	catch(ail::exception & exception)
	{
		std::cout << "An exception occured: " << exception.get_message() << std::endl;
		return 1;
	}

	return 0;	
}
