#ifndef SFORTH_H
#define SFORTH_H

#include <QPlainTextEdit>

extern "C"
{
#include "engine.h"
}

class Sforth
{
private:
public:
	Sforth(QPlainTextEdit * console);
	void evaluate(const QString & sforth_commands);
};

#endif // SFORTH_H
