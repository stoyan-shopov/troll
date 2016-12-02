#ifndef TARGET_H
#define TARGET_H

#include <stdint.h>
#include <QObject>

class Target : public QObject
{
	Q_OBJECT
public:
	virtual uint32_t readWord(uint32_t address) = 0;
	virtual QByteArray readBytes(uint32_t address, int byte_count) = 0;
	virtual uint32_t readRegister(uint32_t register_number) = 0;
	virtual uint32_t singleStep(void) = 0;
};

#endif // TARGET_H
