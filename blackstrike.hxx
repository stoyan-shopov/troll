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
#ifndef BLACKSTRIKE_HXX
#define BLACKSTRIKE_HXX

#include <QSerialPort>
#include <vector>
#include "target.hxx"
#include "util.hxx"

class Blackstrike : public Target
{
	Q_OBJECT
	std::vector<uint32_t>	registers;
private:
	QSerialPort	* port;
	void readAllRegisters(void);
	QByteArray interrogate(const QByteArray &query, bool * isOk = 0);
private slots:
	void portReadyRead(void);
public:
	bool reset(void);
	Blackstrike(QSerialPort * port) { this->port = port; }
	QByteArray readBytes(uint32_t address, int byte_count, bool is_failure_allowed = false);
	uint32_t readWord(uint32_t address);
	uint32_t readRawUncachedRegister(uint32_t register_number);
	uint32_t singleStep(void);
	bool breakpointSet(uint32_t address, int length);
	bool breakpointClear(uint32_t address, int length);
	void requestSingleStep(void);
	bool resume(void);
	bool requestHalt(void);
	bool connect(void) { return (interrogate("\003 abort\n12 12 * .( <<<start>>>). .( <<<end>>>)cr").contains("144")) ? true : false; }
	uint32_t haltReason(void);
	QByteArray memoryMap(void);
	bool syncFlash(const Memory & memory_contents);
};

#endif // BLACKSTRIKE_HXX
