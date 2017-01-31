#ifndef FLASHMEMORYWRITER_HXX
#define FLASHMEMORYWRITER_HXX

#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QLabel>
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
		if (memory_contents.ranges.size() != 1 || !target->isAreaInFlash(memory_contents.ranges[0].address, memory_contents.ranges[0].data.size()))
			Util::panic();
		auto x = target->flashSectorAddressesForRange(memory_contents.ranges[0].address, memory_contents.ranges[0].data.size());
		QDialog dialog;
		Ui::Notification mbox;
		mbox.setupUi(& dialog);
		dialog.setWindowTitle("erasing flash");
		mbox.label->setText(QString("erasing flash at start address $%1, size $%2").arg(x.first, 0, 16).arg(x.second, 0, 16));
		dialog.show();
		QApplication::processEvents();
		auto s = target->interrogate(QString("$%1 $%2 .( <<<start>>>)flash-erase .( <<<end>>>)").arg(x.first, 0, 16).arg(x.second, 0, 16));
		qDebug() << s;
		mbox.label->setText(QString("writing $%1 bytes to flash at start address $%2")
			.arg(memory_contents.ranges[0].data.size(), 0, 16)
			.arg(memory_contents.ranges[0].address, 0, 16));
		QApplication::processEvents();

		s = target->interrogate(QString("$%1 $%2 .( <<<start>>>)\nflash-write\n")
				.arg(memory_contents.ranges[0].address, 0, 16)
		                .arg(memory_contents.ranges[0].data.size(), 0, 16).toLocal8Bit()
		                + memory_contents.ranges[0].data + " .( <<<end>>>)");
		qDebug() << s;
		return s.contains("success") ? true : false;
	}
};

#endif // FLASHMEMORYWRITER_HXX
