#ifndef TARGETCOREFILE_HXX
#define TARGETCOREFILE_HXX

#include <QString>
#include "target.hxx"
#include "util.hxx"

class TargetCorefile : public Target
{
private:
	uint32_t rom_base_address, ram_base_address;
	QByteArray rom, ram, register_file;
public:
	TargetCorefile(const QString & rom_filename, uint32_t rom_base_address, const QString & ram_filename, uint32_t ram_base_address, const QString register_filename);
	virtual QByteArray interrogate(const QByteArray & query, bool * isOk = 0) { Util::panic(); }
	QByteArray readBytes(uint32_t address, int byte_count);
	uint32_t readRegister(uint32_t register_number);
	uint32_t readWord(uint32_t address);
	uint32_t singleStep() { return 0; }
};

#endif // TARGETCOREFILE_HXX
