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
public:
	QByteArray interrogate(const QString & query, bool * isOk = 0);
	Blackstrike(QSerialPort * port) { this->port = port; }
	QByteArray readBytes(uint32_t address, int byte_count);
	virtual uint32_t readWord(uint32_t address);
	virtual uint32_t readRegister(uint32_t register_number);
	virtual uint32_t singleStep(void);
};

#endif // BLACKSTRIKE_HXX
