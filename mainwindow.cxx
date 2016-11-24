#include "mainwindow.hxx"
#include "ui_mainwindow.h"
#include <QByteArray>
#include <QFile>
#include <QMessageBox>
#include <QTime>
#include <QProcess>
#include <QRegExp>
#include <QMap>
#include <QSerialPortInfo>
#include <QSettings>


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
	register_cache->clear();
	register_cache->pushFrame(cortexm0->getRegisters());
	uint32_t last_pc;
	auto context = dwdata->executionContextForAddress(last_pc = cortexm0->programCounter());
	int row;
	
	qDebug() << "backtrace:";
	ui->tableWidgetBacktrace->blockSignals(true);
	ui->tableWidgetBacktrace->setRowCount(0);
	ui->tableWidgetBacktrace->blockSignals(false);
	while (context.size())
	{
		auto unwind_data = dwundwind->sforthCodeForAddress(cortexm0->programCounter());
		auto x = dwdata->sourceCodeCoordinatesForAddress(cortexm0->programCounter(), context.at(0));
		qDebug() << x.filename << (signed) x.line;

		qDebug() << cortexm0->programCounter() << QString::fromStdString(dwdata->nameOfDie(context.back()));
		ui->tableWidgetBacktrace->insertRow(row = ui->tableWidgetBacktrace->rowCount());
		ui->tableWidgetBacktrace->setItem(row, 0, new QTableWidgetItem(QString("$%1").arg(cortexm0->programCounter(), 8, 16, QChar('0'))));
		ui->tableWidgetBacktrace->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(dwdata->nameOfDie(context.back()))));
		ui->tableWidgetBacktrace->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(x.filename)));
		ui->tableWidgetBacktrace->setItem(row, 3, new QTableWidgetItem(QString("%1").arg(x.line)));
		ui->tableWidgetBacktrace->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(x.directoryname)));
		ui->tableWidgetBacktrace->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(dwdata->sforthCodeFrameBaseForContext(context))));
		
		if (cortexm0->unwindFrame(QString::fromStdString(unwind_data.first), unwind_data.second, cortexm0->programCounter()))
			context = dwdata->executionContextForAddress(cortexm0->programCounter()), register_cache->pushFrame(cortexm0->getRegisters());
		if (context.empty() && cortexm0->architecturalUnwind())
		{
			register_cache->pushFrame(cortexm0->getRegisters());
			context = dwdata->executionContextForAddress(cortexm0->programCounter());
			if (!context.empty())
			{
				qDebug() << "architecture-specific unwinding performed";
				ui->tableWidgetBacktrace->insertRow(row = ui->tableWidgetBacktrace->rowCount());
				ui->tableWidgetBacktrace->setItem(row, 1, new QTableWidgetItem("\tarchitecture-specific unwinding performed"));
			}
		}
		if (last_pc == cortexm0->programCounter())
			break;
		last_pc = cortexm0->programCounter();
	}
	qDebug() << "registers: " << cortexm0->getRegisters();
	ui->tableWidgetBacktrace->selectRow(0);
}

bool MainWindow::readElfSections(void)
{
QRegExp rx("\\.debug_info\\s+\\w+\\s+\\w+\\s+(\\w+)\\s+(\\w+)");
QProcess readelf;
QString output;
bool ok1, ok2;

	readelf.start("readelf", QStringList() << "-S" << elf_filename);
	readelf.waitForFinished();
	qDebug() << (output = readelf.readAll());
	if (rx.indexIn(output) != -1)
	{
		qDebug() << ".debug_info at" << rx.cap(1) << "size" << rx.cap(2);
		debug_info_offset = rx.cap(1).toInt(&ok1, 16);
		debug_info_len = rx.cap(2).toInt(&ok2, 16);
		if (!(ok1 && ok2)) Util::panic();
	}
	rx.setPattern("\\.debug_abbrev\\s+\\w+\\s+\\w+\\s+(\\w+)\\s+(\\w+)");
	if (rx.indexIn(output) != -1)
	{
		qDebug() << ".debug_abbrev at" << rx.cap(1) << "size" << rx.cap(2);
		debug_abbrev_offset = rx.cap(1).toInt(&ok1, 16);
		debug_abbrev_len = rx.cap(2).toInt(&ok2, 16);
		if (!(ok1 && ok2)) Util::panic();
	}
	rx.setPattern("\\.debug_aranges\\s+\\w+\\s+\\w+\\s+(\\w+)\\s+(\\w+)");
	if (rx.indexIn(output) != -1)
	{
		qDebug() << ".debug_aranges at" << rx.cap(1) << "size" << rx.cap(2);
		debug_aranges_offset = rx.cap(1).toInt(&ok1, 16);
		debug_aranges_len = rx.cap(2).toInt(&ok2, 16);
		if (!(ok1 && ok2)) Util::panic();
	}
	rx.setPattern("\\.debug_ranges\\s+\\w+\\s+\\w+\\s+(\\w+)\\s+(\\w+)");
	if (rx.indexIn(output) != -1)
	{
		qDebug() << ".debug_ranges at" << rx.cap(1) << "size" << rx.cap(2);
		debug_ranges_offset = rx.cap(1).toInt(&ok1, 16);
		debug_ranges_len = rx.cap(2).toInt(&ok2, 16);
		if (!(ok1 && ok2)) Util::panic();
	}
	rx.setPattern("\\.debug_frame\\s+\\w+\\s+\\w+\\s+(\\w+)\\s+(\\w+)");
	if (rx.indexIn(output) != -1)
	{
		qDebug() << ".debug_frame at" << rx.cap(1) << "size" << rx.cap(2);
		debug_frame_offset = rx.cap(1).toInt(&ok1, 16);
		debug_frame_len = rx.cap(2).toInt(&ok2, 16);
		if (!(ok1 && ok2)) Util::panic();
	}
	rx.setPattern("\\.debug_str\\s+\\w+\\s+\\w+\\s+(\\w+)\\s+(\\w+)");
	if (rx.indexIn(output) != -1)
	{
		qDebug() << ".debug_str at" << rx.cap(1) << "size" << rx.cap(2);
		debug_str_offset = rx.cap(1).toInt(&ok1, 16);
		debug_str_len = rx.cap(2).toInt(&ok2, 16);
		if (!(ok1 && ok2)) Util::panic();
	}
	rx.setPattern("\\.debug_line\\s+\\w+\\s+\\w+\\s+(\\w+)\\s+(\\w+)");
	if (rx.indexIn(output) != -1)
	{
		qDebug() << ".debug_line at" << rx.cap(1) << "size" << rx.cap(2);
		debug_line_offset = rx.cap(1).toInt(&ok1, 16);
		debug_line_len = rx.cap(2).toInt(&ok2, 16);
		if (!(ok1 && ok2)) Util::panic();
	}
	rx.setPattern("\\.debug_loc\\s+\\w+\\s+\\w+\\s+(\\w+)\\s+(\\w+)");
	if (rx.indexIn(output) != -1)
	{
		qDebug() << ".debug_loc at" << rx.cap(1) << "size" << rx.cap(2);
		debug_loc_offset = rx.cap(1).toInt(&ok1, 16);
		debug_loc_len = rx.cap(2).toInt(&ok2, 16);
		if (!(ok1 && ok2)) Util::panic();
	}
}

void MainWindow::updateRegisterView(int frame_number)
{
	ui->tableWidgetRegisters->setRowCount(0);
	auto x = register_cache->registerFrame(frame_number);
	int row;
	for (int i(0); i < x.size(); i ++)
	{
		ui->tableWidgetRegisters->insertRow(row = ui->tableWidgetRegisters->rowCount());
		ui->tableWidgetRegisters->setItem(row, 0, new QTableWidgetItem(QString("r%1").arg(i)));
		ui->tableWidgetRegisters->setItem(row, 1, new QTableWidgetItem(QString("$%1").arg(x.at(i), 0, 16)));
	}
}

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	QCoreApplication::setOrganizationName("shopov instruments");
	QCoreApplication::setApplicationName("troll");
	QSettings s("troll.rc", QSettings::IniFormat);

	setStyleSheet("QSplitter::handle:horizontal { width: 2px; }  /*QSplitter::handle:vertical { height: 20px; }*/ "
	              "QSplitter::handle { border: 1px solid blue; background-color: white; } "
	              );
	
	QTime startup_time;
	elf_filename = "X:/aps-electronics.xs236-gcc/XS236.elf";
//QString elf("X:/build-troll-Desktop_Qt_5_7_0_MinGW_32bit-Debug/main_aps.elf");
	startup_time.start();
	readElfSections();
	ui->setupUi(this);
	restoreGeometry(s.value("window-geometry").toByteArray());
	restoreState(s.value("window-state").toByteArray());
	ui->splitterMain->restoreGeometry(s.value("main-splitter/geometry").toByteArray());
	ui->splitterMain->restoreState(s.value("main-splitter/state").toByteArray());
#if MAIN_APS
	QFile debug_file(elf_filename);
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
	debug_aranges = debug_file.read(debug_aranges_len);
	debug_file.seek(debug_info_offset);
	debug_info = debug_file.read(debug_info_len);
	debug_file.seek(debug_abbrev_offset);
	debug_abbrev = debug_file.read(debug_abbrev_len);
	debug_file.seek(debug_frame_offset);
	debug_frame = debug_file.read(debug_frame_len);
	debug_file.seek(debug_ranges_offset);
	debug_ranges = debug_file.read(debug_ranges_len);
	debug_file.seek(debug_str_offset);
	debug_str = debug_file.read(debug_str_len);
	debug_file.seek(debug_line_offset);
	debug_line = debug_file.read(debug_line_len);
	debug_file.seek(debug_loc_offset);
	debug_loc = debug_file.read(debug_loc_len);
	
	dwdata = new DwarfData(debug_aranges.data(), debug_aranges.length(), debug_info.data(), debug_info.length(), debug_abbrev.data(), debug_abbrev.length(), debug_ranges.data(), debug_ranges.length(), debug_str.data(), debug_str.length(), debug_line.data(), debug_line.length(), debug_loc.data(), debug_loc.length());
	ui->plainTextEdit->appendPlainText(QString("compilation unit count in the .debug_aranges section : %1").arg(dwdata->compilation_unit_count()));
	
#if 0
	
	std::vector<struct DwarfTypeNode> type_cache;
	std::map<uint32_t, uint32_t> x;
	dwdata->get_abbreviations_of_compilation_unit(0, x);
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
	if (0 || context.size())
	{
		qDebug() << context.size();
		qDebug() << context.at(0).offset << context.at(1).offset;
		qDebug() << QString::fromStdString(dwdata->nameOfDie(context.at(1)));
		qDebug() << QString::fromStdString(unwind_data.first);

		target = new TargetCorefile("flash.bin", 0x08000000, "ram.bin", 0x20000000, "registers.bin");
		sforth = new Sforth(ui->plainTextEditSforthConsole);
		cortexm0 = new CortexM0(sforth, target);
		register_cache = new RegisterCache(cortexm0->cfaRegisterNumber());
		cortexm0->primeUnwinder();
		cortexm0->unwindFrame(QString::fromStdString(unwind_data.first), unwind_data.second, 0x800f226);
		auto regs = cortexm0->getRegisters();
		qDebug() << regs;

		qDebug() << "next frame";
		unwind_data = dwundwind->sforthCodeForAddress(regs.at(15));
		qDebug() << QString::fromStdString(unwind_data.first);
		cortexm0->unwindFrame(QString::fromStdString(unwind_data.first), unwind_data.second, regs.at(15));
		regs = cortexm0->getRegisters();
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
	for (i = 0; i < subprograms.size(); i++)
	{
		int row(ui->tableWidgetFunctions->rowCount());
		ui->tableWidgetFunctions->insertRow(row);
		ui->tableWidgetFunctions->setItem(row, 0, new QTableWidgetItem(subprograms.at(i).name));
		ui->tableWidgetFunctions->setItem(row, 1, new QTableWidgetItem(QString("%1").arg(subprograms.at(i).file)));
		ui->tableWidgetFunctions->setItem(row, 2, new QTableWidgetItem(QString("%1").arg(subprograms.at(i).line)));
		ui->tableWidgetFunctions->setItem(row, 3, new QTableWidgetItem(QString("$%1").arg(subprograms.at(i).die_offset, 0, 16)));
	}
	for (i = 0; i < data_objects.size(); i++)
	{
		int row(ui->tableWidgetStaticDataObjects->rowCount());
		ui->tableWidgetStaticDataObjects->insertRow(row);
		ui->tableWidgetStaticDataObjects->setItem(row, 0, new QTableWidgetItem(data_objects.at(i).name));
		ui->tableWidgetStaticDataObjects->setItem(row, 1, new QTableWidgetItem(QString("%1").arg(data_objects.at(i).file)));
		ui->tableWidgetStaticDataObjects->setItem(row, 2, new QTableWidgetItem(QString("%1").arg(data_objects.at(i).line)));
		ui->tableWidgetStaticDataObjects->setItem(row, 3, new QTableWidgetItem(QString("$%1").arg(data_objects.at(i).die_offset, 0, 16)));
	}
	ui->tableWidgetFunctions->sortItems(0);
	ui->tableWidgetStaticDataObjects->sortItems(0);
	qDebug() << "static object lists built in" << t.elapsed() << "milliseconds";

	qDebug() << "debugger startup time:" << startup_time.elapsed() << "milliseconds";

	connect(& blackstrike_port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(blackstrikeError(QSerialPort::SerialPortError)));
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

void MainWindow::on_tableWidgetBacktrace_itemSelectionChanged()
{
QFile src;
QString t;
QTextBlockFormat f, dis;
QTime x;
int i, l, cursor_position_for_line(0);
int row(ui->tableWidgetBacktrace->currentRow());
updateRegisterView(row);
ui->plainTextEdit->clear();
ui->tableWidgetLocalVariables->setRowCount(0);
if (!ui->tableWidgetBacktrace->item(row, 0))
{
	ui->plainTextEdit->appendPlainText("singularity; context undefined");
	return;
}
uint32_t pc(ui->tableWidgetBacktrace->item(row, 0)->text().remove(0, 1).toUInt(0, 16));

	src.setFileName(QString("X:/aps-electronics.xs236-gcc/") + ui->tableWidgetBacktrace->item(row, 4)->text() + "/" + ui->tableWidgetBacktrace->item(row, 2)->text());
	if (src.open(QFile::ReadOnly))
	{
		int i(1);
		l = ui->tableWidgetBacktrace->item(row, 3)->text().toInt();
		while (!src.atEnd())
		{
			if (i == l)
				cursor_position_for_line = t.length() - l + 1;
			t += QString("%1|").arg(i ++, 4, 10, QChar(' ')) + src.readLine().replace('\t', "        ");
		}
	}
	else
		t = QString("cannot open source code file ") + src.fileName();
	ui->plainTextEdit->appendPlainText(t);
	QTextCursor c(ui->plainTextEdit->textCursor());

	std::vector<struct DebugLine::lineAddress> line_addresses;
	x.start();
	dwdata->addressesForFile(ui->tableWidgetBacktrace->item(row, 2)->text().toLatin1().constData(), line_addresses);
	qDebug() << "addresses for file retrieved in " << x.elapsed() << "milliseconds";
	qDebug() << "addresses for file count: " << line_addresses.size();
	x.restart();
	
	std::map<uint32_t, uint32_t> lines;
	for (i = 0; i < line_addresses.size(); i ++)
		lines[line_addresses.at(i).line] ++;
#if 0
	c.movePosition(QTextCursor::Start);
	dis.setBackground(QBrush(Qt::lightGray));
	i = 0;
	while (!c.atEnd())
	{
		i ++;
		if (lines[i])
			c.setBlockFormat(dis);
		if (!c.movePosition(QTextCursor::NextBlock))
			break;
	}
#endif
	c.movePosition(QTextCursor::Start);
#if 0
	c.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, ui->tableWidgetBacktrace->item(row, 3)->text().toInt() - 1);
	qDebug() << "line" << l << "computed position" << cursor_position_for_line << "real position" << c.position() << "delta" << cursor_position_for_line - c.position();
#else
	c.setPosition(cursor_position_for_line);
#endif
	f.setBackground(QBrush(Qt::cyan));
	c.setBlockFormat(f);
	ui->plainTextEdit->setTextCursor(c);
	ui->plainTextEdit->centerCursor();
	qDebug() << "source code view built in " << x.elapsed() << "milliseconds";
	x.restart();
	std::map<uint32_t, uint32_t> abbreviations;
	auto context = dwdata->executionContextForAddress(pc, abbreviations);
	auto locals = dwdata->localDataObjectsForContext(context);
	for (i = 0; i < locals.size(); i ++)
	{
		ui->tableWidgetLocalVariables->insertRow(row = ui->tableWidgetLocalVariables->rowCount());
		ui->tableWidgetLocalVariables->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(dwdata->nameOfDie(locals.at(i)))));
		std::vector<DwarfTypeNode> type_cache;
		dwdata->readType(locals.at(i).offset, abbreviations, type_cache);
		ui->tableWidgetLocalVariables->setItem(row, 1, new QTableWidgetItem(QString("%1").arg(dwdata->sizeOf(type_cache))));
		ui->tableWidgetLocalVariables->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(dwdata->locationSforthCode(locals.at(i), context.at(0), pc))));
		ui->tableWidgetLocalVariables->setItem(row, 3, new QTableWidgetItem(QString("$%1").arg(locals.at(i).offset, 0, 16)));
	}
	qDebug() << "local data objects view built in " << x.elapsed() << "milliseconds";
}

void MainWindow::blackstrikeError(QSerialPort::SerialPortError error)
{
	if (error != QSerialPort::NoError)
		Util::panic();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
QSettings s("troll.rc", QSettings::IniFormat);
	s.setValue("window-geometry", saveGeometry());
	s.setValue("window-state", saveState());
	s.setValue("main-splitter/geometry", ui->splitterMain->saveGeometry());
	s.setValue("main-splitter/state", ui->splitterMain->saveState());
	dwdata->dumpStats();
	QMainWindow::closeEvent(e);
}

void MainWindow::on_actionSingle_step_triggered()
{
	target->singleStep();
	backtrace();
}

void MainWindow::on_actionBlackstrikeConnect_triggered()
{
auto ports = QSerialPortInfo::availablePorts();
int i;
class Target * t;
	for (i = 0; i < ports.size(); i ++)
	{
		if (ports.at(i).hasProductIdentifier() && ports.at(i).vendorIdentifier() == 0x1d50)
		{
			blackstrike_port.setPortName(ports.at(i).portName());
			t = new Blackstrike(& blackstrike_port);
			if (blackstrike_port.open(QSerialPort::ReadWrite))
			{
				QMessageBox::information(0, "blackstrike port opened", "opened blackstrike port " + blackstrike_port.portName());
				cortexm0->setTargetController(target = t);
				backtrace();
				return;
			}
			else
				delete t;
		}
	}
	QMessageBox::warning(0, "blackstrike port not found", "cannot find blackstrike gdbserver port ");
}
