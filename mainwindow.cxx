#include "mainwindow.hxx"
#include "ui_mainwindow.h"
#include <QByteArray>
#include <QFile>
#include <QMessageBox>

void MainWindow::dump_debug_tree(std::vector<struct Die> & dies, int level)
{
int i;
	for (i = 0; i < dies.size(); i++)
	{
		ui->plainTextEdit->appendPlainText(QString(level, QChar('\t')) + QString("tag %1, @offset $%2").arg(dwdata->tag(dies.at(i))).arg(dies.at(i).offset, 0, 16));
		if (!dies.at(i).children.empty())
			dump_debug_tree(dies.at(i).children, level + 1);
	}
}

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
	
	dwdata = new DwarfData(debug_aranges.data(), debug_aranges.length(), debug_info.data(), debug_info.length(), debug_abbrev.data(), debug_abbrev.length());
	ui->plainTextEdit->appendPlainText(QString("compilation unit count in the .debug_aranges section : %1").arg(dwdata->compilation_unit_count()));
	
	auto x = dwdata->abbreviations_of_compilation_unit(0);
	ui->plainTextEdit->appendPlainText(QString("number of abbreviations in the first compilation unit : %1").arg(x.size()));
	
	int i;
	uint32_t cu;
	for (i = cu = 0; cu != -1; i++, cu = dwdata->next_compilation_unit(cu));
	ui->plainTextEdit->appendPlainText(QString("compilation unit count in the .debug_info section : %1").arg(i));
	
	uint32_t die_offset = 11;
	auto debug_tree = dwdata->debug_tree_of_die(die_offset, x);
	ui->plainTextEdit->appendPlainText(QString("debug tree decoding ended at offset: %1").arg(die_offset));
	dump_debug_tree(debug_tree, 1);
}

MainWindow::~MainWindow()
{
	delete ui;
}
