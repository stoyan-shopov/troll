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
#include "troll.hxx"
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
#include <QDir>
#include <QTextBlock>
#include <QFileDialog>

#define DEBUG_BACKTRACE		0

/* syntax highlighter copied from the Qt documentation example on syntax highlighting */
Highlighter::Highlighter(QTextDocument *parent)
        : QSyntaxHighlighter(parent)
{
	HighlightingRule rule;

	//keywordFormat.setForeground(Qt::darkBlue);
	keywordFormat.setForeground(QColor(0x60, 0xff, 0x60));
	//keywordFormat.setFontWeight(QFont::Bold);
	QStringList keywordPatterns;
	keywordPatterns << "\\bchar\\b" << "\\bclass\\b" << "\\bconst\\b"
	                << "\\bdouble\\b" << "\\benum\\b" << "\\bexplicit\\b"
	                << "\\bfriend\\b" << "\\binline\\b" << "\\bint\\b"
	                << "\\blong\\b" << "\\bnamespace\\b" << "\\boperator\\b"
	                << "\\bprivate\\b" << "\\bprotected\\b" << "\\bpublic\\b"
	                << "\\bshort\\b" << "\\bsignals\\b" << "\\bsigned\\b"
	                << "\\bslots\\b" << "\\bstatic\\b" << "\\bstruct\\b"
	                << "\\btemplate\\b" << "\\btypedef\\b" << "\\btypename\\b"
	                << "\\bunion\\b" << "\\bunsigned\\b" << "\\bvirtual\\b"
	                << "\\bvoid\\b" << "\\bvolatile\\b";
	foreach (const QString &pattern, keywordPatterns) {
		rule.pattern = QRegExp(pattern);
		rule.format = keywordFormat;
		highlightingRules.append(rule);
	}
	//classFormat.setFontWeight(QFont::Bold);
	classFormat.setForeground(Qt::darkMagenta);
	rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
	rule.format = classFormat;
	highlightingRules.append(rule);

	quotationFormat.setForeground(Qt::darkGreen);
	rule.pattern = QRegExp("\".*\"");
	rule.format = quotationFormat;
	highlightingRules.append(rule);

	//functionFormat.setFontItalic(true);
	//functionFormat.setForeground(Qt::blue);
	functionFormat.setForeground(QColor(0x40, 0xff, 0xff));
	rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
	rule.format = functionFormat;
	highlightingRules.append(rule);

	//singleLineCommentFormat.setForeground(Qt::red);
	singleLineCommentFormat.setForeground(QColor(0x80, 0xa0, 0xff));
	rule.pattern = QRegExp("//[^\n]*");
	rule.format = singleLineCommentFormat;
	highlightingRules.append(rule);

	//multiLineCommentFormat.setForeground(Qt::red);
	multiLineCommentFormat.setForeground(QColor(0x80, 0xa0, 0xff));

	commentStartExpression = QRegExp("/\\*");
	commentEndExpression = QRegExp("\\*/");
}

void Highlighter::highlightBlock(const QString &text)
{
	foreach (const HighlightingRule &rule, highlightingRules) {
		QRegExp expression(rule.pattern);
		int index = expression.indexIn(text);
		while (index >= 0) {
			int length = expression.matchedLength();
			setFormat(index, length, rule.format);
			index = expression.indexIn(text, index + length);
		}
	}
	setCurrentBlockState(0);
	int startIndex = 0;
	if (previousBlockState() != 1)
		startIndex = commentStartExpression.indexIn(text);	
	while (startIndex >= 0) {
		int endIndex = commentEndExpression.indexIn(text, startIndex);
		int commentLength;
		if (endIndex == -1) {
			setCurrentBlockState(1);
			commentLength = text.length() - startIndex;
		} else {
			commentLength = endIndex - startIndex
			                + commentEndExpression.matchedLength();
		}
		setFormat(startIndex, commentLength, multiLineCommentFormat);
		startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
	}
}


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

QTreeWidgetItem * MainWindow::itemForNode(const DwarfData::DataNode &node, const QByteArray & data, int data_pos, int numeric_base, const QString & numeric_prefix)
{
auto n = new QTreeWidgetItem(QStringList() << QString::fromStdString(node.data.at(0)) << QString("%1").arg(node.bytesize) << "???" << QString("%1").arg(node.data_member_location));
int i;
	if (node.is_pointer)
		n->setData(0, Qt::UserRole, QVariant::fromValue((TreeWidgetNodeData) { .pointer_type_die_offset = node.die_offset, }));
	if (!node.children.size())
	{
		if (data_pos + node.bytesize <= data.size()) switch (node.bytesize)
		{
		uint64_t x;
			case 1: x = * (uint8_t *) (data.data() + data_pos); if (0)
			case 2: x = * (uint16_t *) (data.data() + data_pos); if (0)
			case 4: x = * (uint32_t *) (data.data() + data_pos); if (0)
			case 8: x = * (uint64_t *) (data.data() + data_pos);
				if (node.bitsize)
				{
					x >>= node.bitposition, x &= (1 << node.bitsize) - 1;
					n->setText(1, QString("%1 bit").arg(node.bitsize) + ((node.bitsize != 1) ? "s":""));
				}
				n->setText(2, node.is_pointer ? QString("$%1").arg(x, 8, 16, QChar('0')) : numeric_prefix + QString("%1").arg(x, 0, numeric_base));
				if (node.is_enumeration)
					n->setText(2, n->text(2) + " (" + QString::fromStdString(dwdata->enumeratorNameForValue(x, node.die_offset) + ")"));
				break;
			default:
n->setText(2, "<<< UNKNOWN SIZE >>>");
break;
				Util::panic();
		}
		/*
		else
			Util::panic();
			*/
	}
	if (node.array_dimensions.size())
		for (i = 0; i < (signed) node.array_dimensions.at(0); n->addChild(itemForNode(node.children.at(0), data, data_pos + i * node.children.at(0).bytesize, numeric_base, numeric_prefix)), i ++);
	else
		for (i = 0; i < node.children.size(); n->addChild(itemForNode(node.children.at(i), data, data_pos + node.children.at(i).data_member_location, numeric_base, numeric_prefix)), i ++);
	return n;
}

void MainWindow::displaySourceCodeFile(QString source_filename, QString directory_name, QString compilation_directory, int highlighted_line, uint32_t address)
{
        source_filename.replace(QChar('\\'), QChar('/'));
        directory_name.replace(QChar('\\'), QChar('/'));
        compilation_directory.replace(QChar('\\'), QChar('/'));
        if (TEST_DRIVE_MODE)
	{
		QRegExp rx("^[xX]:[/\\\\]");
		source_filename.replace(rx, "troll-test-drive-files/"), directory_name.replace(rx, "troll-test-drive-files/"), compilation_directory.replace(rx, "troll-test-drive-files/");
        }
QTime stime;
stime.start();
QFile src;
QString t;
QTextBlockFormat f;
QTextCharFormat cf;
QTime x;
int i, cursor_position_for_line(0);
QFileInfo finfo(directory_name + "/" + source_filename);
struct SourceLevelBreakpoint b = { .source_filename = source_filename, .directory_name = directory_name, .compilation_directory = compilation_directory, };
QVector<uint32_t> breakpoint_positions;
QMap<uint32_t /* address */, int /* line position in text document */> addresses;

	if (!finfo.exists())
                finfo.setFile(compilation_directory + "/" + source_filename);
	if (!finfo.exists())
                finfo.setFile(compilation_directory + "/" + directory_name + "/" + source_filename);
	ui->plainTextEdit->clear();
	src.setFileName(finfo.canonicalFilePath());
	
	std::vector<struct DebugLine::lineAddress> line_addresses;
	x.start();
	dwdata->addressesForFile(source_filename.toLocal8Bit().constData(), line_addresses);
	if (/* this is not exact, which it needs not be */ x.elapsed() > profiling.max_addresses_for_file_retrieval_time)
		profiling.max_addresses_for_file_retrieval_time = x.elapsed();
	qDebug() << "addresses for file retrieved in " << x.elapsed() << "milliseconds";
	qDebug() << "addresses for file count: " << line_addresses.size();
	x.restart();
	
	std::map<uint32_t, struct DebugLine::lineAddress *> lines;
	for (i = line_addresses.size() - 1; i >= 0; i --)
	{
		line_addresses.at(i).next = lines[line_addresses.at(i).line];
		lines[line_addresses.at(i).line] = & line_addresses.at(i);
	}

	if (src.open(QFile::ReadOnly))
	{
		int i(1);
		struct DebugLine::lineAddress * dis;
		while (!src.atEnd())
		{
			if (i == highlighted_line)
				cursor_position_for_line = t.length();
			b.line_number = i;
			if (breakpointIndex(b) != -1 || inferredBreakpointIndex(b) != -1)
				breakpoint_positions.push_back(t.length());
			t += QString("%1 %2|").arg(lines[i] ? '*' : ' ')
			                .arg(i, 4, 10, QChar(' ')) + src.readLine().replace('\t', "        ").replace('\r', "");
			if (ui->actionShow_disassembly_address_ranges->isChecked())
			{
				if (0 && address == -1)
					Util::panic();
				dis = lines[i];
				while (dis)
				{
					//t += QString("$%1 - $%2\n").arg(dis->address, 0, 16).arg(dis->address_span, 0, 16), dis = dis->next;
					auto x = disassembly->disassemblyForRange(dis->address, dis->address_span);
					int i;
					for (i = 0; i < x.size(); i ++)
					{
						addresses.insert(x.at(i).first, t.length());
						if (address == x.at(i).first)
							cursor_position_for_line = t.length();
						t += QString(x.at(i).second).replace('\r', "") + "\n";
					}
					t += "...\n";
					dis = dis->next;
				}
			}
			i ++;
		}
	}
	else
		t = QString("cannot open source code file ") + src.fileName();
	
	QSet<uint32_t> breakpointed_addresses;
	for (i = 0; i < breakpoints.size(); i ++)
	{
		int j;
		for (j = 0; j < breakpoints.at(i).addresses.size(); j ++)
			breakpointed_addresses.insert(breakpoints.at(i).addresses.at(j));
	}
	for (i = 0; i < machine_level_breakpoints.size(); i ++)
		breakpointed_addresses.insert(machine_level_breakpoints.at(i).address);
	QSet<uint32_t>::const_iterator bset = breakpointed_addresses.constBegin();
	while (bset != breakpointed_addresses.constEnd())
	{
		if (addresses.contains(* bset))
			breakpoint_positions.push_back(addresses.operator [](*bset));
		bset ++;
	}

	ui->plainTextEdit->appendPlainText(t);
	QTextCursor c(ui->plainTextEdit->textCursor());
	f.setBackground(QBrush(Qt::red));
	for (i = 0; i < breakpoint_positions.size(); i ++)
		c.setPosition(breakpoint_positions.at(i)), c.setBlockFormat(f);

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
	cf.setForeground(QBrush(Qt::blue));
	c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
	c.setBlockFormat(f);
	c.setCharFormat(cf);
	c.clearSelection();
	cf.setForeground(QBrush(Qt::white));
	c.setCharFormat(cf);
	ui->plainTextEdit->setTextCursor(c);
	ui->plainTextEdit->centerCursor();
	if (/* this is not exact, which it needs not be */ x.elapsed() > profiling.max_source_code_view_build_time)
		profiling.max_source_code_view_build_time = x.elapsed();
	qDebug() << "source code view built in " << x.elapsed() << "milliseconds";
	if (/* this is not exact, which it needs not be */ stime.elapsed() > profiling.max_context_view_generation_time)
		profiling.max_context_view_generation_time = stime.elapsed();
	last_source_filename = source_filename;
	last_directory_name = directory_name;
	last_compilation_directory = compilation_directory;
	last_highlighted_line = highlighted_line;
	last_source_highlighted_address = address;
	current_source_code_file_displayed = src.fileName();
	statusBar()->showMessage(current_source_code_file_displayed);
}

void MainWindow::backtrace()
{
	QTime t;
	t.start();
	cortexm0->primeUnwinder();
	register_cache->clear();
	register_cache->pushFrame(cortexm0->getRegisters());
	uint32_t last_pc, last_stack_pointer;
	auto context = dwdata->executionContextForAddress(last_pc = cortexm0->programCounter());
	last_stack_pointer = cortexm0->stackPointerValue();
	int row;
	
	if (DEBUG_BACKTRACE) qDebug() << "backtrace:";
	ui->tableWidgetBacktrace->blockSignals(true);
	ui->tableWidgetBacktrace->setRowCount(0);
	ui->tableWidgetBacktrace->blockSignals(false);
	while (context.size())
	{
		auto subprogram = dwdata->topLevelSubprogramOfContext(context);
		auto unwind_data = dwundwind->sforthCodeForAddress(cortexm0->programCounter());
		auto x = dwdata->sourceCodeCoordinatesForAddress(cortexm0->programCounter());
		if (DEBUG_BACKTRACE) qDebug() << x.file_name << (signed) x.line;
		if (DEBUG_BACKTRACE) qDebug() << "dwarf unwind program:" << QString::fromStdString(unwind_data.first) << "address:" << unwind_data.second;

		if (DEBUG_BACKTRACE) qDebug() << cortexm0->programCounter() << QString(dwdata->nameOfDie(subprogram));
		ui->tableWidgetBacktrace->insertRow(row = ui->tableWidgetBacktrace->rowCount());
		ui->tableWidgetBacktrace->setItem(row, 0, new QTableWidgetItem(QString("$%1").arg(cortexm0->programCounter(), 8, 16, QChar('0'))));
		ui->tableWidgetBacktrace->setVerticalHeaderItem(row, new QTableWidgetItem(QString("%1").arg(register_cache->frameCount())));
		ui->tableWidgetBacktrace->verticalHeaderItem(row)->setData(Qt::UserRole, register_cache->frameCount() - 1);
		ui->tableWidgetBacktrace->setItem(row, 1, new QTableWidgetItem(QString(dwdata->nameOfDie(subprogram))));
		ui->tableWidgetBacktrace->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(x.file_name)));
		ui->tableWidgetBacktrace->setItem(row, 3, new QTableWidgetItem(QString("%1").arg(x.line)));
		ui->tableWidgetBacktrace->setItem(row, 4, new QTableWidgetItem(x.directory_name));
		ui->tableWidgetBacktrace->setItem(row, 5, new QTableWidgetItem(x.compilation_directory_name));
		ui->tableWidgetBacktrace->setItem(row, 6, new QTableWidgetItem(QString("$%1").arg(subprogram.offset, 0, 16)));
		ui->tableWidgetBacktrace->setItem(row, 7, new QTableWidgetItem(QString::fromStdString(dwdata->sforthCodeFrameBaseForContext(context))));
		
		int i;
		auto inlining_chain = dwdata->inliningChainOfContext(context);
		for (i = inlining_chain.size() - 1; i >= 0; i --)
		{
			ui->tableWidgetBacktrace->insertRow(row = ui->tableWidgetBacktrace->rowCount());
			ui->tableWidgetBacktrace->setVerticalHeaderItem(row, new QTableWidgetItem("inlined"));
			ui->tableWidgetBacktrace->verticalHeaderItem(row)->setData(Qt::UserRole, register_cache->frameCount() - 1);
			ui->tableWidgetBacktrace->setItem(row, 1, new QTableWidgetItem(dwdata->nameOfDie(inlining_chain.at(i))));
			ui->tableWidgetBacktrace->setItem(row, 6, new QTableWidgetItem(QString("$%1").arg(inlining_chain.at(i).offset, 0, 16)));
		}
		
		if (cortexm0->unwindFrame(QString::fromStdString(unwind_data.first), unwind_data.second, cortexm0->programCounter()))
			context = dwdata->executionContextForAddress(cortexm0->programCounter()), register_cache->pushFrame(cortexm0->getRegisters());
		if (context.empty() && cortexm0->architecturalUnwind())
		{
			context = dwdata->executionContextForAddress(cortexm0->programCounter());
			if (!context.empty())
			{
				if (DEBUG_BACKTRACE) qDebug() << "architecture-specific unwinding performed";
				register_cache->pushFrame(cortexm0->getRegisters());
				ui->tableWidgetBacktrace->insertRow(row = ui->tableWidgetBacktrace->rowCount());
				ui->tableWidgetBacktrace->setItem(row, 1, new QTableWidgetItem("architecture-specific unwinding performed"));
				ui->tableWidgetBacktrace->setVerticalHeaderItem(row, new QTableWidgetItem(QString("%1 (singularity)").arg(register_cache->frameCount() - 1)));
				ui->tableWidgetBacktrace->verticalHeaderItem(row)->setData(Qt::UserRole, register_cache->frameCount() - 2);
			}
		}
		if (last_pc == cortexm0->programCounter() && last_stack_pointer == cortexm0->stackPointerValue())
			break;
		last_pc = cortexm0->programCounter();
		last_stack_pointer = cortexm0->stackPointerValue();
	}
	if (DEBUG_BACKTRACE) qDebug() << "registers: " << cortexm0->getRegisters();
	ui->tableWidgetBacktrace->resizeColumnsToContents();
	ui->tableWidgetBacktrace->resizeRowsToContents();
	int line_in_disassembly;
	ui->plainTextEdit->setPlainText(disassembly->disassemblyAroundAddress(register_cache->cachedRegisterFrame(0).at(15), & line_in_disassembly));
	QTextCursor c(ui->plainTextEdit->textCursor());
	c.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, line_in_disassembly);
	QTextBlockFormat f;
	f.setBackground(QBrush(Qt::cyan));
	c.setBlockFormat(f);
	if (ui->tableWidgetBacktrace->rowCount())
		ui->tableWidgetBacktrace->selectRow(0);
	else
		updateRegisterView(0);
	if (/* this is not exact, which it needs not be */ t.elapsed() > profiling.max_backtrace_generation_time)
		profiling.max_backtrace_generation_time = t.elapsed();
}

bool MainWindow::readElfSections(void)
{
	debug_aranges_index =
	debug_info_index =
	debug_abbrev_index =
	debug_frame_index =
	debug_ranges_index =
	debug_str_index =
	debug_line_index =
	debug_loc_index = 0;

int i;

	if (elf.get_class() != ELFCLASS32 || elf.get_encoding() != ELFDATA2LSB)
	{
		QMessageBox::critical(0, "invalid ELF file",
				      "cannot read ELF file - only 32 bit, little-endian encoded ELF files are supported"
				      "\n\nthe troll will now abort");
		exit(3);
	}
	for (i = /* section number zero - unused (null section) */ 1; i < elf.sections.size(); i ++)
	{
		auto name = elf.sections[i]->get_name();
		if (name == ".debug_aranges") debug_aranges_index = i;
		else if (name == ".debug_info") debug_info_index = i;
		else if (name == ".debug_abbrev") debug_abbrev_index = i;
		else if (name == ".debug_frame") debug_frame_index = i;
		else if (name == ".debug_ranges") debug_ranges_index = i;
		else if (name == ".debug_str") debug_str_index = i;
		else if (name == ".debug_line") debug_line_index = i;
		else if (name == ".debug_loc") debug_loc_index = i;
	}
	return true;
}

bool MainWindow::loadSRecordFile(void)
{
QProcess objcopy;
QString outfile = QFileInfo(elf_filename).fileName();

	if (!TEST_DRIVE_MODE)
	{
		objcopy.start("arm-none-eabi-objcopy", QStringList() << "-O" << "srec" << elf_filename << outfile + ".srec");
		objcopy.waitForFinished();
		if (objcopy.error() != QProcess::UnknownError || objcopy.exitCode() || objcopy.exitStatus() != QProcess::NormalExit)
		{
			QMessageBox::critical(0, "error retrieving target memory areas",
			                      "error running the 'arm-none-eabi-objcopy' utility in order to retrieve\n"
			                      "target memory area contents!\n\n"
			                      "please, make sure that the 'arm-none-eabi-objcopy' utility is accessible\n"
			                      "in your path environment, and that the file you have specified\n"
			                      "is indeed an ELF file!\n\n"
			                      "target flash programming and verification will be unavailable in this session"
			                      );
			return false;
		}
		return SRecordMemoryData::loadFile(outfile + ".srec", target_memory_contents);
	}
	else
		return SRecordMemoryData::loadFile(QString("troll-test-drive-files/") + outfile + ".srec", target_memory_contents);
}

bool MainWindow::loadElfMemorySegments(void)
{
int i;
	for (i = 0; i < elf.segments.size(); i ++)
		if (elf.segments[i]->get_type() == PT_LOAD || elf.segments[i]->get_type() == PT_ARM_EXIDX)
			target_memory_contents.addRange(elf.segments[i]->get_physical_address(), QByteArray(elf.segments[i]->get_data(), elf.segments[i]->get_file_size()));
}

void MainWindow::updateRegisterView(int frame_number)
{
	auto x = register_cache->cachedRegisterFrame(frame_number);
	for (int row(0); row < x.size(); row ++)
	{
		QString s(QString("$%1").arg(x.at(row), 0, 16)), t = ui->tableWidgetRegisters->item(row, 1)->text();
		ui->tableWidgetRegisters->setItem(row, 0, new QTableWidgetItem(QString("r%1").arg(row)));
		ui->tableWidgetRegisters->setItem(row, 1, new QTableWidgetItem(s));
		ui->tableWidgetRegisters->item(row, 1)->setForeground(QBrush((s != t) ? Qt::red : Qt::black));
	}
	ui->tableWidgetRegisters->resizeColumnsToContents();
	ui->tableWidgetRegisters->resizeRowsToContents();
}

std::string MainWindow::typeStringForDieOffset(uint32_t die_offset)
{
	std::vector<struct DwarfTypeNode> type_cache;
	dwdata->readType(die_offset, type_cache);
	return dwdata->typeString(type_cache, true, 1);
}

void MainWindow::dumpData(uint32_t address, const QByteArray &data)
{
	ui->plainTextEditDataDump->clear();
	ui->plainTextEditDataDump->appendPlainText(data.toPercentEncoding());
}

void MainWindow::updateBreakpoints(void)
{
int i, j;
	ui->treeWidgetBreakpoints->clear();
	for (i = 0; i < breakpoints.size(); i ++)
	{
		const SourceLevelBreakpoint & b(breakpoints.at(i));
		auto t = new QTreeWidgetItem(ui->treeWidgetBreakpoints, QStringList() << b.source_filename << QString("%1").arg(b.line_number));
		if (b.addresses.size() == 1)
			t->setText(2, QString("$%1").arg(b.addresses.at(0), 8, 16, QChar('0')));
		else
		{
			t->setText(2, t->text(2) + QString("%1 locations").arg(b.addresses.size()));
			for (j = 0; j < b.addresses.size(); j ++)
			{
				auto n = new QTreeWidgetItem(t);
				n->setText(2, QString("$%1").arg(b.addresses.at(j), 8, 16, QChar('0')));
			}
		}
	}
	if (machine_level_breakpoints.size())
	{
		auto t = new QTreeWidgetItem(ui->treeWidgetBreakpoints, QStringList() << "machine-level breakpoints");
		for (i = 0; i < machine_level_breakpoints.size(); i ++)
			new QTreeWidgetItem(t, QStringList()
				<< machine_level_breakpoints.at(i).inferred_breakpoint.source_filename
				<< QString("%1").arg(machine_level_breakpoints.at(i).inferred_breakpoint.line_number)
				<< QString("$%1").arg(machine_level_breakpoints.at(i).address, 8, 16, QChar('0')));
		ui->treeWidgetBreakpoints->expandItem(t);
	}
	
}

static bool sortSourcefiles(const struct DebugLine::sourceFileNames & a, const struct DebugLine::sourceFileNames & b)
{ auto x = strcmp(a.file, b.file); if (x) return x < 0; return strcmp(a.directory, b.directory) < 0; }

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	QTime startup_time;
	execution_state = INVALID_EXECUTION_STATE;
	int i;
	QCoreApplication::setOrganizationName("shopov instruments");
	QCoreApplication::setApplicationName("troll");
	QSettings s("troll.rc", QSettings::IniFormat);
	memset(& profiling, 0, sizeof profiling);

	setStyleSheet("QSplitter::handle:horizontal { width: 2px; }  /*QSplitter::handle:vertical { height: 20px; }*/ "
	              "QSplitter::handle { border: 1px solid blue; background-color: white; } "
		      "QTableWidget::item{ selection-background-color: teal}"
 
"QTreeView::branch:has-siblings:!adjoins-item {"
    "border-image: url(:/resources/images/stylesheet-vline.png) 0;"
"}"

"QTreeView::branch:has-siblings:adjoins-item {"
    "border-image: url(:/resources/images/stylesheet-branch-more.png) 0;"
"}"

"QTreeView::branch:!has-children:!has-siblings:adjoins-item {"
    "border-image: url(:/resources/images/stylesheet-branch-end.png) 0;"
"}"

"QTreeView::branch:has-children:!has-siblings:closed,"
"QTreeView::branch:closed:has-children:has-siblings {"
        "border-image: none;"
        "image: url(:/resources/images/stylesheet-branch-closed.png);"
"}"

"QTreeView::branch:open:has-children:!has-siblings,"
"QTreeView::branch:open:has-children:has-siblings  {"
        "border-image: none;"
        "image: url(:/resources/images/stylesheet-branch-open.png);"
"}"
	              );
	
	ui->setupUi(this);
	if (TEST_DRIVE_MODE)
	{
		setWindowTitle(windowTitle() + " - !!! TEST DRIVE MODE !!!");
		auto wd = QDir::currentPath();
		QFileInfo fi(wd + "/troll-test-drive-files");
		if (!fi.exists() || !fi.isDir())
		{
			QMessageBox::critical(0, "static test-drive mode files not found",
				QString("Static troll test-drive mode files not found!!!<br><br>") +
				"You are now running the troll in <b>static test-drive mode</b>.<br>"
				"This is <b><i>IMPOSSIBLE</i></b> without the troll having access to the test files<br><br>"
				"In <b>static test-drive mode</b>, the files for the test must<br>"
				"reside in a directory named <b>'troll-test-drive-files'</b>,<br>"
				"and this directory must reside in the troll's working directory<br><br>"
				"The current troll's working directory is:<br><b>" + wd + "</b><br>"
				"and a directory with the name <b>'troll-test-drive-files'</b>"
				"does not exist there<br><br>"
				"You can obtain snapshots of the <b>'troll-test-drive-files'</b> directory here:<br>"
				"<b>https://github.com/stoyan-shopov/troll-test-drive-snapshots</b><br><br>"
				"Please, do take time to set up your directories properly, and re-run the troll<br><br>"
				"Thank you for your interest in the troll!"
				);
			exit(4);
		}
	}
	restoreGeometry(s.value("window-geometry").toByteArray());
	restoreState(s.value("window-state").toByteArray());
	ui->splitterMain->restoreGeometry(s.value("main-splitter/geometry").toByteArray());
	ui->splitterMain->restoreState(s.value("main-splitter/state").toByteArray());
	ui->actionHack_mode->setChecked(s.value("hack-mode", true).toBool());
	ui->actionHack_mode->setText(!ui->actionHack_mode->isChecked() ? "to hack mode" : "to user mode");
	ui->actionShow_disassembly_address_ranges->setChecked(s.value("show-disassembly-ranges", true).toBool());
	ui->comboBoxDataDisplayNumericBase->setCurrentIndex(s.value("data-display-numeric-base", 1).toUInt());
	ui->plainTextEditScratchpad->setPlainText(s.value("scratchpad-contents").toString());
	{
		QStringList bookmarks = s.value("bookmarks").toStringList();
		int i;
		for (i = 0; i < bookmarks.size() / 4; i ++)
		{
			ui->tableWidgetBookmarks->insertRow(i);
			ui->tableWidgetBookmarks->setItem(i, 0, new QTableWidgetItem(bookmarks.at(i * 4 + 0)));
			ui->tableWidgetBookmarks->setItem(i, 1, new QTableWidgetItem(bookmarks.at(i * 4 + 1)));
			ui->tableWidgetBookmarks->setItem(i, 2, new QTableWidgetItem(bookmarks.at(i * 4 + 2)));
			ui->tableWidgetBookmarks->setItem(i, 3, new QTableWidgetItem(bookmarks.at(i * 4 + 3)));
		}
	}
	elf_filename = s.value("last-elf-file", QString("???")).toString();
	on_actionHack_mode_triggered();
	QFile debug_file;
	QFileInfo file_info(elf_filename);
	
	if (TEST_DRIVE_MODE)
	{
		elf_filename = "troll-test-drive-files/blackmagic.elf";
	}
	else
	{
		if (file_info.exists())
		{
			if (QMessageBox::question(0, "Reopen last file", "Reopen last file, or select a new file?", QString("Reopen\n")
			                          + file_info.fileName(), "Select new") == 1)
				goto there;
		}
		else
		{
			QMessageBox::warning(0, "file not found", QString("file ") + elf_filename + " not found\nplease select an elf file for debugging");
there:
			elf_filename = QFileDialog::getOpenFileName(0, "select an elf file for debugging");
		}
	}
	startup_time.start();

	QTime t;
	t.start();
	if (!elf.load(elf_filename.toStdString()))
	{
		QMessageBox::critical(0, "error loading target ELF file",
				      "cannot read ELF file " + elf_filename +
				      "\n\nthe troll will now abort");
		exit(2);
	}
	profiling.debug_sections_disk_read_time = t.elapsed();
	
	if (!readElfSections())
		exit(1);
	debug_file.setFileName(elf_filename);

	loadElfMemorySegments();

	if (TEST_DRIVE_MODE)
	{
		QFile f("troll-test-drive-files/disassembly.txt");
		f.open(QFile::ReadOnly);
		disassembly = new Disassembly(f.readAll(), target_memory_contents);
	}
	else
	{
		QProcess objdump;
		objdump.start("arm-none-eabi-objdump", QStringList() << "-d" << elf_filename);
		objdump.waitForFinished();

		if (objdump.error() != QProcess::UnknownError || objdump.exitCode() || objdump.exitStatus() != QProcess::NormalExit)
		{
			QMessageBox::critical(0, "error disassembling the target ELF file",
			                      "error running the 'arm-none-eabi-objdump' utility in order to disassemble\n"
			                      "the target ELF file!\n\n"
			                      "please, make sure that the 'arm-none-eabi-objdump' utility is accessible\n"
			                      "in your path environment, and that the file you have specified\n"
			                      "is indeed an ELF file!\n\n"
			                      "disassembly will be unavailable in this session!"
			                      );
			disassembly = new Disassembly(QByteArray(), target_memory_contents);
		}
		else
			disassembly = new Disassembly(objdump.readAll(), target_memory_contents);
	}
	if (!debug_file.open(QFile::ReadOnly))
	{
		QMessageBox::critical(0, "error opening target executable", QString("error opening file ") + debug_file.fileName());
		exit(2);
	}
	if (debug_aranges_index) debug_aranges = QByteArray(elf.sections[debug_aranges_index]->get_data(), elf.sections[debug_aranges_index]->get_size());
	if (debug_info_index) debug_info = QByteArray(elf.sections[debug_info_index]->get_data(), elf.sections[debug_info_index]->get_size());
	if (debug_abbrev_index) debug_abbrev = QByteArray(elf.sections[debug_abbrev_index]->get_data(), elf.sections[debug_abbrev_index]->get_size());
	if (debug_frame_index) debug_frame = QByteArray(elf.sections[debug_frame_index]->get_data(), elf.sections[debug_frame_index]->get_size());
	if (debug_ranges_index) debug_ranges = QByteArray(elf.sections[debug_ranges_index]->get_data(), elf.sections[debug_ranges_index]->get_size());
	if (debug_str_index) debug_str = QByteArray(elf.sections[debug_str_index]->get_data(), elf.sections[debug_str_index]->get_size());
	if (debug_line_index) debug_line = QByteArray(elf.sections[debug_line_index]->get_data(), elf.sections[debug_line_index]->get_size());
	if (debug_loc_index) debug_loc = QByteArray(elf.sections[debug_loc_index]->get_data(), elf.sections[debug_loc_index]->get_size());
	
	t.restart();
	dwdata = new DwarfData(debug_aranges.data(), debug_aranges.length(), debug_info.data(), debug_info.length(), debug_abbrev.data(), debug_abbrev.length(), debug_ranges.data(), debug_ranges.length(), debug_str.data(), debug_str.length(), debug_line.data(), debug_line.length(), debug_loc.data(), debug_loc.length());
	ui->plainTextEdit->appendPlainText(QString("compilation unit count in the .debug_aranges section : %1").arg(dwdata->compilation_unit_count()));
	profiling.all_compilation_units_processing_time = t.elapsed();
	qDebug() << "all compilation units in .debug_info processed in" << profiling.all_compilation_units_processing_time << "milliseconds";
	dwdata->dumpStats();
	
	dwundwind = new DwarfUnwinder(debug_frame.data(), debug_frame.length());
	while (!dwundwind->at_end())
		dwundwind->dump(), dwundwind->next();
	
	if (TEST_DRIVE_MODE)
		target = new TargetCorefile("troll-test-drive-files/flash.bin", 0x08000000, "troll-test-drive-files/ram.bin", 0x20000000, "troll-test-drive-files/registers.bin");
	else
		target = new TargetCorefile("flash.bin", 0x08000000, "ram.bin", 0x20000000, "registers.bin");
	
	sforth = new Sforth(ui->plainTextEditSforthConsole);
	cortexm0 = new CortexM0(sforth, target);
	dwarf_evaluator = new DwarfEvaluator(sforth);
	register_cache = new RegisterCache();
	cortexm0->primeUnwinder();
	for (int row(0); row < CortexM0::registerCount(); row ++)
	{
		ui->tableWidgetRegisters->insertRow(row);
		ui->tableWidgetRegisters->setItem(row, 0, new QTableWidgetItem(QString("r?")));
		ui->tableWidgetRegisters->setItem(row, 1, new QTableWidgetItem("????????"));
	}
	backtrace();
	
	t.restart();
	dwdata->dumpLines();
	profiling.debug_lines_processing_time = t.elapsed();
	qDebug() << ".debug_lines section processed in" << profiling.debug_lines_processing_time << "milliseconds";
	t.restart();
	std::vector<struct StaticObject> data_objects, subprograms;
	dwdata->reapStaticObjects(data_objects, subprograms);
	profiling.static_storage_duration_data_reap_time = t.elapsed();
	qDebug() << "static storage duration data reaped in" << profiling.static_storage_duration_data_reap_time << "milliseconds";
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
		if (!(i % 5000))
			qDebug() << "constructing static subprograms view:" << subprograms.size() - i << "remaining";
	}
	for (i = 0; i < data_objects.size(); i++)
	{
		int row(ui->tableWidgetStaticDataObjects->rowCount());
		ui->tableWidgetStaticDataObjects->insertRow(row);
		ui->tableWidgetStaticDataObjects->setItem(row, 0, new QTableWidgetItem(data_objects.at(i).name));
		ui->tableWidgetStaticDataObjects->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(typeStringForDieOffset(data_objects.at(i).die_offset))));
		ui->tableWidgetStaticDataObjects->setItem(row, 2, new QTableWidgetItem(QString("$%1").arg(data_objects.at(i).address, 0, 16)));
		ui->tableWidgetStaticDataObjects->setItem(row, 3, new QTableWidgetItem(QString("%1").arg(data_objects.at(i).file)));
		ui->tableWidgetStaticDataObjects->setItem(row, 4, new QTableWidgetItem(QString("%1").arg(data_objects.at(i).line)));
		ui->tableWidgetStaticDataObjects->setItem(row, 5, new QTableWidgetItem(QString("$%1").arg(data_objects.at(i).die_offset, 0, 16)));
		if (!(i % 500))
			qDebug() << "constructing static data objects view:" << data_objects.size() - i << "remaining";
	}
	ui->tableWidgetFunctions->sortItems(0);
	ui->tableWidgetFunctions->resizeColumnsToContents();
	/*! \warning	resizing the rows to fit the contents can be **very** expensive */
	//ui->tableWidgetFunctions->resizeRowsToContents();
	ui->tableWidgetStaticDataObjects->sortItems(0);
	ui->tableWidgetStaticDataObjects->resizeColumnsToContents();
	/*! \warning	resizing the rows to fit the contents can be **very** expensive */
	//ui->tableWidgetStaticDataObjects->resizeRowsToContents();
	profiling.static_storage_duration_display_view_build_time = t.elapsed();
	qDebug() << "static object lists built in" << profiling.static_storage_duration_display_view_build_time << "milliseconds";

	profiling.debugger_startup_time = startup_time.elapsed();
	qDebug() << "debugger startup time:" << profiling.debugger_startup_time << "milliseconds";

	connect(& blackstrike_port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(blackstrikeError(QSerialPort::SerialPortError)));
	
	std::vector<DebugLine::sourceFileNames> sources;
	dwdata->getFileAndDirectoryNamesPointers(sources);
	std::sort(sources.begin(), sources.end(), sortSourcefiles);
	int row;
	for (row = i = 0; i < sources.size(); i ++)
	{
		if (i && !strcmp(sources.at(i).file, sources.at(i - 1).file) && !strcmp(sources.at(i).directory, sources.at(i - 1).directory))
			continue;
		ui->tableWidgetFiles->insertRow(row);
		ui->tableWidgetFiles->setItem(row, 0, new QTableWidgetItem(sources.at(i).file));
		ui->tableWidgetFiles->setItem(row, 1, new QTableWidgetItem(sources.at(i).directory));
		ui->tableWidgetFiles->setItem(row, 2, new QTableWidgetItem(sources.at(i).compilation_directory));
		row ++;
	}
	ui->tableWidgetFiles->sortItems(0);
	
	ui->plainTextEdit->installEventFilter(this);
	targetDisconnected();
	highlighter = new Highlighter(ui->plainTextEdit->document());
	
        connect(& polishing_timer, SIGNAL(timeout()), this, SLOT(polishSourceCodeViewOnTargetExecution()));
        ui->plainTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
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
QTime x;
int i;
int row(ui->tableWidgetBacktrace->currentRow());
if (row < 0)
	return;
int frame_number = ui->tableWidgetBacktrace->verticalHeaderItem(row)->data(Qt::UserRole).toInt();
updateRegisterView(frame_number);
ui->tableWidgetLocalVariables->setRowCount(0);
if (!ui->tableWidgetBacktrace->item(row, 6))
{
	ui->plainTextEdit->setPlainText("singularity; context undefined");
	return;
}
uint32_t cfa_value = (register_cache->frameCount() - 1 > frame_number) ? register_cache->cachedRegisterFrame(frame_number + 1).at(13) : -1;
QString frameBaseSforthCode;
QString locationSforthCode;
uint32_t pc = -1;

	if (!ui->tableWidgetBacktrace->item(row, 0))
	{
		auto source_coordinates = dwdata->sourceCodeCoordinatesForDieOffset(ui->tableWidgetBacktrace->item(row, 6)->text().remove(0, 1).toUInt(0, 16));
		if (source_coordinates.call_line == -1 || !source_coordinates.call_file_name)
			displaySourceCodeFile(source_coordinates.file_name, source_coordinates.directory_name, source_coordinates.compilation_directory_name, source_coordinates.line, source_coordinates.address);
		else
			displaySourceCodeFile(source_coordinates.call_file_name, source_coordinates.call_directory_name, source_coordinates.compilation_directory_name, source_coordinates.call_line, source_coordinates.address);
	}
	else 
	{
		pc = ui->tableWidgetBacktrace->item(row, 0)->text().remove(0, 1).toUInt(0, 16);
		displaySourceCodeFile(ui->tableWidgetBacktrace->item(row, 2)->text(), ui->tableWidgetBacktrace->item(row, 4)->text(), ui->tableWidgetBacktrace->item(row, 5)->text(), ui->tableWidgetBacktrace->item(row, 3)->text().toUInt(), pc);
		frameBaseSforthCode = ui->tableWidgetBacktrace->item(row, 7)->text();
	}

	x.start();
	auto context = dwdata->executionContextForAddress(pc);
	auto locals = dwdata->localDataObjectsForContext(context);

	ui->treeWidgetDataObjects->clear();

	for (i = 0; i < locals.size(); i ++)
	{
		QString data_object_name;
		ui->tableWidgetLocalVariables->insertRow(row = ui->tableWidgetLocalVariables->rowCount());
		ui->tableWidgetLocalVariables->setItem(row, 0, new QTableWidgetItem(data_object_name = QString(dwdata->nameOfDie(locals.at(i)))));
		std::vector<DwarfTypeNode> type_cache;
		dwdata->readType(locals.at(i).offset, type_cache);
		ui->tableWidgetLocalVariables->setItem(row, 1, new QTableWidgetItem(QString("%1").arg(dwdata->sizeOf(type_cache))));
		locationSforthCode = QString::fromStdString(dwdata->locationSforthCode(locals.at(i), context.at(0), pc));
		auto x = dwarf_evaluator->evaluateLocation(cfa_value, frameBaseSforthCode, locationSforthCode);
		if (x.type == DwarfEvaluator::INVALID)
			ui->tableWidgetLocalVariables->setItem(row, 2, new QTableWidgetItem("cannot evaluate"));
		else
		{
			QString prefix;
			int base = 16, width = 0;
			if (x.type == DwarfEvaluator::MEMORY_ADDRESS)
				prefix = "@$", width = 8;
			else if (x.type == DwarfEvaluator::REGISTER_NUMBER)
				prefix = "#r";
			else
				base = 10;
			ui->tableWidgetLocalVariables->setItem(row, 2, new QTableWidgetItem((prefix + "%1").arg(x.value, width, base)));
			
			switch (base = ui->comboBoxDataDisplayNumericBase->currentText().toUInt())
			{
				case 2: prefix = "%"; break;
				case 16: prefix = "$"; break;
				case 10: break;
				default: Util::panic();
			}

			struct DwarfData::DataNode node;
			dwdata->dataForType(type_cache, node, true, 1);
			if (x.type == DwarfEvaluator::MEMORY_ADDRESS)
			{
				auto n = new QTreeWidgetItem(QStringList() << data_object_name);
                                n->addChild(itemForNode(node, target->readBytes(x.value, node.bytesize, true), 0, base, ""));
				ui->treeWidgetDataObjects->addTopLevelItem(n);
			}
			else if (x.type == DwarfEvaluator::REGISTER_NUMBER)
			{
				auto n = new QTreeWidgetItem(QStringList() << data_object_name);
				uint32_t register_contents = register_cache->cachedRegisterFrame(frame_number).at(x.value);
				n->addChild(itemForNode(node, QByteArray((const char *) & register_contents, sizeof register_contents), 0, base, ""));
				ui->treeWidgetDataObjects->addTopLevelItem(n);
			}
		}
		ui->tableWidgetLocalVariables->setItem(row, 3, new QTableWidgetItem(locationSforthCode));
		if (ui->tableWidgetLocalVariables->item(row, 3)->text().isEmpty())
			/* the data object may have been evaluated as a compile-time constant - try that */
			ui->tableWidgetLocalVariables->item(row, 3)->setText(QString::fromStdString(dwdata->locationSforthCode(locals.at(i), context.at(0), pc, DW_AT_const_value)));
		ui->tableWidgetLocalVariables->setItem(row, 4, new QTableWidgetItem(QString("$%1").arg(locals.at(i).offset, 0, 16)));
	}
	ui->tableWidgetLocalVariables->resizeColumnsToContents();
	ui->tableWidgetLocalVariables->resizeRowsToContents();
	ui->treeWidgetDataObjects->expandToDepth(1);
	if (/* this is not exact, which it needs not be */ x.elapsed() > profiling.max_local_data_objects_view_build_time)
		profiling.max_local_data_objects_view_build_time = x.elapsed();
	qDebug() << "local data objects view built in " << x.elapsed() << "milliseconds";
}

void MainWindow::blackstrikeError(QSerialPort::SerialPortError error)
{
	switch (error)
	{
	default:
		Util::panic();
	case QSerialPort::NoError:
	case QSerialPort::TimeoutError:
		break;
	case QSerialPort::PermissionError:
		QMessageBox::critical(0, "error connecting to the blackstrike probe", "error connecting to the blackstrike probe\npermission denied(port already open?)");
		break;
	}
}

void MainWindow::closeEvent(QCloseEvent *e)
{
QSettings s("troll.rc", QSettings::IniFormat);
	s.setValue("window-geometry", saveGeometry());
	s.setValue("window-state", saveState());
	s.setValue("main-splitter/geometry", ui->splitterMain->saveGeometry());
	s.setValue("main-splitter/state", ui->splitterMain->saveState());
	s.setValue("hack-mode", ui->actionHack_mode->isChecked());
	s.setValue("show-disassembly-ranges", ui->actionShow_disassembly_address_ranges->isChecked());
	s.setValue("data-display-numeric-base", ui->comboBoxDataDisplayNumericBase->currentIndex());
	s.setValue("last-elf-file", elf_filename);
	s.setValue("scratchpad-contents", ui->plainTextEditScratchpad->toPlainText());
	QStringList bookmarks;
	int i;
	for (i = 0; i < ui->tableWidgetBookmarks->rowCount(); i ++)
		bookmarks << ui->tableWidgetBookmarks->item(i, 0)->text()
			<< ui->tableWidgetBookmarks->item(i, 1)->text()
			<< ui->tableWidgetBookmarks->item(i, 2)->text()
			<< ui->tableWidgetBookmarks->item(i, 3)->text();
	s.setValue("bookmarks", bookmarks);
	qDebug() << "";
	qDebug() << "";
	qDebug() << "";
	qDebug() << "<<< profiling stats >>>";
	qDebug() << "";
	dwdata->dumpStats();
	qDebug() << "frontend profiling stats (all times in milliseconds):";
	qDebug() << "time for reading all debug sections from disk:" << profiling.debug_sections_disk_read_time;
	qDebug() << "time for processing all of the .debug_info data:" << profiling.all_compilation_units_processing_time;
	qDebug() << "time for processing the whole .debug_lines section:" << profiling.debug_lines_processing_time;
	qDebug() << "time for gathering data on all static storage duration data and subprograms:" << profiling.static_storage_duration_data_reap_time;
	qDebug() << "time for building the static storage duration data and subprograms views:" << profiling.static_storage_duration_display_view_build_time;
	qDebug() << "!!! total debugger startup time:" << profiling.debugger_startup_time;
	qDebug() << "maximum time for retrieving line and addresses for a source code file:" << profiling.max_addresses_for_file_retrieval_time;
	qDebug() << "maximum time for building a source code view:" << profiling.max_source_code_view_build_time;
	qDebug() << "maximum time for building a local data view:" << profiling.max_local_data_objects_view_build_time;
	qDebug() << "maximum time for generating a backtrace:" << profiling.max_backtrace_generation_time;
	qDebug() << "maximum time for generating a context view:" << profiling.max_context_view_generation_time;
	qDebug() << "maximum time for retrieving breakpoint addresses for a source code line(filtered):" << profiling.max_time_for_retrieving_breakpoint_addresses_for_line;
	qDebug() << "maximum time for retrieving breakpoint addresses for a source code line(unfiltered):" << profiling.max_time_for_retrieving_unfiltered_breakpoint_addresses_for_line;
	QMainWindow::closeEvent(e);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
bool result = true;
static unsigned accumulator;
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if (Qt::Key_0 <= keyEvent->key() && keyEvent->key() <= Qt::Key_9)
			accumulator *= 10, accumulator += keyEvent->key() - Qt::Key_0;
		switch (keyEvent->key())
		{
			case Qt::Key_Space:
			{
				bool ok;
				int i;
				QString l = ui->plainTextEdit->textCursor().block().text();
				qDebug() << l;
				QRegExp rx("^\\**\\s*(\\w+)\\|");
				if (rx.indexIn(l) != -1)
				{
					int j;
					i = rx.cap(1).toUInt(& ok);
					if (!ok)
						break;
					qDebug() << "requesting breakpoint for source file" << last_source_filename << "line number" << i;
					QTime t;
					t.start();
					struct SourceLevelBreakpoint b = { .source_filename = last_source_filename, .directory_name = last_directory_name, .compilation_directory = last_compilation_directory, .line_number = i, };
					if ((j = breakpointIndex(b)) == -1)
					{
						auto x = dwdata->filteredAddressesForFileAndLineNumber(last_source_filename.toLocal8Bit().constData(), i);
						qDebug() << "filtered addresses:" << x.size();
						if (x.empty())
							break;
						b.addresses = QVector<uint32_t>::fromStdVector(x);
						breakpoints.push_back(b);
						if (t.elapsed() > profiling.max_time_for_retrieving_breakpoint_addresses_for_line)
							profiling.max_time_for_retrieving_breakpoint_addresses_for_line = t.elapsed();
						t.restart();
						x = dwdata->unfilteredAddressesForFileAndLineNumber(last_source_filename.toLocal8Bit().constData(), i);
						if (t.elapsed() > profiling.max_time_for_retrieving_unfiltered_breakpoint_addresses_for_line)
							profiling.max_time_for_retrieving_unfiltered_breakpoint_addresses_for_line = t.elapsed();
						qDebug() << "total addresses:" << x.size();
					}
					else
						breakpoints.removeAt(j);
				}
				else
				{
					/* internal disassembly range format */
					rx.setPattern("^\\$(\\w+)");
					uint32_t address;
					if (rx.indexIn(l) == -1)
						/* objdump disassembly dump format */
						rx.setPattern("^\\s*(\\w+):");
					if (rx.indexIn(l) != -1 && (address = rx.cap(1).toUInt(& ok, 16), ok))
					{
						if ((i = machineBreakpointIndex(address)) == -1)
						{
							auto x = dwdata->sourceCodeCoordinatesForAddress(address);
							SourceLevelBreakpoint b;
							b.source_filename = QString::fromStdString(x.file_name);
							b.directory_name = QString::fromStdString(x.directory_name);
							b.compilation_directory = QString::fromStdString(x.compilation_directory_name);
							b.line_number = x.line;
							b.addresses.push_back(address);
							machine_level_breakpoints.push_back((struct MachineLevelBreakpoint){ .address = address, .inferred_breakpoint = b, });
						}
						else
							machine_level_breakpoints.removeAt(i);
					}
				}
				updateBreakpoints();
				refreshSourceCodeView(ui->plainTextEdit->textCursor().blockNumber());
			}
				break;
		case Qt::Key_G:
			{
			auto c = ui->plainTextEdit->textCursor();
				c.movePosition(QTextCursor::Start);
				c.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, -- accumulator);
				ui->plainTextEdit->setTextCursor(c);
				ui->plainTextEdit->centerCursor();
				accumulator = 0;
			}
			break;
		case Qt::Key_S:
			if (execution_state == HALTED)
			{
				if (keyEvent->text() == "s")
					on_actionSingle_step_triggered();
				else if (keyEvent->text() == "S")
					on_actionSource_step_triggered();
			}
			break;
		case Qt::Key_C:
			if (execution_state == HALTED && keyEvent->modifiers() == Qt::NoModifier)
				on_actionResume_triggered();
			else if (execution_state == FREE_RUNNING && keyEvent->modifiers() == Qt::ControlModifier)
				on_actionHalt_triggered();
			break;
			default:
				result = false;
		}
	}
	else
		result = false;
	return result;
}

void MainWindow::on_actionSingle_step_triggered()
{
	execution_state = FREE_RUNNING;
	target->requestSingleStep();
}

void MainWindow::on_actionSource_step_triggered()
{
	execution_state = SOURCE_LEVEL_SINGLE_STEPPING;
	target->requestSingleStep();
}

void MainWindow::on_actionBlackstrikeConnect_triggered()
{
auto ports = QSerialPortInfo::availablePorts();
int i;
class Target * t;
	for (i = 0; i < ports.size(); i ++)
	{
		qDebug() << ports[i].manufacturer();
		if (ports.at(i).hasProductIdentifier() && ports.at(i).vendorIdentifier() == BLACKMAGIC_USB_VENDOR_ID)
		{
			blackstrike_port.setPortName(ports.at(i).portName());
			if (blackstrike_port.open(QSerialPort::ReadWrite))
			{
				t = new Blackmagic(& blackstrike_port);
				if (!t->connect())
				{
					delete t;
					t = new Blackstrike(& blackstrike_port);
					if (!t->connect())
					{
						blackstrike_port.close();
						delete t;
						continue;
					}
				}
				cortexm0->setTargetController(target = t);
				connect(target, SIGNAL(targetHalted(TARGET_HALT_REASON)), this, SLOT(targetHalted(TARGET_HALT_REASON)));
				connect(target, SIGNAL(targetRunning()), this, SLOT(targetRunning()));
				targetConnected();
				auto s = target->memoryMap();
				target->parseMemoryAreas(s);
				if (!target->syncFlash(target_memory_contents))
				{
					QMessageBox::critical(0, "memory contents mismatch", "target memory contents mismatch");
					Util::panic();
				}
				else
					QMessageBox::information(0, "memory contents match", "target memory contents match");
				execution_state = HALTED;
				return;
			}
		}
	}
	QMessageBox::warning(0, "blackstrike port not found", "cannot find blackstrike gdbserver port ");
}

void MainWindow::on_actionShell_triggered()
{
	if (ui->tableWidgetBacktrace->selectionModel()->hasSelection())
	{
		QFileInfo f(current_source_code_file_displayed);
		if (f.exists())
			QProcess::startDetached("cmd", QStringList() , QDir::toNativeSeparators(f.canonicalPath()));
	}
}

void MainWindow::on_actionExplore_triggered()
{
	if (ui->tableWidgetBacktrace->selectionModel()->hasSelection())
	{
		QFileInfo f(current_source_code_file_displayed);
		if (f.exists())
			QProcess::startDetached("explorer", QStringList() << QString("/select,") + QDir::toNativeSeparators(f.canonicalFilePath()) , QDir::toNativeSeparators(f.canonicalPath()));
	}
}

void MainWindow::on_tableWidgetStaticDataObjects_itemSelectionChanged()
{
int row(ui->tableWidgetStaticDataObjects->currentRow());
uint32_t die_offset = ui->tableWidgetStaticDataObjects->item(row, 5)->text().replace('$', "0x").toUInt(0, 0);
uint32_t address = ui->tableWidgetStaticDataObjects->item(row, 2)->text().replace('$', "0x").toUInt(0, 0);
QByteArray data;
int numeric_base;
QString numeric_prefix;
	std::vector<struct DwarfTypeNode> type_cache;
	dwdata->readType(die_offset, type_cache);
	
	struct DwarfData::DataNode node;
	dwdata->dataForType(type_cache, node, true, 1);
	ui->treeWidgetDataObjects->clear();
	switch (numeric_base = ui->comboBoxDataDisplayNumericBase->currentText().toUInt())
	{
		case 2: numeric_prefix = "%"; break;
		case 16: numeric_prefix = "$"; break;
		case 10: break;
		default: Util::panic();
	}
	ui->treeWidgetDataObjects->addTopLevelItem(itemForNode(node, data = target->readBytes(address, node.bytesize, true), 0, numeric_base, numeric_prefix));
	ui->treeWidgetDataObjects->expandAll();
	ui->treeWidgetDataObjects->resizeColumnToContents(0);
	dumpData(address, data);

	auto source_coordinates = dwdata->sourceCodeCoordinatesForDieOffset(ui->tableWidgetStaticDataObjects->item(row, 5)->text().remove(0, 1).toUInt(0, 16));
	if (source_coordinates.line != -1)
		displaySourceCodeFile(source_coordinates.file_name, source_coordinates.directory_name, source_coordinates.compilation_directory_name, source_coordinates.line);
}

void MainWindow::on_actionHack_mode_triggered()
{
bool hack_mode(ui->actionHack_mode->isChecked());
	ui->tableWidgetBacktrace->setColumnHidden(4, !hack_mode);
	ui->tableWidgetBacktrace->setColumnHidden(5, !hack_mode);
	ui->tableWidgetBacktrace->setColumnHidden(6, !hack_mode);
	ui->tableWidgetBacktrace->setColumnHidden(7, !hack_mode);
	
	ui->tableWidgetStaticDataObjects->setColumnHidden(3, !hack_mode);
	ui->tableWidgetStaticDataObjects->setColumnHidden(4, !hack_mode);
	ui->tableWidgetStaticDataObjects->setColumnHidden(5, !hack_mode);
	
	ui->treeWidgetDataObjects->setColumnHidden(1, !hack_mode);
	ui->treeWidgetDataObjects->setColumnHidden(3, !hack_mode);
	ui->treeWidgetDataObjects->setColumnHidden(4, !hack_mode);
	
	ui->tableWidgetLocalVariables->setColumnHidden(1, !hack_mode);
	ui->tableWidgetLocalVariables->setColumnHidden(2, !hack_mode);
	ui->tableWidgetLocalVariables->setColumnHidden(3, !hack_mode);
	ui->tableWidgetLocalVariables->setColumnHidden(4, !hack_mode);
	
	ui->tableWidgetFunctions->setColumnHidden(1, !hack_mode);
	ui->tableWidgetFunctions->setColumnHidden(2, !hack_mode);
	ui->tableWidgetFunctions->setColumnHidden(3, !hack_mode);
	
	ui->tableWidgetFiles->setColumnHidden(1, !hack_mode);
	ui->tableWidgetFiles->setColumnHidden(2, !hack_mode);
	
	ui->actionHack_mode->setText(!hack_mode ? "to hack mode" : "to user mode");
}

void MainWindow::on_actionReset_target_triggered()
{
	if (!target->reset())
		Util::panic();
	backtrace();
}

void MainWindow::on_actionCore_dump_triggered()
{
QDir dir;
QFile f;
QDate date(QDate::currentDate());
QTime time(QTime::currentTime());
QString dirname = QString("coredump-%1%2%3-%4%5%6")
			.arg(date.day(), 2, 10, QChar('0'))
			.arg(date.month(), 2, 10, QChar('0'))
			.arg(date.year() % 100, 2, 10, QChar('0'))
			.arg(time.hour(), 2, 10, QChar('0'))
			.arg(time.minute(), 2, 10, QChar('0'))
			.arg(time.second(), 2, 10, QChar('0'));
int i;

	if (!dir.mkdir(dirname))
		Util::panic();
	f.setFileName(dirname + "/flash.bin");
	if (!f.open(QFile::WriteOnly))
		Util::panic();
	f.write(target->readBytes(0x08000000, 0x20000));
	f.close();
	f.setFileName(dirname + "/ram.bin");
	if (!f.open(QFile::WriteOnly))
		Util::panic();
	f.write(target->readBytes(0x20000000, /*0x20000*/0x4000));
	f.close();
	f.setFileName(dirname + "/registers.bin");
	if (!f.open(QFile::WriteOnly))
		Util::panic();
	for (i = 0; i < register_cache->cachedRegisterFrame(0).size(); i ++)
		f.write((const char * ) & register_cache->cachedRegisterFrame(0).at(i), sizeof register_cache->cachedRegisterFrame(0).at(i));
	f.close();
	f.setFileName(elf_filename);
	if (!f.copy(dirname + "/" + QFileInfo(elf_filename).fileName()))
		Util::panic();
}

void MainWindow::on_comboBoxDataDisplayNumericBase_currentIndexChanged(int index)
{
	if (ui->tableWidgetStaticDataObjects->currentRow() >= 0)
		on_tableWidgetStaticDataObjects_itemSelectionChanged();
}

void MainWindow::on_actionResume_triggered()
{
auto breakpointed_addresses = breakpointedAddresses();

	auto x = breakpointed_addresses.begin();
	while (x != breakpointed_addresses.end())
	{
		if (!target->breakpointSet(* x, 2))
		{
			QMessageBox::critical(0, "failed to set breakpoint", "failed to set breakpoint!\ntoo many breakpoints requested?");
			Util::panic();
		}
		x ++;
	}
	execution_state = FREE_RUNNING;
	target->resume();
}

void MainWindow::on_actionHalt_triggered()
{
	target->requestHalt();
}

void MainWindow::on_actionRead_state_triggered()
{
	if (target->haltReason())
		backtrace();
}

void MainWindow::on_tableWidgetFiles_itemSelectionChanged()
{
int row(ui->tableWidgetFiles->currentRow());
	displaySourceCodeFile(ui->tableWidgetFiles->item(row, 0)->text(),
		ui->tableWidgetFiles->item(row, 1)->text(),
		ui->tableWidgetFiles->item(row, 2)->text(), 0);
}

void MainWindow::on_actionShow_disassembly_address_ranges_triggered()
{
	displaySourceCodeFile(last_source_filename, last_directory_name, last_compilation_directory, last_highlighted_line, last_source_highlighted_address);
}

void MainWindow::refreshSourceCodeView(int center_line)
{
auto c = ui->plainTextEdit->textCursor();
	displaySourceCodeFile(last_source_filename, last_directory_name, last_compilation_directory, last_highlighted_line, last_source_highlighted_address); 
	c.movePosition(QTextCursor::Start);
	c.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, center_line);
	ui->plainTextEdit->setTextCursor(c);
	ui->plainTextEdit->centerCursor();
}

void MainWindow::on_tableWidgetLocalVariables_itemSelectionChanged()
{
int row(ui->tableWidgetLocalVariables->currentRow());
	if (row < 0)
		return;
	auto source_coordinates = dwdata->sourceCodeCoordinatesForDieOffset(ui->tableWidgetLocalVariables->item(row, 4)->text().remove(0, 1).toUInt(0, 16));
	if (source_coordinates.line != -1)
		displaySourceCodeFile(source_coordinates.file_name, source_coordinates.directory_name, source_coordinates.compilation_directory_name, source_coordinates.line);
}

void MainWindow::on_tableWidgetFunctions_itemSelectionChanged()
{
int row(ui->tableWidgetFunctions->currentRow());
	if (row < 0)
		return;
	auto source_coordinates = dwdata->sourceCodeCoordinatesForDieOffset(ui->tableWidgetFunctions->item(row, 3)->text().remove(0, 1).toUInt(0, 16));
	if (source_coordinates.line != -1)
		displaySourceCodeFile(source_coordinates.file_name, source_coordinates.directory_name, source_coordinates.compilation_directory_name, source_coordinates.line);
}

void MainWindow::targetHalted(TARGET_HALT_REASON reason)
{
auto breakpointed_addresses = breakpointedAddresses();

	switch (execution_state)
	{
		case SOURCE_LEVEL_SINGLE_STEPPING:
		{
			/*! \todo	this is evil, make this portable */
			auto x = target->readRawUncachedRegister(15) &~ 1;
			bool is_statement;
			dwdata->sourceCodeCoordinatesForAddress(x, & is_statement);
			if (!is_statement)
			{
				on_actionSource_step_triggered();
				return;
			}
		}
		break;
		case FREE_RUNNING:
			break;
	}
	execution_state = HALTED;
	auto x = breakpointed_addresses.begin();
	while (x != breakpointed_addresses.end())
	{
		target->breakpointClear(* x, 2);
		x ++;
	}
	polishing_timer.stop();
	ui->actionBlackstrikeConnect->setEnabled(false);
	ui->actionSingle_step->setEnabled(true);
	ui->actionSource_step->setEnabled(true);
	ui->actionReset_target->setEnabled(true);
	ui->actionResume->setEnabled(true);
	ui->actionHalt->setEnabled(false);
	ui->actionRead_state->setEnabled(true);
	ui->actionCore_dump->setEnabled(true);
	backtrace();
}

void MainWindow::targetDisconnected()
{
	ui->actionBlackstrikeConnect->setEnabled(true);
	ui->actionSingle_step->setEnabled(false);
	ui->actionSource_step->setEnabled(false);
	ui->actionReset_target->setEnabled(false);
	ui->actionResume->setEnabled(false);
	ui->actionHalt->setEnabled(false);
	ui->actionRead_state->setEnabled(false);
	ui->actionCore_dump->setEnabled(false);
}

void MainWindow::targetConnected()
{
	ui->actionBlackstrikeConnect->setEnabled(false);
	ui->actionSingle_step->setEnabled(true);
	ui->actionSource_step->setEnabled(true);
	ui->actionReset_target->setEnabled(true);
	ui->actionResume->setEnabled(true);
	ui->actionHalt->setEnabled(true);
	ui->actionRead_state->setEnabled(true);
	ui->actionCore_dump->setEnabled(true);

	backtrace();
}

void MainWindow::polishSourceCodeViewOnTargetExecution()
{
static int i;

	if (i ++ == 10)
		i = 0;
	polishing_timer.setInterval(200);
	ui->plainTextEdit->setPlainText(QString("target running...") + QString(i, QChar('.')));;
	/*! \todo	WARNING - it seems that I am doing something wrong with the 'readyRead()' signal;
	 *		without the call to 'waitForReadyRead()' below, on one machine that I am testing on,
	 *		incoming data from the blackmagic probe sometimes does not cause the 'readyRead()'
	 *		signal to be emitted, which breaks the code badly; I discovered through trial and
	 *		error that the call to 'WaitForReadyRead()' function below seems to work around this issue */
	blackstrike_port.waitForReadyRead(1);
}

void MainWindow::targetRunning()
{
	ui->actionBlackstrikeConnect->setEnabled(false);
	ui->actionSingle_step->setEnabled(false);
	ui->actionSource_step->setEnabled(false);
	ui->actionReset_target->setEnabled(false);
	ui->actionResume->setEnabled(false);
	ui->actionHalt->setEnabled(true);
	ui->actionRead_state->setEnabled(false);
	ui->actionCore_dump->setEnabled(false);
	polishing_timer.start(500);
	ui->tableWidgetBacktrace->setRowCount(0);
	ui->tableWidgetLocalVariables->setRowCount(0);
}

void MainWindow::on_treeWidgetDataObjects_itemActivated(QTreeWidgetItem *item, int column)
{
	if (item->data(0, Qt::UserRole).canConvert<TreeWidgetNodeData>() && !item->childCount())
	{
		QMessageBox::information(0, "deref", "deref requested");
		bool ok;
		uint32_t address = item->text(2).replace('$', "0x").toUInt(& ok, 0);

		std::vector<struct DwarfTypeNode> type_cache;
		dwdata->readType(item->data(0, Qt::UserRole).value<struct TreeWidgetNodeData>().pointer_type_die_offset, type_cache);

		struct DwarfData::DataNode node;
		dwdata->dataForType(type_cache, node, true, 1);
		item->addChild(itemForNode(node, ok ? target->readBytes(address, node.bytesize, true) : QByteArray(), 0, 10, ""));
		item->setExpanded(true);
	}
}

void MainWindow::on_lineEditStaticDataObjects_textChanged(const QString &arg1)
{
	auto x = ui->tableWidgetStaticDataObjects->findItems(arg1, Qt::MatchStartsWith);
	if (x.isEmpty())
		ui->tableWidgetStaticDataObjects->scrollToTop();
	else
		ui->tableWidgetStaticDataObjects->scrollToItem(x.at(0), QAbstractItemView::PositionAtTop);
}

void MainWindow::on_lineEditSubprograms_textChanged(const QString &arg1)
{
	auto x = ui->tableWidgetFunctions->findItems(arg1, Qt::MatchStartsWith);
	if (x.isEmpty())
		ui->tableWidgetFunctions->scrollToTop();
	else
		ui->tableWidgetFunctions->scrollToItem(x.at(0), QAbstractItemView::PositionAtTop);
}

void MainWindow::on_lineEditStaticDataObjects_returnPressed()
{
	auto x = ui->tableWidgetStaticDataObjects->findItems(ui->lineEditStaticDataObjects->text(), Qt::MatchStartsWith);
	if (!x.isEmpty())
	{
		ui->tableWidgetStaticDataObjects->scrollToItem(x.at(0), QAbstractItemView::PositionAtTop);
		ui->tableWidgetStaticDataObjects->selectRow(x[0]->row());
	}
}

void MainWindow::on_lineEditSubprograms_returnPressed()
{
	auto x = ui->tableWidgetFunctions->findItems(ui->lineEditSubprograms->text(), Qt::MatchStartsWith);
	if (!x.isEmpty())
	{
		ui->tableWidgetFunctions->scrollToItem(x.at(0), QAbstractItemView::PositionAtTop);
		ui->tableWidgetFunctions->selectRow(x[0]->row());
	}
}

void MainWindow::on_pushButtonCreateBookmark_clicked()
{
auto row = ui->tableWidgetBookmarks->rowCount();
QRegExp rx("^\\**\\s*(\\w+)\\|");
QTextCursor c = ui->plainTextEdit->textCursor();
int i, line_number = -1;

	for (i = c.blockNumber(); i >= 0; i --)
	{
		bool ok;
		auto x = c.block().text();
		if (rx.indexIn(x) != -1 && (line_number = rx.cap(1).toInt(& ok), ok))
			break;
		c.movePosition(QTextCursor::PreviousBlock);
	}
	if (line_number == -1)
		return;

	ui->tableWidgetBookmarks->insertRow(row);
	ui->tableWidgetBookmarks->setItem(row, 0, new QTableWidgetItem(last_source_filename));
	ui->tableWidgetBookmarks->setItem(row, 1, new QTableWidgetItem(QString("%1").arg(line_number)));
	ui->tableWidgetBookmarks->setItem(row, 2, new QTableWidgetItem(last_directory_name));
	ui->tableWidgetBookmarks->setItem(row, 3, new QTableWidgetItem(last_compilation_directory));
}

void MainWindow::on_tableWidgetBookmarks_doubleClicked(const QModelIndex &index)
{
	displaySourceCodeFile(ui->tableWidgetBookmarks->item(index.row(), 0)->text(),
	                      ui->tableWidgetBookmarks->item(index.row(), 2)->text(),
	                      ui->tableWidgetBookmarks->item(index.row(), 3)->text(),
	                      ui->tableWidgetBookmarks->item(index.row(), 1)->text().toUInt()
	                      );
}

void MainWindow::on_pushButtonRemoveBookmark_clicked()
{
auto row = ui->tableWidgetBookmarks->currentRow();
	if (row >= 0)
	{
		ui->tableWidgetBookmarks->removeRow(row);
		ui->tableWidgetBookmarks->setCurrentCell(-1, -1);
	}
}

void MainWindow::on_pushButtonMoveBookmarkUp_clicked()
{
auto row = ui->tableWidgetBookmarks->currentRow();
	if (row > 0)
	{
		QStringList x;
		x << ui->tableWidgetBookmarks->item(row, 0)->text()
		  << ui->tableWidgetBookmarks->item(row, 1)->text()
		  << ui->tableWidgetBookmarks->item(row, 2)->text()
		  << ui->tableWidgetBookmarks->item(row, 3)->text();
		ui->tableWidgetBookmarks->removeRow(row);
		ui->tableWidgetBookmarks->insertRow(-- row);
		ui->tableWidgetBookmarks->setItem(row, 0, new QTableWidgetItem(x[0]));
		ui->tableWidgetBookmarks->setItem(row, 1, new QTableWidgetItem(x[1]));
		ui->tableWidgetBookmarks->setItem(row, 2, new QTableWidgetItem(x[2]));
		ui->tableWidgetBookmarks->setItem(row, 3, new QTableWidgetItem(x[3]));
		ui->tableWidgetBookmarks->selectRow(row);
	}
}

void MainWindow::on_pushButtonMoveBookmarkDown_clicked()
{
auto row = ui->tableWidgetBookmarks->currentRow();
	if (row >= 0 && row < ui->tableWidgetBookmarks->rowCount() - 1)
	{
		QStringList x;
		x << ui->tableWidgetBookmarks->item(row, 0)->text()
		  << ui->tableWidgetBookmarks->item(row, 1)->text()
		  << ui->tableWidgetBookmarks->item(row, 2)->text()
		  << ui->tableWidgetBookmarks->item(row, 3)->text();
		ui->tableWidgetBookmarks->removeRow(row);
		ui->tableWidgetBookmarks->insertRow(++ row);
		ui->tableWidgetBookmarks->setItem(row, 0, new QTableWidgetItem(x[0]));
		ui->tableWidgetBookmarks->setItem(row, 1, new QTableWidgetItem(x[1]));
		ui->tableWidgetBookmarks->setItem(row, 2, new QTableWidgetItem(x[2]));
		ui->tableWidgetBookmarks->setItem(row, 3, new QTableWidgetItem(x[3]));
		ui->tableWidgetBookmarks->selectRow(row);
	}
}

void MainWindow::on_actionView_windows_triggered()
{
	createPopupMenu()->popup(QCursor::pos());
}
