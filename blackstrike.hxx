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
private slots:
	void blackstrikeError(QSerialPort::SerialPortError error) { if (error != QSerialPort::NoError) *(int*)0=0; }
public:
	Blackstrike(QSerialPort * port) { this->port = port; connect(port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(blackstrikeError(QSerialPort::SerialPortError))); }
	virtual uint32_t readWord(uint32_t address);
	virtual uint32_t readRegister(uint32_t register_number);
	virtual uint32_t singleStep(void);
};

#endif // BLACKSTRIKE_HXX
