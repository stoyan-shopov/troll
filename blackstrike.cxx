#include <QDebug>
#include "blackstrike.hxx"

void Blackstrike::readAllRegisters()
{
QString s;
QRegExp rx("<<<start>>>(.*)<<<end>>>");
int pos;
	registers.clear();
	port->write("swdp-scan drop gdb-attach drop\n");
	port->write(".( <<<start>>>)cr ?regs .( <<<end>>>)cr\n");
	do
	{
		if (!port->waitForReadyRead(2000))
			Util::panic();
		s += port->readAll();
	}
	while (!s.contains("<<<end>>>"));
	qDebug() << s;
	s.replace('\n', "");
	if (rx.indexIn(s) == -1)
		Util::panic();
	qDebug() << "string recognized: " << rx.cap();
	s = rx.cap(1);
	rx.setPattern("\\s*(\\S+)");
	pos = 0;
	while ((pos = rx.indexIn(s, pos)) != -1)
	{
		bool ok;
		registers.push_back(rx.cap(1).toUInt(& ok, 16));
		if (!ok)
			Util::panic();
		qDebug() << "read value" << registers.back();
		pos += rx.matchedLength();
	}
}

uint32_t Blackstrike::readWord(uint32_t address)
{
QString s;
QRegExp rx("<<<start>>>(.*)<<<end>>>");
bool ok;
uint32_t x;
	registers.clear();
	port->write(QString("$%1 ").arg(address, 0, 16).toLocal8Bit());
	port->write("base @ >r hex .( <<<start>>>) t@ u. .( <<<end>>>) r> base ! cr\n");
	do
	{
		if (!port->waitForReadyRead(2000))
			Util::panic();
		s += port->readAll();
	}
	while (!s.contains("<<<end>>>"));
	qDebug() << s;
	s.replace('\n', "");
	if (rx.indexIn(s) == -1)
		Util::panic();
	qDebug() << "string recognized: " << rx.cap();
	s = rx.cap(1);
	rx.setPattern("\\s*(\\S+)");
	if (rx.indexIn(s) == -1)
		Util::panic();
	x = rx.cap(1).toUInt(& ok, 16);
	if (!ok)
		Util::panic();
	qDebug() << "read value" << x;
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
