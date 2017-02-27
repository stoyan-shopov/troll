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
	void portReadyRead(void);
public:
	virtual QByteArray interrogate(const QByteArray &query, bool * isOk = 0);
	bool reset(void);
	Blackstrike(QSerialPort * port) { this->port = port; }
	QByteArray readBytes(uint32_t address, int byte_count, bool is_failure_allowed = false);
	uint32_t readWord(uint32_t address);
	uint32_t readRawUncachedRegister(uint32_t register_number);
	uint32_t singleStep(void);
	void requestSingleStep(void);
	bool resume(void);
	bool requestHalt(void);
	uint32_t haltReason(void);
};

#endif // BLACKSTRIKE_HXX
