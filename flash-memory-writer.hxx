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
#ifndef FLASHMEMORYWRITER_HXX
#define FLASHMEMORYWRITER_HXX

#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QLabel>
#include <QTime>
#include <QProgressDialog>

#include "util.hxx"
#include "target.hxx"
#include "memory.hxx"

#include "ui_notification.h"

class FlashMemoryWriter
{
public:
	static bool syncFlash(Target * target, const Memory & memory_contents)
	{
		QTime t;
		int i;
		QString s;
		uint32_t total;
		if (memory_contents.isMemoryMatching(target))
			return true;
		auto ranges = target->flashAreasForRange(memory_contents.ranges[0].address, memory_contents.ranges[0].data.size());
		if (memory_contents.ranges.size() != 1 || ranges.empty())
			Util::panic();
		QDialog dialog;
		Ui::Notification mbox;
		mbox.setupUi(& dialog);
		dialog.setWindowTitle("erasing flash");
		t.start();
		for (total = i = 0; i < ranges.size(); i ++)
		{
			mbox.label->setText(QString("erasing flash at start address $%1, size $%2").arg(ranges[i].first, 0, 16).arg(ranges[i].second, 0, 16));
			dialog.show();
			QApplication::processEvents();

			s = target->interrogate(QString("$%1 $%2 .( <<<start>>>)flash-erase .( <<<end>>>)").arg(ranges[i].first, 0, 16).arg(ranges[i].second, 0, 16));
			if (!s.contains("erased successfully"))
			{
				QMessageBox::critical(0, "error erasing flash", QString("error erasing $%1 bytes of flash at address $%2").arg(ranges[i].second, 0, 16).arg(ranges[i].first, 0, 16));
				Util::panic();
			}
			total += ranges[i].second;
		}
		qDebug() << "flash erase speed" << QString("%1 bytes per second").arg((float) (total * 1000.) / t.elapsed());
		dialog.setWindowTitle("writing flash");
		mbox.label->setText(QString("writing $%1 bytes to flash at start address $%2")
			.arg(memory_contents.ranges[0].data.size(), 0, 16)
			.arg(memory_contents.ranges[0].address, 0, 16));
		QApplication::processEvents();

		t.restart();
		s = target->interrogate(QString("$%1 $%2 .( <<<start>>>)flash-write\n")
				.arg(memory_contents.ranges[0].address, 0, 16)
		                .arg(memory_contents.ranges[0].data.size(), 0, 16).toLocal8Bit()
		                + memory_contents.ranges[0].data + " .( <<<end>>>)");
		if (!s.contains("written successfully"))
		{
			QMessageBox::critical(0, "error writing flash", QString("error writing $%1 bytes of flash at address $%2").arg(memory_contents.ranges[0].data.size(), 0, 16).arg(memory_contents.ranges[0].address, 0, 16));
			Util::panic();
		}
		qDebug() << "flash write speed" << QString("%1 bytes per second").arg((float) (memory_contents.ranges[0].data.size() * 1000.) / t.elapsed());
		return memory_contents.isMemoryMatching(target);
	}
};

#endif // FLASHMEMORYWRITER_HXX
