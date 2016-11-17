#include <QDebug>
#include <QTime>
#include "blackstrike.hxx"

#define BLACKSTIRKE_DEBUG	0

void Blackstrike::readAllRegisters()
{
QString s;
QRegExp rx("<<<start>>>(.*)<<<end>>>");
int pos;
QTime t;

	t.start();
	registers.clear();
	port->write("swdp-scan drop gdb-attach drop\n");
	port->write(".( <<<start>>>)cr ?regs .( <<<end>>>)cr\n");
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

uint32_t Blackstrike::readWord(uint32_t address)
{
QString s;
QRegExp rx("<<<start>>>(.*)<<<end>>>");
bool ok;
uint32_t x;
QTime t;

	t.start();
	port->write(QString("$%1 ").arg(address, 0, 16).toLocal8Bit());
	port->write("base @ >r hex .( <<<start>>>) t@ u. .( <<<end>>>) r> base ! cr\n");
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
	if (rx.indexIn(s) == -1)
		Util::panic();
	x = rx.cap(1).toUInt(& ok, 16);
	if (!ok)
		Util::panic();
	if (BLACKSTIRKE_DEBUG) qDebug() << "read value" << x;
	qDebug() << "target word read took" << t.elapsed() << "milliseconds";
	return x;
}

uint32_t Blackstrike::readRegister(uint32_t register_number)
{
	if (register_number >= registers.size())
		readAllRegisters();
	if (register_number >= registers.size())
		Util::panic();
	return registers.at(register_number);
}

uint32_t Blackstrike::singleStep(void)
{
QString s;
QRegExp rx("<<<start>>>(.*)<<<end>>>");
bool ok;
uint32_t x;
QTime t;

	t.start();

	registers.clear();
	port->write("base @ >r hex .( <<<start>>>) step u. .( <<<end>>>) r> base ! cr\n");
	do
	{
		if (port->bytesAvailable())
			s += port->readAll();
		else if (!port->waitForReadyRead(2000))
			Util::panic();
	}
	while (!s.contains("<<<end>>>"));

	s.replace('\n', "");
	if (rx.indexIn(s) == -1)
		Util::panic();
	if (BLACKSTIRKE_DEBUG) qDebug() << "string recognized: " << rx.cap();
	s = rx.cap(1);
	rx.setPattern("\\s*(\\S+)");
	if (rx.indexIn(s) == -1)
		Util::panic();
	x = rx.cap(1).toUInt(& ok, 16);
	if (!ok)
		Util::panic();
	if (BLACKSTIRKE_DEBUG) qDebug() << "target halt reason: " << x;
	qDebug() << "target single-stepping took" << t.elapsed() << "milliseconds";
	return x;
}
