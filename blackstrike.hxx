#ifndef BLACKSTRIKE_HXX
#define BLACKSTRIKE_HXX

#include <QSerialPort>
#include "target.hxx"
#include "util.hxx"

class Blackstrike : public Target
{
	Q_OBJECT
private:
	QSerialPort	* port;
private slots:
	void blackstrikeError(QSerialPort::SerialPortError error) { *(int*)0=0; }
public:
	Blackstrike(void){}
	Blackstrike(QSerialPort * port){}// { this->port = port; connect(port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(blackstrikeError(QSerialPort::SerialPortError))); }
	virtual uint32_t readWord(uint32_t address) { Util::panic(); }
	virtual uint32_t readRegister(uint32_t register_number) { Util::panic(); }
};

#endif // BLACKSTRIKE_HXX
