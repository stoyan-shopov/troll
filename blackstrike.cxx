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
#include <QDebug>
#include <QTime>
#include <QMessageBox>
#include "blackstrike.hxx"
#include "memory.hxx"

#define BLACKSTIRKE_DEBUG	0

void Blackstrike::readAllRegisters()
{
QString s;
QRegExp rx("<<<start>>>(.*)<<<end>>>");
int pos;
QTime t;

	t.start();
	registers.clear();
	if (port->write("swdp-scan drop gdb-attach drop\n") == -1) Util::panic();
	if (port->write(".( <<<start>>>)cr ?regs .( <<<end>>>)cr\n") == -1) Util::panic();
	do
	{
		if (port->bytesAvailable())
			s += port->readAll();
		else if (!port->waitForReadyRead(2000))
			Util::panic();
	}
	while (!s.contains("<<<end>>>"));
	if (BLACKSTIRKE_DEBUG) qDebug() << s;
	s.replace('\n', "");
	if (rx.indexIn(s) == -1)
		Util::panic();
	if (BLACKSTIRKE_DEBUG) qDebug() << "string recognized: " << rx.cap();
	s = rx.cap(1);
	rx.setPattern("\\s*(\\S+)");
	pos = 0;
	while ((pos = rx.indexIn(s, pos)) != -1)
	{
		bool ok;
		registers.push_back(rx.cap(1).toUInt(& ok, 16));
		if (!ok)
			Util::panic();
		if (BLACKSTIRKE_DEBUG) qDebug() << "read value" << registers.back();
		pos += rx.matchedLength();
	}
	qDebug() << "target register read took" << t.elapsed() << "milliseconds";
}

void Blackstrike::portReadyRead()
{
QString halt_reason = port->readAll();
	if (!disconnect(port, SIGNAL(readyRead()), 0, 0))
		Util::panic();
	if (halt_reason.contains("target-halted-breakpoint"))
		emit targetHalted(BREAKPOINT_HIT);
	else if (halt_reason.contains("target-halted"))
		emit targetHalted(GENERIC_HALT_CONDITION);
	else
		Util::panic();
}

QByteArray Blackstrike::interrogate(const QByteArray & query, bool *isOk)
{
	if (query.indexOf(".( <<<start>>>)") >= query.indexOf(".( <<<end>>>)"))
		Util::panic();

QByteArray s;
QRegExp rx("<<<start>>>(.*)<<<end>>>");
int l, r;
bool ok;
QTime t;

	t.start();
	if (isOk)
		* isOk = true;
	if (port->write(query + '\n') == -1) Util::panic();
	do
	{
		if (port->bytesAvailable())
			s += port->readAll();
		else if (!port->waitForReadyRead(6000))
		{
			if (isOk)
				* isOk = false;
			return "query timed out";
		}
	}
	while (!s.contains("<<<end>>>"));
	if (BLACKSTIRKE_DEBUG) qDebug() << s;
	//s.replace('\n', "");
	l = s.indexOf("<<<start>>>");
	r = s.indexOf("<<<end>>>");
	if (l >= r)
		Util::panic();
	s = s.mid(l + sizeof "<<<start>>>" - /* ignore null terminator */ 1, r - l - sizeof "<<<start>>>" + /* ignore null terminator */ 1);
	if (BLACKSTIRKE_DEBUG) qDebug() << "string recognized: " << s;
	if (BLACKSTIRKE_DEBUG) qDebug() << "target query took" << t.elapsed() << "milliseconds";
	return s;
}

bool Blackstrike::reset(void)
{
	registers.clear();
	if (!interrogate(QString("target-reset .( <<<start>>>).( <<<end>>>)").toLocal8Bit()).isEmpty())
		Util::panic();
	return true;
}

QByteArray Blackstrike::readBytes(uint32_t address, int byte_count, bool is_failure_allowed)
{
QTime t;
QString s(
" $%1 $%2 "
" .( <<<start>>>) target-dump .( <<<end>>>) cr "
);
	t.start();
	auto x = interrogate(s.arg(address, 0, 16).arg(byte_count, 0, 16).toLocal8Bit());
	qDebug() << "usb xfer speed:" << ((float) x.length() / t.elapsed()) * 1000. << "bytes/second";
	return x;
}

uint32_t Blackstrike::readWord(uint32_t address)
{
QString s;
QRegExp rx("\\s*(\\S+)");
bool ok;
uint32_t x;

	s = interrogate(QString("$%1 base @ >r hex .( <<<start>>>) t@ u. .( <<<end>>>) r> base ! cr").arg(address, 0, 16).toLocal8Bit(), & ok);
	if (!ok)
		Util::panic();

	if (rx.indexIn(s) == -1)
		Util::panic();
	x = rx.cap(1).toUInt(& ok, 16);
	if (!ok)
		Util::panic();
	if (BLACKSTIRKE_DEBUG) qDebug() << "read value" << x;
	return x;
}

uint32_t Blackstrike::readRawUncachedRegister(uint32_t register_number)
{
	if (register_number >= registers.size())
		readAllRegisters();
	if (register_number >= registers.size())
		Util::panic();
	return registers.at(register_number);
}

bool Blackstrike::breakpointSet(uint32_t address, int length)
{
	return interrogate(QString("#%1 #%2 .( <<<start>>>) breakpoint-set .( <<<end>>>)").arg(address).arg(length).toUtf8()).contains("breakpoint-ok")
			? true : false;
}

bool Blackstrike::breakpointClear(uint32_t address, int length)
{
	return interrogate(QString("#%1 #%2 .( <<<start>>>) breakpoint-clear .( <<<end>>>)").arg(address).arg(length).toUtf8()).contains("breakpoint-ok")
			? true : false;
}

void Blackstrike::requestSingleStep()
{
	emit targetRunning();
	QObject::connect(port, SIGNAL(readyRead()), this, SLOT(portReadyRead()));
	registers.clear();
	if (port->write("step\n") == -1)
		Util::panic();
}

bool Blackstrike::resume()
{
	emit targetRunning();
	QObject::connect(port, SIGNAL(readyRead()), this, SLOT(portReadyRead()));
	registers.clear();
	if (port->write("target-resume\n") == -1)
		Util::panic();
	return true;
}

bool Blackstrike::requestHalt()
{
	if (port->write("\003") == -1)
		Util::panic();
	return true;
}

uint32_t Blackstrike::haltReason()
{
bool ok;
QString s;
unsigned halt_reason; 
	halt_reason = (s = interrogate(QString("?target-run-state .( <<<start>>>). .( <<<end>>>)").toLocal8Bit())).toUInt(& ok);
	if (!ok)
	{
		QMessageBox::critical(0, "cannot read target state", "cannot read target state, response is: " + s);
		Util::panic();
	}
	QMessageBox::information(0, "target halt reason", QString("target halt reason: %1").arg(halt_reason));
	return halt_reason;
}

QByteArray Blackstrike::memoryMap()
{
	auto s = interrogate(QByteArray(".( <<<start>>>)?target-mem-map .( <<<end>>>)"));
	if (!s.contains("memory-map"))
	{
		QMessageBox::critical(0, "error reading target memory map", QString("error reading target memory map"));
		Util::panic();
	}
	qDebug() << s;
	return s;
}

bool Blackstrike::syncFlash(const Memory &memory_contents)
{
	QTime t;
	int i;
	QString s;
	uint32_t total;
	if (memory_contents.isMemoryMatching(this))
		return true;
	auto ranges = flashAreasForRange(memory_contents.ranges[0].address, memory_contents.ranges[0].data.size());
	if (memory_contents.ranges.size() != 1 || ranges.empty())
		Util::panic();
	QDialog dialog;
	Ui::Notification mbox;
	mbox.setupUi(& dialog);
	dialog.setWindowTitle("erasing flash");
	t.start();
	for (total = i = 0; i < ranges.size(); i ++)
	{
		mbox.label->setText(QString("erasing flash at start address $%1, size $%2").arg(ranges[i].first, 0, 16).arg(ranges[i].second, 0, 16));
		dialog.show();
		QApplication::processEvents();

		s = interrogate(QString("$%1 $%2 .( <<<start>>>)flash-erase .( <<<end>>>)").arg(ranges[i].first, 0, 16).arg(ranges[i].second, 0, 16).toLocal8Bit());
		if (!s.contains("erased successfully"))
		{
			QMessageBox::critical(0, "error erasing flash", QString("error erasing $%1 bytes of flash at address $%2").arg(ranges[i].second, 0, 16).arg(ranges[i].first, 0, 16));
			Util::panic();
		}
		total += ranges[i].second;
	}
	qDebug() << "flash erase speed" << QString("%1 bytes per second").arg((float) (total * 1000.) / t.elapsed());
	dialog.setWindowTitle("writing flash");
	mbox.label->setText(QString("writing $%1 bytes to flash at start address $%2")
	                    .arg(memory_contents.ranges[0].data.size(), 0, 16)
	                .arg(memory_contents.ranges[0].address, 0, 16));
	QApplication::processEvents();

	t.restart();
	s = interrogate(QString("$%1 $%2 .( <<<start>>>)flash-write\n")
	                        .arg(memory_contents.ranges[0].address, 0, 16)
	                .arg(memory_contents.ranges[0].data.size(), 0, 16).toLocal8Bit()
	                + memory_contents.ranges[0].data + " .( <<<end>>>)");
	if (!s.contains("written successfully"))
	{
		QMessageBox::critical(0, "error writing flash", QString("error writing $%1 bytes of flash at address $%2").arg(memory_contents.ranges[0].data.size(), 0, 16).arg(memory_contents.ranges[0].address, 0, 16));
		Util::panic();
	}
	qDebug() << "flash write speed" << QString("%1 bytes per second").arg((float) (memory_contents.ranges[0].data.size() * 1000.) / t.elapsed());
	return memory_contents.isMemoryMatching(this);
}
