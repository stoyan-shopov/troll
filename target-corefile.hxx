/*
Copyright (c) 2016-2017 stoyan shopov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
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
	bool connect(void) { Util::panic(); }
	uint32_t haltReason(void) { Util::panic(); }
	virtual QByteArray memoryMap(void) { Util::panic(); }
	virtual bool syncFlash(const Memory & memory_contents) { Util::panic(); }
};

#endif // TARGETCOREFILE_HXX
