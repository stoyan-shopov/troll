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
	bool reset(void) { Util::panic(); }
	QByteArray readBytes(uint32_t address, int byte_count, bool is_failure_allowed = false);
	uint32_t readRawUncachedRegister(uint32_t register_number);
	uint32_t readWord(uint32_t address);
	uint32_t singleStep() { return 0; }
	bool breakpointSet(uint32_t address, int length) { Util::panic(); }
	bool breakpointClear(uint32_t address, int length) { Util::panic(); }
	void requestSingleStep() { Util::panic(); }
	bool resume(void) { Util::panic(); }
	bool requestHalt(void) { Util::panic(); }
	uint32_t haltReason(void) { Util::panic(); }
};

#endif // TARGETCOREFILE_HXX
