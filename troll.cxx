/*
Copyright (c) 2016-2017, 2019 Stoyan Shopov

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

QPlainTextEdit * MainWindow::sforth_console;

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
	//return;
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

int MainWindow::buildArrayViewNode(QTreeWidgetItem *parent, const DwarfData::DataNode &array_node, int dimension_index, const QByteArray& hexAsciiData, int data_pos, int numeric_base, const QString numeric_prefix)
{
int i;
	if (dimension_index == array_node.array_dimensions.size() - 1)
	{
		/* last dimension - process array elements */
		QTreeWidgetItem * n;
		for (i = 0; i < (signed) array_node.array_dimensions.at(dimension_index); i ++)
		{
			parent->addChild(n = itemForNode(array_node.children.at(0),  hexAsciiData, data_pos + i * array_node.children.at(0).bytesize, numeric_base, numeric_prefix));
			n->setText(0, QString("[%1] ").arg(i) + n->text(0));
		}
		return i * array_node.children.at(0).bytesize;
	}
	else
	{
		int size;
		for (i = 0; i < array_node.array_dimensions.at(dimension_index); i ++)
			data_pos += (size = buildArrayViewNode(new QTreeWidgetItem(parent, QStringList() << QString("[%1]").arg(i)), array_node, dimension_index + 1,  hexAsciiData, data_pos, numeric_base, numeric_prefix));
		return size * i;
	}
}

QTreeWidgetItem * MainWindow::itemForNode(const DwarfData::DataNode &node, const QByteArray& hexAsciiData, int data_pos, int numeric_base, const QString & numeric_prefix)
{
auto n = new QTreeWidgetItem(QStringList() << QString::fromStdString(node.data.at(0)) << QString("%1").arg(node.bytesize) << "???" << QString("%1").arg(node.data_member_location));
int i;
QByteArray data;

	if (node.is_pointer)
		n->setData(0, Qt::UserRole, QVariant::fromValue((TreeWidgetNodeData) { .pointer_type_die_offset = node.die_offset, }));
	if (!node.children.size())
	{
		if (data_pos + node.bytesize <= hexAsciiData.size() / /* Two bytes of hex ascii data */ 2) switch (node.bytesize)
		{
		uint64_t x;
			case 1: x = * (uint8_t *) QByteArray::fromHex(data = hexAsciiData.mid(data_pos * 2, 1 * 2)).data(); if (0)
			case 2: x = * (uint16_t *) QByteArray::fromHex(data = hexAsciiData.mid(data_pos * 2, 2 * 2)).data(); if (0)
			case 4: x = * (uint32_t *) QByteArray::fromHex(data = hexAsciiData.mid(data_pos * 2, 4 * 2)).data(); if (0)
			case 8: x = * (uint64_t *) QByteArray::fromHex(data = hexAsciiData.mid(data_pos * 2, 8 * 2)).data();
				/* Check if the data is valid/available */
				if (data.contains('?'))
				{
						n->setText(2, "Data unavailable");
						break;
				}
				if (node.bitsize)
				{
					x >>= node.bitposition, x &= (1 << node.bitsize) - 1;
					n->setText(1, QString("%1 bit").arg(node.bitsize) + ((node.bitsize != 1) ? "s":""));
				}
				n->setText(2, node.is_pointer ? QString("$%1").arg(x, 8, 16, QChar('0')) : numeric_prefix + QString("%1").arg(x, 0, numeric_base));
				if (node.is_enumeration)
					n->setText(2, n->text(2) + " (" + QString::fromStdString(dwdata->enumeratorNameForValue(x, node.die_offset) + ")"));
				switch (node.base_type_encoding)
				{
				case DW_ATE_float:
					switch (node.bytesize)
					{
					case 4:
						n->setText(2, QString("%1").arg(* (float *) & x));
						break;
					case 8:
						n->setText(2, QString("%1").arg(* (double*) & x));
						break;
					default:
						n->setText(2, "UNKNOWN FLOATING POINT REPRESENTATION");
						break;
					}
					break;
				case DW_ATE_unsigned_char:
					n->setText(2, QString("'%1' (%2)").arg(* (char *) & x).arg((unsigned) * (unsigned char *) & x));
					break;
				case DW_ATE_signed_char:
					n->setText(2, QString("'%1' (%2)").arg(* (char *) & x).arg((signed) * (char *) & x));
					break;
				case DW_ATE_unsigned:
					switch (node.bytesize)
					{
					case 1:
						n->setText(2, QString("%1").arg((unsigned)* (uint8_t *) & x));
						break;
					case 2:
						n->setText(2, QString("%1").arg(* (uint16_t *) & x));
						break;
					case 4:
						n->setText(2, QString("%1").arg(* (uint32_t *) & x));
						break;
					case 8:
						n->setText(2, QString("%1").arg(* (uint64_t *) & x));
						break;
					default:
						n->setText(2, "UNKNOWN UNSIGNED INTEGER REPRESENTATION");
						break;
					}
					break;
				case DW_ATE_signed:
					switch (node.bytesize)
					{
					case 1:
						n->setText(2, QString("%1").arg((signed)* (int8_t *) & x));
						break;
					case 2:
						n->setText(2, QString("%1").arg(* (int16_t *) & x));
						break;
					case 4:
						n->setText(2, QString("%1").arg(* (int32_t *) & x));
						break;
					case 8:
						n->setText(2, QString("%1").arg(* (int64_t *) & x));
						break;
					default:
						n->setText(2, "UNKNOWN SIGNED INTEGER REPRESENTATION");
						break;
					}
					break;
				}
				break;
			default:
n->setText(2, "<<< UNKNOWN SIZE >>>");
break;
				Util::panic();
		}
		else
			n->setText(2, "DATA UNAVAILABLE");
	}
	if (node.array_dimensions.size())
		buildArrayViewNode(n, node, 0, hexAsciiData, data_pos, numeric_base, numeric_prefix);
	else
		for (i = 0; i < node.children.size(); n->addChild(itemForNode(node.children.at(i), hexAsciiData, data_pos + node.children.at(i).data_member_location, numeric_base, numeric_prefix)), i ++);
	return n;
}

void MainWindow::colorizeSourceCodeView(void)
{
#if 1
QVector<int> enabled_breakpoint_positions, disabled_breakpoint_positions;
QTextBlockFormat f;
int i, x;
QList<QTextEdit::ExtraSelection> selections;
QTextEdit::ExtraSelection sel;
QTextCharFormat cf;

	ui->plainTextEdit->setExtraSelections(selections);
	QSet<struct BreakpointCache::SourceCodeBreakpoint>::const_iterator sset = breakpoints.enabledSourceCodeBreakpoints.constBegin();
	while (sset != breakpoints.enabledSourceCodeBreakpoints.constEnd())
	{
		if ((x = (* sset).breakpointedLineNumberForSourceCode(last_source_filename, last_directory_name, last_compilation_directory)) != -1)
			if (src.line_positions_in_document.contains(x))
				enabled_breakpoint_positions << src.line_positions_in_document[x];
		sset ++;
	}
	sset = breakpoints.disabledSourceCodeBreakpoints.constBegin();
	while (sset != breakpoints.disabledSourceCodeBreakpoints.constEnd())
	{
		if ((x = (* sset).breakpointedLineNumberForSourceCode(last_source_filename, last_directory_name, last_compilation_directory)) != -1)
			if (src.line_positions_in_document.contains(x))
				disabled_breakpoint_positions << src.line_positions_in_document[x];
		sset ++;
	}
	QSet<uint32_t>::const_iterator bset = breakpoints.enabledMachineAddressBreakpoints.constBegin();
	while (bset != breakpoints.enabledMachineAddressBreakpoints.constEnd())
	{
		if (src.address_positions_in_document.contains(* bset))
			enabled_breakpoint_positions.push_back(src.address_positions_in_document.operator [](*bset));
		bset ++;
	}
	bset = breakpoints.disabledMachineAddressBreakpoints.constBegin();
	while (bset != breakpoints.disabledMachineAddressBreakpoints.constEnd())
	{
		if (src.address_positions_in_document.contains(* bset))
			disabled_breakpoint_positions.push_back(src.address_positions_in_document.operator [](*bset));
		bset ++;
	}

	QTextCursor c(ui->plainTextEdit->textCursor());
	f.setBackground(QBrush(Qt::red));
	cf.setForeground(QBrush(Qt::white));
	cf.setBackground(QBrush(Qt::red));
	for (i = 0; i < enabled_breakpoint_positions.size(); i ++)
	{
		c.setPosition(enabled_breakpoint_positions.at(i));
		c.select(QTextCursor::LineUnderCursor);
		sel.cursor = c;
		sel.format = cf;
		selections << sel;
		//c.setBlockFormat(f);
	}
	f.setBackground(QBrush(Qt::yellow));
	cf.setForeground(QBrush(Qt::red));
	cf.setBackground(QBrush(Qt::yellow));
	for (i = 0; i < disabled_breakpoint_positions.size(); i ++)
	{
		c.setPosition(disabled_breakpoint_positions.at(i));
		c.select(QTextCursor::LineUnderCursor);
		sel.cursor = c;
		sel.format = cf;
		selections << sel;
		//c.setBlockFormat(f);
	}
	ui->plainTextEdit->setExtraSelections(selections);

#endif
}

static bool sortSourcefiles(const struct DebugLine::sourceFileNames & a, const struct DebugLine::sourceFileNames & b)
{
	auto x = strcmp(a.file, b.file);
	if (x)
		return x < 0;
	return strcmp(a.directory, b.directory) < 0;
}

void MainWindow::populateSourceFilesView(bool show_only_files_with_generated_machine_code)
{
	ui->tableWidgetFiles->blockSignals(true);
	ui->tableWidgetFiles->setRowCount(0);
	std::vector<DebugLine::sourceFileNames> sources;
	dwdata->getFileAndDirectoryNamesPointers(sources);
	std::sort(sources.begin(), sources.end(), sortSourcefiles);
	int row, i;
	for (row = i = 0; i < sources.size(); i ++)
	{
		if (i && !strcmp(sources.at(i).file, sources.at(i - 1).file) && !strcmp(sources.at(i).directory, sources.at(i - 1).directory))
			continue;
		if (show_only_files_with_generated_machine_code)
		{
			std::vector<struct DebugLine::lineAddress> line_addresses;
			dwdata->addressesForFile(sources.at(i).file, line_addresses);
			if (!line_addresses.size())
				continue;
		}
		ui->tableWidgetFiles->insertRow(row);
		ui->tableWidgetFiles->setItem(row, 0, new QTableWidgetItem(sources.at(i).file));
		ui->tableWidgetFiles->setItem(row, 1, new QTableWidgetItem(sources.at(i).directory));
		ui->tableWidgetFiles->setItem(row, 2, new QTableWidgetItem(sources.at(i).compilation_directory));
		row ++;
	}
	ui->tableWidgetFiles->sortItems(0);
	ui->tableWidgetFiles->blockSignals(false);
}

void MainWindow::showDisassembly()
{
	int line_in_disassembly, cursor_position_for_line = -1, i;
	ui->plainTextEdit->clear();
	QString t;
	auto x = disassembly->disassemblyAroundAddress(register_cache.readCachedRegister(15), target, & line_in_disassembly);
	src.address_positions_in_document.clear();
	src.line_positions_in_document.clear();
	for (i = 0; i < x.size(); i ++)
	{
		src.address_positions_in_document.insert(x.at(i).first, t.length());
		if (i == line_in_disassembly)
			cursor_position_for_line = t.length();
		t += QString(x.at(i).second).replace('\r', "") + "\n";
	}
	ui->plainTextEdit->setPlainText(t);
	QTextCursor c(ui->plainTextEdit->textCursor());
	c.setPosition(cursor_position_for_line);
	QTextBlockFormat f;
	QTextCharFormat cf, original_format;
	f.setBackground(QBrush(Qt::cyan));
	cf.setForeground(QBrush(Qt::black));
	c.select(QTextCursor::LineUnderCursor);
	c.setBlockFormat(f);
	c.setCharFormat(cf);
	c.clearSelection();
	ui->plainTextEdit->setTextCursor(c);
	c.setCharFormat(original_format);
	ui->plainTextEdit->setTextCursor(c);
	ui->plainTextEdit->centerCursor();

	colorizeSourceCodeView();
}

void MainWindow::attachBlackmagicProbe(Target *blackmagic)
{
	delete target;
	cortexm0->setTargetController(target = blackmagic);
	connect(target, SIGNAL(targetHalted(TARGET_HALT_REASON)), this, SLOT(targetHalted(TARGET_HALT_REASON)));
	connect(target, SIGNAL(targetRunning()), this, SLOT(targetRunning()));
	targetConnected();
	execution_state = HALTED;
	
}

void MainWindow::detachBlackmagicProbe()
{
	polishing_timer.stop();
	blackstrike_port.close();
	delete target;
	target = new TargetCorefile("flash.bin", 0x08000000, "ram.bin", 0x20000000, "registers.bin");
	cortexm0->setTargetController(target);
	targetDisconnected();
	execution_state = INVALID_EXECUTION_STATE;
}

void MainWindow::displayVerboseDataTypeForDieOffset(uint32_t die_offset)
{
	ui->lineEditDieOffsetForVerboseTypeDisplay->setText(QString("$%1").arg(die_offset, 0, 16));
	std::vector<struct DwarfTypeNode> type_cache;
	dwdata->readType(die_offset, type_cache);
	struct DwarfData::TypePrintFlags flags;
	flags.verbose_printing = true;
	flags.discard_typedefs = ui->checkBoxDiscardTypedefSpecifiers->isChecked();
	int start_node = 0;
	ui->plainTextEditVerboseDataType->clear();
	if (type_cache[0].die.isDataObject())
	{
		ui->plainTextEditVerboseDataType->appendPlainText(QString("data object: %1\n------------------------------\n").arg(dwdata->nameOfDie(type_cache[0].die)));
		start_node = 1;
	}
	ui->plainTextEditVerboseDataType->appendPlainText(QString("%1\n------------------------------\nverbose data type:\n")
		       .arg(QString::fromStdString(dwdata->typeString(type_cache, start_node))));
	ui->plainTextEditVerboseDataType->appendPlainText(QString::fromStdString(dwdata->typeString(type_cache, start_node, flags)));
}

void MainWindow::populateFunctionsListView(bool merge_duplicates)
{
	SourceCodeCoordinates c;
	ui->tableWidgetFunctions->clearContents();
	ui->tableWidgetFunctions->setRowCount(0);
	if (merge_duplicates && subprograms.size())
		c = dwdata->sourceCodeCoordinatesForDieOffset(subprograms.at(0).die_offset);
	std::sort(subprograms.begin(), subprograms.end(), [](const struct StaticObject & a, const struct StaticObject & b) -> bool
	        //{ return (strcmp(a.name, b.name) == -1) ? true : false; });
	        { return QString(a.name).toLower() < QString(b.name).toLower(); });
	for (int i = 0; i < subprograms.size(); i++)
	{
		int row(ui->tableWidgetFunctions->rowCount());
		if (merge_duplicates && i)
		{
			/* check for duplicate entries */
			SourceCodeCoordinates n = dwdata->sourceCodeCoordinatesForDieOffset(subprograms.at(i).die_offset);;
			bool skip = (!strcmp(c.file_name, n.file_name) && !strcmp(c.directory_name, n.directory_name) && c.line == n.line
			             && !strcmp(subprograms.at(i - 1).name, subprograms.at(i).name));
			c = n;
			if (skip)
				continue;
		}
		ui->tableWidgetFunctions->insertRow(row);
		ui->tableWidgetFunctions->setItem(row, 0, new QTableWidgetItem(subprograms.at(i).name));
		ui->tableWidgetFunctions->setItem(row, 1, new QTableWidgetItem(QString("%1").arg(subprograms.at(i).file)));
		ui->tableWidgetFunctions->setItem(row, 2, new QTableWidgetItem(QString("%1").arg(subprograms.at(i).line)));
		ui->tableWidgetFunctions->setItem(row, 3, new QTableWidgetItem(QString("$%1").arg(subprograms.at(i).die_offset, 0, 16)));
		if (!(i % 5000))
			qDebug() << "constructing static subprograms view:" << subprograms.size() - i << "remaining";
	}
	/*! \warning	resizing the rows to fit the contents can be **very** expensive */
	ui->tableWidgetFunctions->resizeColumnsToContents();
}

void MainWindow::searchSourceView(const QString & search_pattern)
{
	if (search_pattern.isEmpty())
		return;
	auto c = ui->plainTextEdit->textCursor();
	int x = c.position();

	/* avoid matches starting at the current cursor position */
	c.movePosition(QTextCursor::Right);
	c = ui->plainTextEdit->document()->find(search_pattern, c, QTextDocument::FindWholeWords);
	if (!c.isNull())
		/*c.setPosition(c.anchor()), */ui->plainTextEdit->setTextCursor(c), ui->plainTextEdit->centerCursor();
	else
	{
		c.movePosition(QTextCursor::Start);
		c = ui->plainTextEdit->document()->find(search_pattern, c);
		if (!c.isNull() && c.anchor() != x)
			/*c.setPosition(c.anchor()), */ui->plainTextEdit->setTextCursor(c), ui->plainTextEdit->centerCursor();
	}
	ui->plainTextEdit->setFocus();
	last_search_pattern = search_pattern;
}

void MainWindow::displaySourceCodeFile(QString source_filename, QString directory_name, QString compilation_directory, int highlighted_line, uint32_t address)
{
	setWindowTitle(QString("troll debugger    File: [%1]").arg(source_filename));
	QString adjusted_filename = source_filename;
	adjusted_filename.replace(QChar('\\'), QChar('/'));
        directory_name.replace(QChar('\\'), QChar('/'));
        compilation_directory.replace(QChar('\\'), QChar('/'));
        if (TEST_DRIVE_MODE)
	{
		QRegExp rx("^[xX]:[/\\\\]");
		adjusted_filename.replace(rx, "troll-test-drive-files/"), directory_name.replace(rx, "troll-test-drive-files/"), compilation_directory.replace(rx, "troll-test-drive-files/");
        }

QTime stime;
stime.start();
QFile source_file;
QString t;
QTextBlockFormat f;
QTextCharFormat cf;
QTime x;
int i, cursor_position_for_line(0);
QFileInfo finfo(directory_name + "/" + adjusted_filename);
std::vector<struct DebugLine::lineAddress> line_addresses;
std::map<uint32_t, struct DebugLine::lineAddress *> line_indices;

	src.address_positions_in_document.clear();
	src.line_positions_in_document.clear();

	if (!finfo.exists())
		finfo.setFile(compilation_directory + "/" + adjusted_filename);
	if (!finfo.exists())
		finfo.setFile(compilation_directory + "/" + directory_name + "/" + adjusted_filename);
	if (!finfo.exists())
		finfo.setFile(adjusted_filename);
	ui->plainTextEdit->clear();
	source_file.setFileName(finfo.canonicalFilePath());
	
	x.start();
	dwdata->addressRangesForFile(source_filename.toLocal8Bit().constData(), line_addresses);
	//dwdata->addressesForFile(source_filename.toLocal8Bit().constData(), line_addresses);
	if (/* this is not exact, which it needs not be */ x.elapsed() > profiling.max_addresses_for_file_retrieval_time)
		profiling.max_addresses_for_file_retrieval_time = x.elapsed();
	qDebug() << "addresses for file retrieved in " << x.elapsed() << "milliseconds";
	qDebug() << "addresses for file count: " << line_addresses.size();
	x.restart();
	
	for (i = line_addresses.size() - 1; i >= 0; i --)
	{
		line_addresses.at(i).next = line_indices[line_addresses.at(i).line];
		line_indices[line_addresses.at(i).line] = & line_addresses.at(i);
	}

	if (source_file.open(QFile::ReadOnly))
	{
		int i(1);
		struct DebugLine::lineAddress * dis;
		while (!source_file.atEnd())
		{
			src.line_positions_in_document[i] = t.length();
			if (i == highlighted_line)
				cursor_position_for_line = t.length();
			t += QString("%2 %1|").arg(line_indices[i] ? '*' : ' ')
					.arg(i, 0, 10, QChar(' ')) + source_file.readLine().replace('\t', "        ").replace('\r', "");
			if (ui->actionShow_disassembly_address_ranges->isChecked())
			{
				if (0 && address == -1)
					Util::panic();
				dis = line_indices[i];
				while (dis)
				{
					//t += QString("$%1 - $%2\n").arg(dis->address, 0, 16).arg(dis->address_span, 0, 16), dis = dis->next;
					auto x = disassembly->disassemblyForRange(dis->address, dis->address_span);
					int i;
					for (i = 0; i < x.size(); i ++)
					{
						src.address_positions_in_document.insert(x.at(i).first, t.length());
						if (address == x.at(i).first)
							cursor_position_for_line = t.length();
						t += QString(x.at(i).second).replace('\r', "") + "\n";
					}
					/* Merge successive disassembly ranges */
					if (dis->next && dis->next->address != dis->address_span)
						t += "...\n";
					dis = dis->next;
				}
			}
			i ++;
		}
	}
	else
	{
		statusBar()->showMessage(QString("cannot open source code file ") + source_file.fileName());
		showDisassembly();
		return;
	}
	
	ui->plainTextEdit->appendPlainText(t);

	QTextCursor c(ui->plainTextEdit->textCursor());
	c.movePosition(QTextCursor::Start);
	c.setPosition(cursor_position_for_line);
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
	statusBar()->showMessage(last_source_filename);
	
	colorizeSourceCodeView();
}

void MainWindow::backtrace()
{
	/*! \todo	Test the case of missing debug information, and only dwarf unwinding information
	 * 		(i.e., only '.debug_frame' section) present. You may strip all but the '.debug_frame'
	 * 		ELF section from an executable ELF file for testing. */
	QTime t;
	struct Die call_site;
	t.start();
	cortexm0->primeUnwinder();
	register_cache.clear();
	register_cache.pushFrame(cortexm0->getRegisters());
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
		row = ui->tableWidgetBacktrace->rowCount();
		if (row) ui->tableWidgetBacktrace->setItem(row - 1, 8, new QTableWidgetItem(dwdata->callSiteAtAddress(cortexm0->programCounter(), call_site) ? "yes" : "no"));
		if (DEBUG_BACKTRACE) qDebug() << x.file_name << (signed) x.line;
		if (DEBUG_BACKTRACE) qDebug() << "dwarf unwind program:" << QString::fromStdString(unwind_data.first) << "address:" << unwind_data.second;

		if (DEBUG_BACKTRACE) qDebug() << cortexm0->programCounter() << QString(dwdata->nameOfDie(subprogram));
		ui->tableWidgetBacktrace->insertRow(row);
		ui->tableWidgetBacktrace->setItem(row, 0, new QTableWidgetItem(QString("$%1").arg(cortexm0->programCounter(), 8, 16, QChar('0'))));
		ui->tableWidgetBacktrace->setVerticalHeaderItem(row, new QTableWidgetItem(QString("%1").arg(register_cache.frameCount())));
		ui->tableWidgetBacktrace->verticalHeaderItem(row)->setData(Qt::UserRole, register_cache.frameCount() - 1);
		ui->tableWidgetBacktrace->setItem(row, 1, new QTableWidgetItem(QString(dwdata->nameOfDie(subprogram))));
		ui->tableWidgetBacktrace->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(x.file_name)));
		ui->tableWidgetBacktrace->setItem(row, 3, new QTableWidgetItem(QString("%1").arg(x.line)));
		ui->tableWidgetBacktrace->setItem(row, 4, new QTableWidgetItem(x.directory_name));
		ui->tableWidgetBacktrace->setItem(row, 5, new QTableWidgetItem(x.compilation_directory_name));
		ui->tableWidgetBacktrace->setItem(row, 6, new QTableWidgetItem(QString("$%1").arg(subprogram.offset, 0, 16)));
		ui->tableWidgetBacktrace->setItem(row, 7, new QTableWidgetItem(QString::fromStdString(dwdata->sforthCodeFrameBaseForContext(context, last_pc))));
		
		int i;
		auto inlining_chain = dwdata->inliningChainOfContext(context);
		for (i = inlining_chain.size() - 1; i >= 0; i --)
		{
			ui->tableWidgetBacktrace->insertRow(row = ui->tableWidgetBacktrace->rowCount());
			ui->tableWidgetBacktrace->setVerticalHeaderItem(row, new QTableWidgetItem("inlined"));
			ui->tableWidgetBacktrace->verticalHeaderItem(row)->setData(Qt::UserRole, register_cache.frameCount() - 1);
			ui->tableWidgetBacktrace->setItem(row, 1, new QTableWidgetItem(dwdata->nameOfDie(inlining_chain.at(i))));
			ui->tableWidgetBacktrace->setItem(row, 6, new QTableWidgetItem(QString("$%1").arg(inlining_chain.at(i).offset, 0, 16)));
		}
		
		if (cortexm0->unwindFrame(QString::fromStdString(unwind_data.first), unwind_data.second, cortexm0->programCounter()))
			context = dwdata->executionContextForAddress(cortexm0->programCounter()), register_cache.pushFrame(cortexm0->getRegisters());
		if (context.empty() && cortexm0->architecturalUnwind())
		{
			context = dwdata->executionContextForAddress(cortexm0->programCounter());
			if (!context.empty())
			{
				if (DEBUG_BACKTRACE) qDebug() << "architecture-specific unwinding performed";
				register_cache.pushFrame(cortexm0->getRegisters());
				ui->tableWidgetBacktrace->insertRow(row = ui->tableWidgetBacktrace->rowCount());
				ui->tableWidgetBacktrace->setItem(row, 1, new QTableWidgetItem("architecture-specific unwinding performed"));
				ui->tableWidgetBacktrace->setVerticalHeaderItem(row, new QTableWidgetItem(QString("%1 (singularity)").arg(register_cache.frameCount() - 1)));
				ui->tableWidgetBacktrace->verticalHeaderItem(row)->setData(Qt::UserRole, register_cache.frameCount() - 2);
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
	
	if (row = ui->tableWidgetBacktrace->rowCount())
	{
		ui->tableWidgetBacktrace->selectRow(0);
		ui->tableWidgetBacktrace->setItem(row - 1, 8, new QTableWidgetItem("n/a"));
	}
	else
		register_cache.setActiveFrame(0), updateRegisterView(), showDisassembly();
	if (/* this is not exact, which it needs not be */ t.elapsed() > profiling.max_backtrace_generation_time)
		profiling.max_backtrace_generation_time = t.elapsed();
}

bool MainWindow::readElfSections(void)
{
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
		if (name == ".debug_info") debug_info_index = i;
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
int i, j;
	for (i = 0; i < elf.segments.size(); i ++)
		if (elf.segments[i]->get_type() == PT_LOAD || elf.segments[i]->get_type() == PT_ARM_EXIDX)
		{
			/* this is very confusing, I could not think of anything better */
			uint64_t l = elf.segments[i]->get_virtual_address(), h = l + elf.segments[i]->get_file_size(), pa = elf.segments[i]->get_physical_address(), address;
			for (j = 0; j < elf.sections.size(); j ++)
				if (l <= (address = elf.sections[j]->get_address()) && address < h)
					target_memory_contents.addRange(pa + address - l, QByteArray(elf.sections[j]->get_data(), elf.sections[j]->get_size()));
		}
	target_memory_contents.dump();
}

void MainWindow::updateRegisterView(void)
{
	for (int row(0); row < register_cache.registerCount(); row ++)
	{
		QString s(QString("$%1").arg(register_cache.readCachedRegister(row), 0, 16)), t = ui->tableWidgetRegisters->item(row, 1)->text();
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
	return dwdata->typeString(type_cache, 1);
}

void MainWindow::dumpData(uint32_t address, const QByteArray &data)
{
	ui->plainTextEditDataDump->clear();
	ui->plainTextEditDataDump->appendPlainText(data.toHex());
}

void MainWindow::updateBreakpointsView(void)
{
int i, j;
QMap<QString, QVariant> user_data;
	ui->treeWidgetBreakpoints->blockSignals(true);
	ui->treeWidgetBreakpoints->clear();
	for (i = 0; i < breakpoints.sourceCodeBreakpoints.size(); i ++)
	{
		const BreakpointCache::SourceCodeBreakpoint & b(breakpoints.sourceCodeBreakpoints.at(i));
		auto t = new QTreeWidgetItem(ui->treeWidgetBreakpoints, QStringList() << b.source_filename << QString("%1").arg(b.line_number));
		t->setFlags(t->flags() | Qt::ItemIsUserCheckable);
		t->setCheckState(0, b.enabled ? Qt::Checked : Qt::Unchecked);
		user_data.clear();
		user_data["source-breakpoint-index"] = i;
		t->setData(0, Qt::UserRole, user_data);
		if (b.addresses.size() == 1)
		{
			t->setText(2, QString("$%1").arg(b.addresses.at(0), 8, 16, QChar('0')));
			user_data["source-breakpoint-sub-index"] = 0;
		}
		else
		{
			t->setText(2, t->text(2) + QString("%1 locations").arg(b.addresses.size()));
			for (j = 0; j < b.addresses.size(); j ++)
			{
				auto n = new QTreeWidgetItem(t);
				user_data["source-breakpoint-sub-index"] = j;
				n->setData(0, Qt::UserRole, user_data);
				n->setText(2, QString("$%1").arg(b.addresses.at(j), 8, 16, QChar('0')));
			}
		}
	}
	user_data.clear();
	if (breakpoints.machineAddressBreakpoints.size())
	{
		auto t = new QTreeWidgetItem(ui->treeWidgetBreakpoints, QStringList() << "machine-level breakpoints");
		for (i = 0; i < breakpoints.machineAddressBreakpoints.size(); i ++)
		{
			user_data["machine-breakpoint-index"] = i;
			auto x =new QTreeWidgetItem(t, QStringList()
				<< breakpoints.machineAddressBreakpoints.at(i).inferred_breakpoint.source_filename
				<< QString("%1").arg(breakpoints.machineAddressBreakpoints.at(i).inferred_breakpoint.line_number)
				<< QString("$%1").arg(breakpoints.machineAddressBreakpoints.at(i).address, 8, 16, QChar('0')));
			x->setFlags(x->flags() | Qt::ItemIsUserCheckable);
			x->setCheckState(0, breakpoints.machineAddressBreakpoints.at(i).enabled ? Qt::Checked : Qt::Unchecked);
			x->setData(0, Qt::UserRole, user_data);
		}
		ui->treeWidgetBreakpoints->expandItem(t);
	}
	ui->treeWidgetBreakpoints->blockSignals(false);
}

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
	connect(& elf_file_modification_watcher, SIGNAL(fileChanged(QString)), this, SLOT(elfFileModified(QString)));
	elf_file_modification_watcher.addPath(elf_filename);
	
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
	if (debug_info_index) debug_info = QByteArray(elf.sections[debug_info_index]->get_data(), elf.sections[debug_info_index]->get_size());
	if (debug_abbrev_index) debug_abbrev = QByteArray(elf.sections[debug_abbrev_index]->get_data(), elf.sections[debug_abbrev_index]->get_size());
	if (debug_frame_index) debug_frame = QByteArray(elf.sections[debug_frame_index]->get_data(), elf.sections[debug_frame_index]->get_size());
	if (debug_ranges_index) debug_ranges = QByteArray(elf.sections[debug_ranges_index]->get_data(), elf.sections[debug_ranges_index]->get_size());
	if (debug_str_index) debug_str = QByteArray(elf.sections[debug_str_index]->get_data(), elf.sections[debug_str_index]->get_size());
	if (debug_line_index) debug_line = QByteArray(elf.sections[debug_line_index]->get_data(), elf.sections[debug_line_index]->get_size());
	if (debug_loc_index) debug_loc = QByteArray(elf.sections[debug_loc_index]->get_data(), elf.sections[debug_loc_index]->get_size());
	
	t.restart();
	dwdata = new DwarfData(debug_info.data(), debug_info.length(), debug_abbrev.data(), debug_abbrev.length(), debug_ranges.data(), debug_ranges.length(), debug_str.data(), debug_str.length(), debug_line.data(), debug_line.length(), debug_loc.data(), debug_loc.length());
	
	{
		auto source_breakpoints = s.value("source-level-breakpoints", QStringList()).toStringList();
		ui->treeWidgetBreakpoints->blockSignals(true);
		//QRegExp rx("(.+):(.*):(.*):(\\d+)$");
		QRegExp rx("([^>]+)>([^>]*)>([^>]*)>(\\d+)>([^>]+)$");
		for (i = 0; i < source_breakpoints.size(); i ++)
			if (rx.indexIn(source_breakpoints[i]) != -1)
			{
				BreakpointCache::SourceCodeBreakpoint b;
				b.source_filename = rx.cap(1);
				b.directory_name = rx.cap(2);
				b.compilation_directory = rx.cap(3);
				b.line_number = rx.cap(4).toUInt();
				b.addresses = QVector<uint32_t>::fromStdVector(dwdata->filteredAddressesForFileAndLineNumber(b.source_filename.toLocal8Bit().constData(), b.line_number));
				b.enabled = (rx.cap(5) == "enabled");
				breakpoints.addSourceCodeBreakpoint(b);
			}
		QStringList saved_breakpoints = s.value("machine-level-breakpoints", QStringList()).toStringList();
		rx.setPattern("([^>]+)>([^>]*)");
		for (i = 0; i < saved_breakpoints.length(); i ++)
			if (rx.indexIn(saved_breakpoints[i]) != -1)
			{
				uint32_t address = rx.cap(1).toUInt();
	
				auto x = dwdata->sourceCodeCoordinatesForAddress(address);
				BreakpointCache::SourceCodeBreakpoint b;
				b.source_filename = QString::fromStdString(x.file_name);
				b.directory_name = QString::fromStdString(x.directory_name);
				b.compilation_directory = QString::fromStdString(x.compilation_directory_name);
				b.line_number = x.line;
				b.addresses.push_back(address);
				b.enabled = (rx.cap(2) == "enabled");
				breakpoints.addMachineAddressBreakpoint((struct BreakpointCache::MachineAddressBreakpoint){ .address = address, .inferred_breakpoint = b, .enabled = b.enabled, });
			}
		updateBreakpointsView();
		ui->treeWidgetBreakpoints->blockSignals(false);
	}
	
	ui->plainTextEdit->appendPlainText(QString("compilation unit count in the .debug_aranges section : %1").arg(dwdata->compilation_unit_count()));
	profiling.all_compilation_units_processing_time = t.elapsed();
	qDebug() << "all compilation units in .debug_info processed in" << profiling.all_compilation_units_processing_time << "milliseconds";
	dwdata->dumpStats();
	
	dwundwind = new DwarfUnwinder(debug_frame.data(), debug_frame.length());
	while (!dwundwind->at_end())
		dwundwind->dump(), dwundwind->next();
	
	if (TEST_DRIVE_MODE)
	{
		target = new TargetCorefile("troll-test-drive-files/flash.bin", 0x08000000, "troll-test-drive-files/ram.bin", 0x20000000, "troll-test-drive-files/registers.bin");
		gdbserver = new GdbServer(target);
	}
	else
		target = new TargetCorefile("flash.bin", 0x08000000, "ram.bin", 0x20000000, "registers.bin");
	
	sforth_console = ui->plainTextEditSforthConsole;
	sforth = new Sforth(sforth_console_output_function);
	/*! \todo	WARNING!!! as of writing this note (30042017), the order of creating
	 *		the objects of classes 'CortexM0' and 'DwarfEvaluator' is important,
	 *		because they both define a value with the name 'cfa-value' - a
	 *		misfortune that must be fixed; if the order of creation
	 *		of these objects is reversed, the incorrect 'cfa-value' variable
	 *		will be used when evaluating dwarf expression, leading to very
	 *		bizarre and mystique misbehavior */
	cortexm0 = new CortexM0(sforth, target, & register_cache);
	cortexm0->primeUnwinder();
	dwarf_evaluator = new DwarfEvaluator(sforth, dwdata, & register_cache);
	connect(dwarf_evaluator, SIGNAL(entryValueComputed(DwarfEvaluator::DwarfExpressionValue)), this, SLOT(dwarfEntryValueComputed(DwarfEvaluator::DwarfExpressionValue)));
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
	dwdata->reapStaticObjects(data_objects, subprograms);
	profiling.static_storage_duration_data_reap_time = t.elapsed();
	qDebug() << "static storage duration data reaped in" << profiling.static_storage_duration_data_reap_time << "milliseconds";
	qDebug() << "data objects:" << data_objects.size() << ", subprograms:" << subprograms.size();
	populateFunctionsListView();
	t.restart();

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
	ui->tableWidgetStaticDataObjects->sortItems(0);
	ui->tableWidgetStaticDataObjects->resizeColumnsToContents();
	/*! \warning	resizing the rows to fit the contents can be **very** expensive */
	//ui->tableWidgetStaticDataObjects->resizeRowsToContents();
	profiling.static_storage_duration_display_view_build_time = t.elapsed();
	qDebug() << "static object lists built in" << profiling.static_storage_duration_display_view_build_time << "milliseconds";

	profiling.debugger_startup_time = startup_time.elapsed();
	qDebug() << "debugger startup time:" << profiling.debugger_startup_time << "milliseconds";

	connect(& blackstrike_port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(blackstrikeError(QSerialPort::SerialPortError)));
	
	populateSourceFilesView(false);

	ui->plainTextEdit->installEventFilter(this);
	targetDisconnected();
	highlighter = new Highlighter(ui->plainTextEdit->document());
	verbose_data_type_highlighter = new Highlighter(ui->plainTextEditVerboseDataType->document());
	
        connect(& polishing_timer, SIGNAL(timeout()), this, SLOT(polishSourceCodeViewOnTargetExecution()));
        ui->plainTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        ui->mainToolBar->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        ui->actionTarget_status->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        ui->plainTextEditVerboseDataType->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	connect(ui->checkBoxHideDuplicateSubprograms, &QCheckBox::stateChanged, [this] { populateFunctionsListView(ui->checkBoxHideDuplicateSubprograms->isChecked());});
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::elfFileModified(QString name)
{
	QMessageBox::warning(0, "Loaded ELF file changed", "The loaded ELF file has been changed!");
}

void MainWindow::dwarfEntryValueComputed(DwarfEvaluator::DwarfExpressionValue entry_value)
{
	currently_evaluated_local_data_object->setText(QString("%1 ").arg(entry_value.value));
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
register_cache.setActiveFrame(frame_number);
updateRegisterView();
ui->tableWidgetLocalVariables->setRowCount(0);
if (!ui->tableWidgetBacktrace->item(row, 6))
{
	ui->plainTextEdit->setPlainText("singularity; context undefined");
	return;
}
uint32_t cfa_value = (register_cache.frameCount() - 1 > frame_number) ? register_cache.readCachedRegister(/*! \todo fix this! don't hardcode it! */13, 1) : -1;
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
		ui->tableWidgetLocalVariables->setItem(row, 3, currently_evaluated_local_data_object = new QTableWidgetItem("n/a"));
		auto location = dwarf_evaluator->evaluateLocation(cfa_value, frameBaseSforthCode, locationSforthCode);
		if (location.type == DwarfEvaluator::DwarfExpressionValue::INVALID)
			ui->tableWidgetLocalVariables->setItem(row, 2, new QTableWidgetItem("cannot evaluate"));
		else
		{
			QString prefix;
			int base = 16, width = 0;
			if (location.type == DwarfEvaluator::DwarfExpressionValue::MEMORY_ADDRESS)
				prefix = "@$", width = 8;
			else if (location.type == DwarfEvaluator::DwarfExpressionValue::REGISTER_NUMBER)
				prefix = "#r";
			else
				base = 10;
			ui->tableWidgetLocalVariables->setItem(row, 2, new QTableWidgetItem((prefix + "%1").arg(location.value, width, base)));
			
			switch (base = ui->comboBoxDataDisplayNumericBase->currentText().toUInt())
			{
				case 2: prefix = "%"; break;
				case 16: prefix = "$"; break;
				case 10: break;
				default: Util::panic();
			}

			struct DwarfData::DataNode node;
			dwdata->dataForType(type_cache, node, 1);
			/*
			if (location.type == DwarfEvaluator::DwarfExpressionValue::MEMORY_ADDRESS)
			{
				auto n = new QTreeWidgetItem(QStringList() << data_object_name);
				n->addChild(itemForNode(node, target->readBytes(location.value, node.bytesize, true).toHex(), 0, base, ""));
				ui->treeWidgetDataObjects->addTopLevelItem(n);
			}
			else if (location.type == DwarfEvaluator::DwarfExpressionValue::REGISTER_NUMBER)
			{
				auto n = new QTreeWidgetItem(QStringList() << data_object_name);
				uint32_t register_contents = register_cache.readCachedRegister(location.value);
				n->addChild(itemForNode(node, QByteArray((const char *) & register_contents, sizeof register_contents).toHex(), 0, base, ""));
				ui->treeWidgetDataObjects->addTopLevelItem(n);
			}
			*/
			auto n = new QTreeWidgetItem(QStringList() << data_object_name);
			n->addChild(itemForNode(node, DwarfEvaluator::fetchValueFromTarget(location, target, node.bytesize), 0, base, ""));
			ui->treeWidgetDataObjects->addTopLevelItem(n);
		}
		ui->tableWidgetLocalVariables->setItem(row, 4, new QTableWidgetItem(locationSforthCode));
		if (locationSforthCode.isEmpty())
			/* the data object may have been evaluated as a compile-time constant - try that */
			ui->tableWidgetLocalVariables->item(row, 4)->setText(dwdata->constantValueSforthCode(locals.at(i)).toHex());
		ui->tableWidgetLocalVariables->setItem(row, 5, new QTableWidgetItem(QString("$%1").arg(locals.at(i).offset, 0, 16)));
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
	case QSerialPort::ResourceError:
		QMessageBox::critical(0, "blackmagic probe connection error", "blackmagic probe connection error - probe disconnected?");
		detachBlackmagicProbe();
		break;
	}
}

void MainWindow::closeEvent(QCloseEvent *e)
{
QSettings s("troll.rc", QSettings::IniFormat);
int i;
	s.setValue("window-geometry", saveGeometry());
	s.setValue("window-state", saveState());
	s.setValue("main-splitter/geometry", ui->splitterMain->saveGeometry());
	s.setValue("main-splitter/state", ui->splitterMain->saveState());
	s.setValue("hack-mode", ui->actionHack_mode->isChecked());
	s.setValue("show-disassembly-ranges", ui->actionShow_disassembly_address_ranges->isChecked());
	s.setValue("data-display-numeric-base", ui->comboBoxDataDisplayNumericBase->currentIndex());
	s.setValue("last-elf-file", elf_filename);
	s.setValue("scratchpad-contents", ui->plainTextEditScratchpad->toPlainText());
	QStringList source_breakpoints;
	for (i = 0; i < breakpoints.sourceCodeBreakpoints.size(); i ++)
		source_breakpoints << QString("%1>%2>%3>%4>%5")
		                      .arg(breakpoints.sourceCodeBreakpoints[i].source_filename)
		                      .arg(breakpoints.sourceCodeBreakpoints[i].directory_name)
		                      .arg(breakpoints.sourceCodeBreakpoints[i].compilation_directory)
		                      .arg(breakpoints.sourceCodeBreakpoints[i].line_number)
		                      .arg(breakpoints.sourceCodeBreakpoints[i].enabled ? "enabled" : "disabled");
	s.setValue("source-level-breakpoints", source_breakpoints);
	QStringList machine_brekpoints;
	for (i = 0; i < breakpoints.machineAddressBreakpoints.size(); i ++)
		machine_brekpoints << QString("%1>%2")
		                      .arg(breakpoints.machineAddressBreakpoints[i].address)
		                      .arg(breakpoints.machineAddressBreakpoints[i].enabled ? "enabled" : "disabled");
	s.setValue("machine-level-breakpoints", machine_brekpoints);
	QStringList bookmarks;
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
bool is_running_to_cursor = false;
bool is_breakpoint_toggled = false;
static unsigned accumulator;
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if (Qt::Key_0 <= keyEvent->key() && keyEvent->key() <= Qt::Key_9)
			accumulator *= 10, accumulator += keyEvent->key() - Qt::Key_0;
		switch (keyEvent->key())
		{
			case Qt::Key_Z:
				ui->plainTextEdit->centerCursor();
				break;
			case Qt::Key_Asterisk:
			{
				auto c = ui->plainTextEdit->textCursor();
				c.select(QTextCursor::WordUnderCursor);
				searchSourceView(c.selectedText());
			}
				break;
			case Qt::Key_F:
				if (!keyEvent->modifiers() & Qt::ControlModifier)
					break;
			case Qt::Key_Slash:
				ui->lineEditSearchForText->clear();
				ui->lineEditSearchForText->setFocus();
				break;
			case Qt::Key_N:
				searchSourceView(last_search_pattern);
				break;
			case Qt::Key_W:
			{
				auto c = ui->plainTextEdit->textCursor();
				c.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, accumulator ? accumulator : 1);
				ui->plainTextEdit->setTextCursor(c);
				accumulator = 0;
			}
				break;
			case Qt::Key_B:
			{
				auto c = ui->plainTextEdit->textCursor();
				c.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, accumulator ? accumulator : 1);
				ui->plainTextEdit->setTextCursor(c);
				accumulator = 0;
			}
				break;
			case Qt::Key_F4:
				is_running_to_cursor = true;
				if (0)
			case Qt::Key_T:
					is_breakpoint_toggled = true;
			case Qt::Key_Space:
			{
				bool ok;
				int i;
				QString l = ui->plainTextEdit->textCursor().block().text();
				qDebug() << l;
				QRegExp rx("^(\\w+)\\s*\\**\\s*\\|");
				if (rx.indexIn(l) != -1)
				{
					int j;
					i = rx.cap(1).toUInt(& ok);
					if (!ok)
						break;
					qDebug() << "requesting breakpoint for source file" << last_source_filename << "line number" << i;
					QTime t;
					t.start();
					struct BreakpointCache::SourceCodeBreakpoint b = { .source_filename = last_source_filename, .directory_name = last_directory_name, .compilation_directory = last_compilation_directory, .enabled = true, .line_number = i, };
					if ((j = breakpoints.sourceBreakpointIndex(b)) == -1
							|| is_running_to_cursor)
					{
						auto x = dwdata->filteredAddressesForFileAndLineNumber(last_source_filename.toLocal8Bit().constData(), i);
						qDebug() << "filtered addresses:" << x.size();
						if (x.empty())
							break;
						if (is_running_to_cursor)
						{
							for (i = 0; i < x.size(); i ++)
							{
								struct BreakpointCache::MachineAddressBreakpoint b;
								b.address = x.at(i);
								b.enabled = true;
								run_to_cursor_breakpoints.addMachineAddressBreakpoint(b);
							}
							on_actionResume_triggered();
							break;
						}
						b.addresses = QVector<uint32_t>::fromStdVector(x);
						breakpoints.addSourceCodeBreakpoint(b);
						if (t.elapsed() > profiling.max_time_for_retrieving_breakpoint_addresses_for_line)
							profiling.max_time_for_retrieving_breakpoint_addresses_for_line = t.elapsed();
						t.restart();
						x = dwdata->unfilteredAddressesForFileAndLineNumber(last_source_filename.toLocal8Bit().constData(), i);
						if (t.elapsed() > profiling.max_time_for_retrieving_unfiltered_breakpoint_addresses_for_line)
							profiling.max_time_for_retrieving_unfiltered_breakpoint_addresses_for_line = t.elapsed();
						qDebug() << "total addresses:" << x.size();
					}
					else
						if (is_breakpoint_toggled)
							breakpoints.toggleSourceBreakpointAtIndex(j);
						else
							breakpoints.removeSourceCodeBreakpointAtIndex(j);
				}
				else
				{
					/* internal disassembly range format */
					rx.setPattern("^\\$(\\w+)");
					uint32_t address;
					if (rx.indexIn(l) == -1)
						/* objdump disassembly dump format */
						rx.setPattern("^\\s*(\\w+):");
					if ((rx.indexIn(l) != -1 && (address = rx.cap(1).toUInt(& ok, 16), ok))
							|| is_running_to_cursor)
					{
						if (is_running_to_cursor)
						{
							struct BreakpointCache::MachineAddressBreakpoint b;
							b.address = address;
							b.enabled = true;
							run_to_cursor_breakpoints.addMachineAddressBreakpoint(b);
							on_actionResume_triggered();
							break;
						}
						if ((i = breakpoints.machineBreakpointIndex(address)) == -1)
						{
							if (is_breakpoint_toggled)
								break;
							auto x = dwdata->sourceCodeCoordinatesForAddress(address);
							BreakpointCache::SourceCodeBreakpoint b;
							b.source_filename = QString::fromStdString(x.file_name);
							b.directory_name = QString::fromStdString(x.directory_name);
							b.compilation_directory = QString::fromStdString(x.compilation_directory_name);
							b.line_number = x.line;
							b.addresses.push_back(address);
							breakpoints.addMachineAddressBreakpoint((struct BreakpointCache::MachineAddressBreakpoint){ .address = address, .inferred_breakpoint = b, .enabled = true, });
						}
						else
						{
							if (is_breakpoint_toggled)
								breakpoints.toggleMachineBreakpointAtIndex(i);
							else
								breakpoints.removeMachineAddressBreakpointAtIndex(i);
						}
					}
					else
						break;
				}
				updateBreakpointsView();
				colorizeSourceCodeView();
			}
				break;
		case Qt::Key_G:
			{
			auto c = ui->plainTextEdit->textCursor();
				c.movePosition(QTextCursor::Start);
				c.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, accumulator - 1);
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
		case Qt::Key_BracketRight:
		{
			if (!keyEvent->modifiers() & Qt::ControlModifier)
				break;
			QTextCursor c = ui->plainTextEdit->textCursor();
			c.select(QTextCursor::WordUnderCursor);
			auto x = ui->tableWidgetFunctions->findItems(c.selectedText(), Qt::MatchExactly);
			if (x.empty())
				break;
			if (x.size() > 1)
				QMessageBox::information(0, "Multiple symbols found", "Multiple symbols found for id: " + c.selectedText() + "\nNavigating to the first item in the list");
			ui->tableWidgetFunctions->clearSelection();
			ui->tableWidgetFunctions->selectRow(x.at(0)->row());
			break;
		}
		default:
			result = false;
			break;
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
	/* Note: prefer reverse iterating of the serial ports, because, at least on the machine I am testing on,
	 * it seems that Windows enumerates the serial ports in such a manner, that the blackmagic
	 * serial wire output/debug serial port gets a number that is lower
	 * than the blackmagic gdbserver port. Iterating in reverse saves some nuisance during autoprobing
	 * for the blackmagic gdbserver port, because it will be queried before querying the blackmagic
	 * debug port (at which there will be no answer). */
	for (i = ports.size() - 1; i >= 0; i --)
	{
		qDebug() << ports[i].manufacturer() << ports.at(i).description() << ports.at(i).serialNumber() << ports.at(i).portName();
		if (ports.at(i).hasProductIdentifier() && ports.at(i).vendorIdentifier() == BLACKMAGIC_USB_VENDOR_ID)
		{
			blackstrike_port.setPortName(ports.at(i).portName());
			if (blackstrike_port.open(QSerialPort::ReadWrite))
			{
				t = new Blackmagic(& blackstrike_port);
				if (!t->connect())
				{
					delete t;
#if BLACKSTRIKE_SUPPORT_ENABLED
					t = new Blackstrike(& blackstrike_port);
					if (!t->connect())
					{
						blackstrike_port.close();
						delete t;
						continue;
					}
#else
					blackstrike_port.close();
					continue;
#endif /* BLACKSTRIKE_SUPPORT_ENABLED */
				}
				auto s = t->memoryMap();
				t->parseMemoryAreas(s);
				if (!t->syncFlash(target_memory_contents))
				{
					QMessageBox::critical(0, "memory contents mismatch", "target memory contents mismatch");
					Util::panic();
				}
				else
					QMessageBox::information(0, "memory contents match", "target memory contents match");
				attachBlackmagicProbe(t);
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
		QFileInfo f(last_source_filename);
		if (f.exists())
			QProcess::startDetached("cmd", QStringList() , QDir::toNativeSeparators(f.canonicalPath()));
	}
}

void MainWindow::on_actionExplore_triggered()
{
	if (ui->tableWidgetBacktrace->selectionModel()->hasSelection())
	{
		QFileInfo f(last_source_filename);
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

	displayVerboseDataTypeForDieOffset(die_offset);
	std::vector<struct DwarfTypeNode> type_cache;
	dwdata->readType(die_offset, type_cache);
	
	struct DwarfData::DataNode node;
	dwdata->dataForType(type_cache, node, 1);
	ui->treeWidgetDataObjects->clear();
	switch (numeric_base = ui->comboBoxDataDisplayNumericBase->currentText().toUInt())
	{
		case 2: numeric_prefix = "%"; break;
		case 16: numeric_prefix = "$"; break;
		case 10: break;
		default: Util::panic();
	}
	if (isTargetAccessible())
	{
		ui->treeWidgetDataObjects->addTopLevelItem(itemForNode(node, (data = target->readBytes(address, node.bytesize, true)).toHex(), 0, numeric_base, numeric_prefix));
		ui->treeWidgetDataObjects->expandAll();
		ui->treeWidgetDataObjects->resizeColumnToContents(0);
		dumpData(address, data);
	}

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
	ui->tableWidgetLocalVariables->setColumnHidden(4, !hack_mode);
	ui->tableWidgetLocalVariables->setColumnHidden(5, !hack_mode);
	
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
	for (i = 0; i < /*! \todo fix this! do not hardcode it! */16; i ++)
	{
		uint32_t x = target->readRawUncachedRegister(i);
		f.write((const char * ) & x, sizeof x);
	}
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
auto b = breakpoints.enabledMachineAddressBreakpoints + run_to_cursor_breakpoints.enabledMachineAddressBreakpoints;
enum TARGET_STATE_ENUM new_target_state = FREE_RUNNING;
	auto x = b.find(address_of_step_over_breakpoint = target->readRawUncachedRegister(15));
	if (x != b.end())
		b.erase(x), new_target_state = STEPPING_OVER_BREAKPOINT_AND_THEN_RESUMING;
auto breakpointed_addresses = b.constBegin();

	while (breakpointed_addresses != b.constEnd())
	{
		if (!target->breakpointSet(* breakpointed_addresses, 2))
		{
			QMessageBox::critical(0, "Failed to set breakpoint", "Failed to set breakpoint!\nToo many breakpoints requested?\nTarget execution aborted");
			return;
		}
		breakpointed_addresses ++;
	}
	execution_state = new_target_state;
	if (execution_state == STEPPING_OVER_BREAKPOINT_AND_THEN_RESUMING)
		target->requestSingleStep();
	else
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
	auto source_coordinates = dwdata->sourceCodeCoordinatesForDieOffset(ui->tableWidgetLocalVariables->item(row, 5)->text().remove(0, 1).toUInt(0, 16));
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
	if (reason == TARGET_LOST)
	{
		polishing_timer.stop();
		QMessageBox::critical(0, "connection to target lost", "connection to target lost - disconnecting blackmagic probe");
		detachBlackmagicProbe();
		return;
	}
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
		case STEPPING_OVER_BREAKPOINT_AND_THEN_RESUMING:
			if (!target->breakpointSet(address_of_step_over_breakpoint, 2))
				Util::panic();
			execution_state = FREE_RUNNING;
			target->resume();
			return;
			break;
		case FREE_RUNNING:
			break;
	}
	auto b = run_to_cursor_breakpoints.enabledMachineAddressBreakpoints.constBegin();
	while (b != run_to_cursor_breakpoints.enabledMachineAddressBreakpoints.constEnd())
		target->breakpointClear(* b, 2), b ++;
	run_to_cursor_breakpoints.removeAll();

	execution_state = HALTED;

	auto breakpointed_addresses = breakpoints.enabledMachineAddressBreakpoints.constBegin();
	while (breakpointed_addresses != breakpoints.enabledMachineAddressBreakpoints.constEnd())
	{
		target->breakpointClear(* breakpointed_addresses, 2);
		breakpointed_addresses ++;
	}
	polishing_timer.stop();
	switchActionOff(ui->actionBlackstrikeConnect);
	switchActionOn(ui->actionSingle_step);
	switchActionOn(ui->actionSource_step);
	switchActionOn(ui->actionReset_target);
	switchActionOn(ui->actionResume);
	switchActionOff(ui->actionHalt);
	switchActionOn(ui->actionRead_state);
	switchActionOn(ui->actionCore_dump);
	backtrace();
}

void MainWindow::targetDisconnected(void)
{
	switchActionOn(ui->actionBlackstrikeConnect);
	switchActionOff(ui->actionSingle_step);
	switchActionOff(ui->actionSource_step);
	switchActionOff(ui->actionReset_target);
	switchActionOff(ui->actionResume);
	switchActionOff(ui->actionHalt);
	switchActionOff(ui->actionRead_state);
	switchActionOff(ui->actionCore_dump);
}

void MainWindow::targetConnected()
{
	switchActionOff(ui->actionBlackstrikeConnect);
	switchActionOn(ui->actionSingle_step);
	switchActionOn(ui->actionSource_step);
	switchActionOn(ui->actionReset_target);
	switchActionOn(ui->actionResume);
	switchActionOn(ui->actionHalt);
	switchActionOn(ui->actionRead_state);
	switchActionOn(ui->actionCore_dump);

	backtrace();
}

void MainWindow::polishSourceCodeViewOnTargetExecution()
{
static int i;
static const char c[] = { '-', '\\', '|', '/', };
static const char * x[8] = { ">   ", " >  ", "  > ", "   >", "   <", "  < ", " <  ", "<   ", };
	polishing_timer.setInterval(150);
	//ui->actionTarget_status->setText(QString("target running [%1]").arg(c[i ++]));
	ui->actionTarget_status->setText(QString("target running [%1]").arg(x[i ++]));
	i &= 7;
	/*! \todo	WARNING - it seems that I am doing something wrong with the 'readyRead()' signal;
	 *		without the call to 'waitForReadyRead()' below, on one machine that I am testing on,
	 *		incoming data from the blackmagic probe sometimes does not cause the 'readyRead()'
	 *		signal to be emitted, which breaks the code badly; I discovered through trial and
	 *		error that the call to 'WaitForReadyRead()' function below seems to work around this issue */
	blackstrike_port.waitForReadyRead(1);
}

void MainWindow::targetRunning()
{
	switchActionOff(ui->actionBlackstrikeConnect);
	switchActionOff(ui->actionSingle_step);
	switchActionOff(ui->actionSource_step);
	switchActionOff(ui->actionReset_target);
	switchActionOff(ui->actionResume);
	switchActionOn(ui->actionHalt);
	switchActionOff(ui->actionRead_state);
	switchActionOff(ui->actionCore_dump);
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
		dwdata->dataForType(type_cache, node, 1);
		item->addChild(itemForNode(node, ok ? target->readBytes(address, node.bytesize, true).toHex() : QByteArray(), 0, 10, ""));
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

void MainWindow::on_actionRun_dwarf_tests_triggered()
{
	dwdata->runTests();
}

void MainWindow::on_treeWidgetBreakpoints_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
QMap<QString, QVariant> x = item->data(0, Qt::UserRole).toMap();
	if (x.contains("source-breakpoint-index"))
	{
		const BreakpointCache::SourceCodeBreakpoint & b(breakpoints.sourceCodeBreakpoints[x["source-breakpoint-index"].toInt()]);
		uint32_t address = x.contains("source-breakpoint-sub-index") ? b.addresses[x["source-breakpoint-sub-index"].toInt()] : -1;
		displaySourceCodeFile(b.source_filename, b.directory_name, b.compilation_directory, b.line_number, address);
	}
	else if (x.contains("machine-breakpoint-index"))
	{
		const BreakpointCache::SourceCodeBreakpoint & b(breakpoints.machineAddressBreakpoints[x["machine-breakpoint-index"].toInt()].inferred_breakpoint);
		displaySourceCodeFile(b.source_filename, b.directory_name, b.compilation_directory, b.line_number, b.addresses[0]);
	}
}

void MainWindow::on_treeWidgetBreakpoints_itemChanged(QTreeWidgetItem *item, int column)
{
	if (column)
		return;
QMap<QString, QVariant> x = item->data(0, Qt::UserRole).toMap();
int i;
	if (x.contains("source-breakpoint-index"))
	{
		const BreakpointCache::SourceCodeBreakpoint & b(breakpoints.sourceCodeBreakpoints[x["source-breakpoint-index"].toInt()]);
		i = x["source-breakpoint-index"].toInt();
		if (breakpoints.sourceCodeBreakpoints[i].enabled != (item->checkState(0) == Qt::Checked))
			breakpoints.setSourceBreakpointAtIndexEnabled(i, item->checkState(0) == Qt::Checked), colorizeSourceCodeView();
	}
	else if (x.contains("machine-breakpoint-index"))
	{
		const BreakpointCache::SourceCodeBreakpoint & b(breakpoints.machineAddressBreakpoints[x["machine-breakpoint-index"].toInt()].inferred_breakpoint);
		i = x["machine-breakpoint-index"].toInt();
		if (breakpoints.machineAddressBreakpoints[i].enabled != (item->checkState(0) == Qt::Checked))
			breakpoints.setMachineBreakpointAtIndexEnabled(i, item->checkState(0) == Qt::Checked), colorizeSourceCodeView();
	}
}

void MainWindow::on_checkBoxShowOnlyFilesWithMachineCode_stateChanged(int is_checked)
{
	populateSourceFilesView(is_checked);
}

void MainWindow::on_checkBoxDiscardTypedefSpecifiers_stateChanged(int arg1)
{
int die_offset = ui->lineEditDieOffsetForVerboseTypeDisplay->text().replace("$", "").toInt(0, 16);
	if (die_offset != -1)
		displayVerboseDataTypeForDieOffset(die_offset);
}

void MainWindow::on_lineEditSearchForText_returnPressed()
{
	searchSourceView(ui->lineEditSearchForText->text());
	ui->lineEditSearchForText->clear();
}
