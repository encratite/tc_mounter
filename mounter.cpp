#include <iostream>
#include <string>
#include <ail/windows.hpp>
#include <ail/types.hpp>

bool get_disk_information(std::string const & path, uword & serial_number)
{
	std::size_t const buffer_size = 1024;

	char
		volume_name[buffer_size],
		file_system_name[buffer_size];

	DWORD
		volume_serial_number,
		maximum_component_length,
		file_system_flags;

	if(!GetVolumeInformation(path.c_str(), volume_name, buffer_size, &volume_serial_number, &maximum_component_length, &file_system_flags, file_system_name, buffer_size))
		return false;

	serial_number = volume_serial_number;

	return true;
}

int main()
{
	uword serial_number;
	//if(!get_disk_information("\\Device\\Harddisk1\\Partition1", serial_number))
	if(!get_disk_information("\\\\?\\Volume{4d36e967-e325-11ce-bfc1-08002be10318}", serial_number))
	{
		std::cout << "Error: " << GetLastError() << std::endl;
		std::cin.get();
		return 1;
	}
	std::cout << "Serial: " << std::hex << serial_number << std::endl;
	std::cin.get();
	return 0;
}
