#ifndef TARGET_H
#define TARGET_H

#include <stdint.h>
#include <QString>

class Target
{
private:
	uint32_t rom_base_address, ram_base_address;
	QByteArray rom, ram, register_file;
public:
	Target(const QString & rom_filename, uint32_t rom_base_address, const QString & ram_filename, uint32_t ram_base_address, const QString register_filename);
	uint32_t readWord(uint32_t address);
	uint32_t readRegister(uint32_t register_number);
};

#endif // TARGET_H
