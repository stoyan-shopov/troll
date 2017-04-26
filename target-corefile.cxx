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
#include <QFile>
#include <QMessageBox>

#include "target.hxx"
#include "util.hxx"
#include "target-corefile.hxx"

TargetCorefile::TargetCorefile(const QString & rom_filename, uint32_t rom_base_address, const QString & ram_filename, uint32_t ram_base_address, const QString register_filename)
{
	this->rom_base_address = rom_base_address;
	this->ram_base_address = ram_base_address;
	QFile f;
	f.setFileName(rom_filename);
	if (f.open(QFile::ReadOnly))
		rom = f.readAll(), f.close();
	f.setFileName(ram_filename);
	if (f.open(QFile::ReadOnly))
		ram = f.readAll(), f.close();
	f.setFileName(register_filename);
	if (f.open(QFile::ReadOnly))
		register_file = f.readAll(), f.close();
}

QByteArray TargetCorefile::readBytes(uint32_t address, int byte_count, bool is_failure_allowed)
{
	if (rom_base_address <= address && address < rom_base_address + rom.length() - byte_count + 1)
		return QByteArray(rom.data() + address - rom_base_address, byte_count);
	if (ram_base_address <= address && address < ram_base_address + ram.length() - byte_count + 1)
		return QByteArray(ram.data() + address - ram_base_address, byte_count);
	if (is_failure_allowed)
	{
		qDebug() << QString("cannot read $%1 bytes at address $%2 from target memory").arg(byte_count).arg(address);
		if (0) QMessageBox::warning(0, "cannot read target memory", QString("cannot read $%1 bytes at address $%2 from target memory\n")
		                     .arg(byte_count)
		                     .arg(address)
		                     );
		return QByteArray();
	}
	else
	{
		QMessageBox::critical(0, "cannot read target memory", QString("cannot read $%1 bytes at address $%1 from target memory\n"
		                                                              "the troll will now abort"
		                                                              )
		                      .arg(byte_count)
		                      .arg(address)
		                      );
		Util::panic();
	}
}

uint32_t TargetCorefile::readWord(uint32_t address)
{
	if (rom_base_address <= address && address < rom_base_address + rom.length() - sizeof(uint32_t) + 1)
		return * (uint32_t *) (rom.data() + address - rom_base_address);
	if (ram_base_address <= address && address < ram_base_address + ram.length() - sizeof(uint32_t) + 1)
		return * (uint32_t *) (ram.data() + address - ram_base_address);
	throw MEMORY_READ_ERROR;
	Util::panic();
}

uint32_t TargetCorefile::readRawUncachedRegister(uint32_t register_number)
{
	if (register_number < register_file.length() / sizeof(uint32_t))
		return * (uint32_t *) (register_file.data() + register_number * sizeof(uint32_t));
	qDebug() << "warning: no registers - returning dummy value!!!";
	return -1;
}
