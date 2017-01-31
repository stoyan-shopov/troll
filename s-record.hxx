#ifndef SRECORDMEMORYDATA_H
#define SRECORDMEMORYDATA_H

#include <QFile>
#include <QFile>
#include <QDebug>
#include "memory.hxx"
#include "util.hxx"
#include <QRegExp>


class SRecordMemoryData : public Memory
{
public:
	bool loadFile(const QString & filename)
	{
		QFile f(filename);
		QRegExp rx("S(.)(..)(........)(.+)(\\w\\w)");
		if (!f.open(QFile::ReadOnly))
			Util::panic();
		while (!f.atEnd())
		{
			if (rx.indexIn(f.readLine()) != -1)
			{
				if (rx.cap(1) != "3")
					continue;
				addRange(rx.cap(3).toUInt(0, 16), QByteArray::fromHex(rx.cap(4).toLocal8Bit()));
			}
		}
		dump();
		return true;
	}
};

#endif // SRECORDMEMORYDATA_H
