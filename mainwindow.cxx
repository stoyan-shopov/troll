#include "mainwindow.hxx"
#include "ui_mainwindow.h"

#include "libtroll.hxx"

#include <QByteArray>
#include <QFile>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	QFile debug_file("blackmagic");
	if (!debug_file.open(QFile::ReadOnly))
	{
		QMessageBox::critical(0, "error opening target executable", QString("error opening file ") + debug_file.fileName());
		exit(1);
	}
	debug_file.seek(debug_aranges_offset);
	QByteArray debug_aranges = debug_file.read(debug_aranges_len);
	debug_file.seek(debug_info_offset);
	QByteArray debug_info = debug_file.read(debug_info_len);
	debug_file.seek(debug_abbrev_offset);
	QByteArray debug_abbrev = debug_file.read(debug_abbrev_len);
	
	DwarfData dwdata(debug_aranges.data(), debug_aranges.length(), debug_info.data(), debug_info.length(), debug_abbrev.data(), debug_abbrev.length());
	ui->plainTextEdit->appendPlainText(QString("compilation unit count in the .debug_aranges section : %1").arg(dwdata.compilation_unit_count()));
	
	//auto x = dwdata.abbreviations_of_compilation_unit(0);
	//ui->plainTextEdit->appendPlainText(QString("number of abbreviations in the first compilation unit : %1").arg(x.size()));
}

MainWindow::~MainWindow()
{
	delete ui;
}
