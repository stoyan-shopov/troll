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
#ifndef DISASSEMBLY_HXX
#define DISASSEMBLY_HXX

#include <QFile>
#include <QVector>
#include <QPair>
#include <QRegExp>
#include <QDebug>
#include <stdint.h>
#include "util.hxx"

class Disassembly
{
private:
	QByteArray	disassembly_text;
	QVector<QPair<uint32_t /* address */, int /* index in disassembly text */> > index_table;
	int indexOfAddress(uint32_t address)
	{
		int l = 0, h = index_table.size() - 1, m;
		while (l <= h)
		{
			m = (l + h) >> 1;
			if (index_table.at(m).first == address)
				break;
			if (index_table.at(m).first < address)
				l = m + 1;
			else
				h = m - 1;
		}
		if (l > h)
			return -1;
		return m;
	}
public:
	Disassembly(QByteArray disassembly)
	{
		int i = 0, x;
		bool ok;
		QRegExp rx("^\\s*(\\w+):");

		disassembly_text = disassembly;
		while (i < disassembly_text.size())
		{
			QString s(disassembly_text.mid(i, (x = disassembly_text.indexOf('\n', i)) - i));
			if (rx.indexIn(s) != -1)
			{
				uint32_t address = rx.cap(1).toUInt(& ok, 16);
				if (ok)
					index_table << QPair<uint32_t, int>(address, i);
			}
			if (x == -1)
				break;
			i = x + 1;
		}
		for (i = 1; i < index_table.size(); i ++)
			if (index_table.at(i).first <= index_table.at(i - 1).first)
				Util::panic();
	}
	QString disassemblyAroundAddress(uint32_t address, int * line_for_address = 0)
	{
		int l, m = indexOfAddress(address), h;
		if (m == -1)
			return QString("<<< no disassembly for address $%1 >>>").arg(address, 8, 16, QChar('0'));
		if ((l = m - 5) < 0)
			l = 0;
		if ((h = m + 6) >= index_table.size())
			h = index_table.size() - 1;
		if (line_for_address)
		{
			* line_for_address = disassembly_text.mid(index_table.at(l).second, index_table.at(m).second - index_table.at(l).second).count('\n');
		}
		return disassembly_text.mid(index_table.at(l).second, disassembly_text.indexOf('\n', index_table.at(h).second) - index_table.at(l).second);
	}
	QList<QPair<uint32_t /* address */, QString /* disassembly */> > disassemblyForRange(uint32_t start, uint32_t end)
	{
		QList<QPair<uint32_t , QString > > dis;
		int i = indexOfAddress(start);
		if (i == -1)
			return dis << QPair<uint32_t, QString>(start, QString("<<< no disassembly for address $%1 >>>").arg(start, 8, 16, QChar('0')));
		while (start < end && i < index_table.size() - 1)
		{
			dis << QPair<uint32_t , QString >(start, disassembly_text.mid(index_table.at(i).second, disassembly_text.indexOf('\n', index_table.at(i).second) - index_table.at(i).second ));
			start = index_table.at(++ i).first;
		}
		return dis;
	}
};

#endif // DISASSEMBLY_HXX
