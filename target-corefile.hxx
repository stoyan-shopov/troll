#ifndef TARGETCOREFILE_HXX
#define TARGETCOREFILE_HXX

#include <QString>
#include "target.hxx"

class TargetCorefile : public Target
{
private:
	uint32_t rom_base_address, ram_base_address;
	QByteArray rom, ram, register_file;
public:
	TargetCorefile(const QString & rom_filename, uint32_t rom_base_address, const QString & ram_filename, uint32_t ram_base_address, const QString register_filename);
	uint32_t readRegister(uint32_t register_number);
	uint32_t readWord(uint32_t address);
};

#endif // TARGETCOREFILE_HXX
