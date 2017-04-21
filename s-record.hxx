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
#ifndef SRECORDMEMORYDATA_H
#define SRECORDMEMORYDATA_H

#include <QFile>
#include <QFile>
#include <QDebug>
#include "memory.hxx"
#include "util.hxx"
#include <QRegExp>


class SRecordMemoryData
{
public:
	static bool loadFile(const QString & filename, Memory & memory)
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
				memory.addRange(rx.cap(3).toUInt(0, 16), QByteArray::fromHex(rx.cap(4).toLocal8Bit()));
			}
		}
		memory.dump();
		return true;
	}
};

#endif // SRECORDMEMORYDATA_H
