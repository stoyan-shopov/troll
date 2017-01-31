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
public:
	Disassembly(const QString & disassembly_filename)
	{
		int i = 0, x;
		bool ok;
		QFile f(disassembly_filename);
		QRegExp rx("^\\s+(\\w+):");
		if (!f.open(QFile::ReadOnly))
			Util::panic();
		disassembly_text = f.readAll();
		f.close();
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
	QString disassemblyAroundAddress(uint32_t address)
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
			return QString("<<< no disassembly for address $%1 >>>").arg(address, 8, 16, QChar('0'));
		if ((l = m - 5) < 0)
			l = 0;
		if ((h = m + 6) >= index_table.size())
			h = index_table.size() - 1;
		return disassembly_text.mid(index_table.at(l).second, disassembly_text.indexOf('\n', index_table.at(h).second) - index_table.at(l).second);
	}
};

#endif // DISASSEMBLY_HXX
