/*
Copyright (c) 2017 stoyan shopov

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
#ifndef BLACKMAGIC_HXX
#define BLACKMAGIC_HXX

#include <QSerialPort>

#include "target.hxx"

class Blackmagic : public Target
{
private:
	QSerialPort	* port;
	void putPacket(const QByteArray & request);
	QByteArray getPacket(void);
	char getChar(void);
public:
	Blackmagic(QSerialPort * port) { this->port = port; }
	uint32_t readWord(uint32_t address) { Util::panic(); }
	bool reset(void){ Util::panic(); }
	QByteArray readBytes(uint32_t address, int byte_count, bool is_failure_allowed = false){ Util::panic(); }
	uint32_t readRawUncachedRegister(uint32_t register_number){ Util::panic(); }
	uint32_t singleStep(void){ Util::panic(); }
	bool breakpointSet(uint32_t address, int length){ Util::panic(); }
	bool breakpointClear(uint32_t address, int length){ Util::panic(); }
	void requestSingleStep(void){ Util::panic(); }
	bool resume(void){ Util::panic(); }
	bool requestHalt(void){ Util::panic(); }
	bool connect(void);
	uint32_t haltReason(void){ Util::panic(); }
	QByteArray memoryMap(void){ Util::panic(); }
	bool syncFlash(const Memory & memory_contents){ Util::panic(); }
};

#endif // BLACKMAGIC_HXX
