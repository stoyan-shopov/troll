#include "mainwindow.hxx"
#include "ui_mainwindow.h"
#include <QByteArray>
#include <QFile>
#include <QMessageBox>
#include <QTime>

void MainWindow::dump_debug_tree(std::vector<struct Die> & dies, int level)
{
int i;
	for (i = 0; i < dies.size(); i++)
	{
		ui->plainTextEdit->appendPlainText(QString(level, QChar('\t')) + QString("tag %1, @offset $%2").arg(dies.at(i).tag).arg(dies.at(i).offset, 0, 16));
		if (!dies.at(i).children.empty())
			dump_debug_tree(dies.at(i).children, level + 1);
	}
}

QTreeWidgetItem * MainWindow::itemForNode(const DwarfData::DataNode &node)
{
auto n = new QTreeWidgetItem(QStringList() << QString::fromStdString(node.data.at(0)));
int i;
	for (i = 0; i < node.children.size(); n->addChild(itemForNode(node.children.at(i ++ ))));
	return n;
}

void MainWindow::backtrace()
{
	cortexm0->primeUnwinder();
	uint32_t last_pc;
	auto context = dwdata->executionContextForAddress(last_pc = cortexm0->programCounter());
	
	qDebug() << "backtrace:";
	ui->listWidget->clear();
	while (context.size())
	{
		auto unwind_data = dwundwind->sforthCodeForAddress(cortexm0->programCounter());
		auto x = dwdata->sourceCodeCoordinatesForAddress(cortexm0->programCounter(), context.at(0));
		qDebug() << QString::fromStdString(x.first) << (signed) x.second;

		qDebug() << cortexm0->programCounter() << QString::fromStdString(dwdata->nameOfDie(context.back()));
		ui->listWidget->addItem(QString("$%1").arg(cortexm0->programCounter(), 8, 16, QChar('0')) + QString("\t") + QString::fromStdString(dwdata->nameOfDie(context.back()))
				+ QString("\t%1").arg(x.second));
		
		if (cortexm0->unwindFrame(QString::fromStdString(unwind_data.first), unwind_data.second, cortexm0->programCounter()))
			context = dwdata->executionContextForAddress(cortexm0->programCounter());
		if (context.empty() && cortexm0->architecturalUnwind())
		{
			context = dwdata->executionContextForAddress(cortexm0->programCounter());
			if (!context.empty())
			{
				qDebug() << "architecture-specific unwinding performed";
				ui->listWidget->addItem("\tarchitecture-specific unwinding performed");
			}
		}
		if (last_pc == cortexm0->programCounter())
			break;
		last_pc = cortexm0->programCounter();
	}
	qDebug() << "registers: " << cortexm0->unwoundRegisters();
}

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
#if MAIN_APS
	QFile debug_file("main_aps.elf");
#else
	QFile debug_file("Qt5Guid.dll");
#endif
	QTime t;
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
	debug_file.seek(debug_frame_offset);
	QByteArray debug_frame = debug_file.read(debug_frame_len);
	debug_file.seek(debug_ranges_offset);
	QByteArray debug_ranges = debug_file.read(debug_ranges_len);
	debug_file.seek(debug_str_offset);
	QByteArray debug_str = debug_file.read(debug_str_len);
	debug_file.seek(debug_line_offset);
	QByteArray debug_line = debug_file.read(debug_line_len);
	debug_file.seek(debug_loc_offset);
	QByteArray debug_loc = debug_file.read(debug_loc_len);
	
	dwdata = new DwarfData(debug_aranges.data(), debug_aranges.length(), debug_info.data(), debug_info.length(), debug_abbrev.data(), debug_abbrev.length(), debug_ranges.data(), debug_ranges.length(), debug_str.data(), debug_str.length(), debug_line.data(), debug_line.length(), debug_loc.data(), debug_loc.length());
	ui->plainTextEdit->appendPlainText(QString("compilation unit count in the .debug_aranges section : %1").arg(dwdata->compilation_unit_count()));
	
#if MAIN_APS
	
	std::vector<struct DwarfTypeNode> type_cache;
	std::map<uint32_t, uint32_t> x;
	dwdata->get_abbreviations_of_compilation_unit(11, x);
	dwdata->readType(0x98, x, type_cache);
	qDebug() << __FILE__ << __LINE__ << type_cache.size() << type_cache.at(0).die.children.size();
	qDebug() << QString::fromStdString(dwdata->typeString(type_cache));
	
	struct DwarfData::DataNode node;
	dwdata->dataForType(type_cache, node);
	qDebug() << node.children.size();
	ui->treeWidget->addTopLevelItem(itemForNode(node));
#endif
	
	//*/
	
	int i;
	uint32_t cu;
	t.start();
	uint32_t die_offset;
	for (i = cu = 0; cu != -1; i++, cu = dwdata->next_compilation_unit(cu))
	{
		die_offset = cu + 11;
		std::map<uint32_t, uint32_t> abbreviations;
		dwdata->get_abbreviations_of_compilation_unit(cu, abbreviations);
		dwdata->debug_tree_of_die(die_offset, abbreviations);
	}
	qDebug() << "all compilation units in .debug_info processed in" << t.elapsed() << "milliseconds";
	qDebug() << "decoding of .debug_info ended at" << die_offset;

	ui->plainTextEdit->appendPlainText(QString("compilation unit count in the .debug_info section : %1").arg(i));
	
	dwundwind = new DwarfUnwinder(debug_frame.data(), debug_frame.length());
	while (!dwundwind->at_end())
		dwundwind->dump(), dwundwind->next();
	
	auto unwind_data = dwundwind->sforthCodeForAddress(0x800f226);
	auto context = dwdata->executionContextForAddress(0x800f226);
	if (context.size())
	{
		qDebug() << context.size();
		qDebug() << context.at(0).offset << context.at(1).offset;
		qDebug() << QString::fromStdString(dwdata->nameOfDie(context.at(1)));
		qDebug() << QString::fromStdString(unwind_data.first);

		target = new Target("flash.bin", 0x08000000, "ram.bin", 0x20000000, "registers.bin");
		sforth = new Sforth(ui->plainTextEditSforthConsole);
		cortexm0 = new CortexM0(sforth, target);
		cortexm0->primeUnwinder();
		cortexm0->unwindFrame(QString::fromStdString(unwind_data.first), unwind_data.second, 0x800f226);
		auto regs = cortexm0->unwoundRegisters();
		qDebug() << regs;

		qDebug() << "next frame";
		unwind_data = dwundwind->sforthCodeForAddress(regs.at(15));
		qDebug() << QString::fromStdString(unwind_data.first);
		cortexm0->unwindFrame(QString::fromStdString(unwind_data.first), unwind_data.second, regs.at(15));
		regs = cortexm0->unwoundRegisters();
		qDebug() << regs;

		backtrace();
	}
	
	t.restart();
	dwdata->dumpLines();
	qDebug() << ".debug_lines section processed in" << t.elapsed() << "milliseconds";
	t.restart();
	std::vector<struct StaticObject> data_objects, subprograms;
	dwdata->reapStaticObjects(data_objects, subprograms);
	qDebug() << "static storage duration data reaped in" << t.elapsed() << "milliseconds";
	qDebug() << "data objects:" << data_objects.size() << ", subprograms:" << subprograms.size();
	t.restart();
	for (i = 0; i < data_objects.size(); i++)
		ui->listWidgetDataObjects->addItem(QString(data_objects.at(i).name));
	ui->listWidgetDataObjects->sortItems();
	qDebug() << "static object lists built in" << t.elapsed() << "milliseconds";
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_lineEditSforthCommand_returnPressed()
{
	sforth->evaluate(ui->lineEditSforthCommand->text() + '\n');
	ui->lineEditSforthCommand->clear();
}
