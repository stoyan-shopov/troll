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
#ifndef MEMORY_H
#define MEMORY_H
 
#include <stdint.h>
#include <QVector>
#include <QByteArray>
#include <QFile>
#include <QApplication>
#include "target.hxx"

#include "ui_notification.h"

struct memory_range
{
	uint32_t	address;
	QByteArray	data;
};

class Memory
{
public:
	QVector<struct memory_range> ranges;
	void addRange(uint32_t address, const QByteArray & data)
	{
		int i;
		for (i = 0; i < ranges.size(); i ++)
			if ((ranges[i].address <= address && address <= ranges[i].address + ranges[i].data.size())
				|| (address <= ranges[i].address && ranges[i].address <= address + data.size()))
			{
				/* regions overlap - coalesce */
				QByteArray x;
				if (address >= ranges[i].address)
				{
					x = ranges[i].data.left(address - ranges[i].address);
					x += data;
					address = ranges[i].address;
				}
				else
					x = data + ranges[i].data.right(ranges[i].data.size() - ((address + data.size()) - ranges[i].address));
				ranges[i] = (struct memory_range) { .address = address, .data = x, };
				auto s = ranges[i];
				ranges.removeAt(i);
				addRange(s.address, s.data);
				return;
			}
		ranges.push_back((struct memory_range) { .address = address, .data = data, });
	}
	bool isMemoryMatching(class Target * target) const
	{
		int i;
		QDialog dialog;
		Ui::Notification mbox;
		mbox.setupUi(& dialog);
		dialog.setWindowTitle("verifying target memory contents");
		for (i = 0; i < ranges.size(); i ++)
		{
			mbox.label->setText(QString("verifying target memory region\nstart address $%1, size $%2")
			                    .arg(ranges[i].address, 0, 16)
			                    .arg(ranges[i].data.size(), 0, 16));
			dialog.show();
			QApplication::processEvents();
			auto x = target->readBytes(ranges[i].address, ranges[i].data.size());
			qDebug() << x.size() << ranges[i].data.size();
			
			QFile f1("out1.bin");
			f1.open(QFile::WriteOnly);
			f1.write(x);
			f1.close();
			f1.setFileName("out2.bin");
			f1.open(QFile::WriteOnly);
			f1.write(ranges[i].data);
			f1.close();
			if (x != ranges[i].data)
				return false;
		}
		return true;
	}
	void dump(void) const
	{
		int i;
		for (i = 0; i < ranges.size(); i ++)
			qDebug() << "memory range at" << ranges[i].address << "size" << ranges[i].data.size();
	}
	QByteArray data(uint32_t address, int length)
	{
		int i;
		for (i = 0; i < ranges.size(); i ++)
			if (ranges[i].address <= address && address <= ranges[i].address + ranges[i].data.size())
				return ranges[i].data.mid(address - ranges[i].address, length);
		return QByteArray();
	}
};

#endif // MEMORY_H
