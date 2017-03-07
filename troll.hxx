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
#ifndef MAINWINDOW_HXX
#define MAINWINDOW_HXX

#include <QMainWindow>
#include <QTreeWidget>
#include "libtroll.hxx"
#include "sforth.hxx"
#include "target-corefile.hxx"
#include "blackstrike.hxx"
#include "cortexm0.hxx"
#include "dwarf-evaluator.hxx"
#include "registercache.hxx"
#include <QSerialPort>
#include <QSyntaxHighlighter>
#include <QTimer>
#include "s-record.hxx"
#include "disassembly.hxx"

namespace Ui {
class MainWindow;
}

class Highlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	Highlighter(QTextDocument *parent = 0);

protected:
	void highlightBlock(const QString &text) override;

private:
	struct HighlightingRule
	{
		QRegExp pattern;
		QTextCharFormat format;
	};
	QVector<HighlightingRule> highlightingRules;

	QRegExp commentStartExpression;
	QRegExp commentEndExpression;

	QTextCharFormat keywordFormat;
	QTextCharFormat classFormat;
	QTextCharFormat singleLineCommentFormat;
	QTextCharFormat multiLineCommentFormat;
	QTextCharFormat quotationFormat;
	QTextCharFormat functionFormat;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT
	qint64 debug_aranges_offset, debug_aranges_len;
	qint64 debug_info_offset, debug_info_len;
	qint64 debug_abbrev_offset, debug_abbrev_len;
	qint64 debug_frame_offset, debug_frame_len;
	qint64 debug_ranges_offset, debug_ranges_len;
	qint64 debug_str_offset, debug_str_len;
	qint64 debug_line_offset, debug_line_len;
	qint64 debug_loc_offset, debug_loc_len;

	QByteArray debug_aranges, debug_info, debug_abbrev, debug_frame, debug_ranges, debug_str, debug_line, debug_loc;
	
	void dump_debug_tree(std::vector<struct Die> & dies, int level);
	QTimer		polishing_timer;
	DwarfData * dwdata;
	Disassembly 	* disassembly;
	Highlighter	* highlighter;
	Sforth	* sforth;
	Target	* target;
	RegisterCache	* register_cache;
	DwarfUnwinder	* dwundwind;
	CortexM0	* cortexm0;
	DwarfEvaluator	* dwarf_evaluator;
	SRecordMemoryData s_record_file;
public:
	struct TreeWidgetNodeData
	{
		uint32_t	pointer_type_die_offset;
	};
private:
	QTreeWidgetItem * itemForNode(const struct DwarfData::DataNode & node, const QByteArray & data = QByteArray(), int data_pos = 0, int numeric_base = 10, const QString & numeric_prefix = QString());
	QString last_source_filename, last_directory_name, last_compilation_directory;
	int last_highlighted_line;
	void displaySourceCodeFile(const QString & source_filename, const QString & directory_name, const QString &compilation_directory, int highlighted_line, uint32_t address = -1);
	void refreshSourceCodeView(int center_line = -1);
	void backtrace(void);
	bool readElfSections(void);
	bool loadSRecordFile(void);
	QString elf_filename;
	void updateRegisterView(int frame_number);
	std::string typeStringForDieOffset(uint32_t die_offset);
	void dumpData(uint32_t address, const QByteArray & data);
	void updateBreakpoints(void);
	struct SourceLevelBreakpoint
	{
		QString source_filename, directory_name, compilation_directory;
		int line_number;
		QVector<uint32_t> addresses;
		bool operator == (const struct SourceLevelBreakpoint & other) const
		{
			return line_number == other.line_number && source_filename == other.source_filename
					&& directory_name == other.directory_name && compilation_directory == other.compilation_directory;
		}
	};
	struct MachineLevelBreakpoint
	{
		uint32_t	address;
		struct SourceLevelBreakpoint inferred_breakpoint;
	};
	QVector<struct MachineLevelBreakpoint> machine_level_breakpoints;
	int machineBreakpointIndex(uint32_t address)
	{
		int i;
		for (i = 0; i < machine_level_breakpoints.size(); i ++)
			if (machine_level_breakpoints.at(i).address == address)
				return i;
		return -1;
	}

	QVector<struct SourceLevelBreakpoint> breakpoints;
	int breakpointIndex(const struct SourceLevelBreakpoint & breakpoint)
	{
		int i;
		for (i = 0; i < breakpoints.size(); i ++)
			if (breakpoints.at(i) == breakpoint)
				return i;
		return -1;
	}
	int inferredBreakpointIndex(const struct SourceLevelBreakpoint & breakpoint)
	{
		int i;
		for (i = 0; i < machine_level_breakpoints.size(); i ++)
			if (machine_level_breakpoints.at(i).inferred_breakpoint == breakpoint)
				return i;
		return -1;
	}

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	
private slots:
	void on_lineEditSforthCommand_returnPressed();
	void on_tableWidgetBacktrace_itemSelectionChanged();
	void blackstrikeError(QSerialPort::SerialPortError error);
	
	void on_actionSingle_step_triggered();
	
	void on_actionBlackstrikeConnect_triggered();
	
	void on_actionShell_triggered();
	void on_actionExplore_triggered();
	
	void on_tableWidgetStaticDataObjects_itemSelectionChanged();
	
	void on_actionHack_mode_triggered();
	
	void on_actionReset_target_triggered();
	
	void on_actionCore_dump_triggered();
	
	void on_comboBoxDataDisplayNumericBase_currentIndexChanged(int index);
	
	void on_actionResume_triggered();
	
	void on_actionHalt_triggered();
	
	void on_actionRead_state_triggered();
	
	void on_tableWidgetFiles_itemSelectionChanged();
	
	void on_actionShow_disassembly_address_ranges_triggered();
	
	void on_tableWidgetLocalVariables_itemSelectionChanged();
	
	void on_tableWidgetFunctions_itemSelectionChanged();
	
	void targetHalted(enum TARGET_HALT_REASON reason);
	void targetRunning(void);
	void targetDisconnected(void);
	void targetConnected(void);
	void polishSourceCodeViewOnTargetExecution(void);

	void on_treeWidgetDataObjects_itemActivated(QTreeWidgetItem *item, int column);
	
protected:
	void closeEvent(QCloseEvent * e);
	bool eventFilter(QObject * watched, QEvent * event);
	
private:
	Ui::MainWindow *ui;
	QSerialPort	blackstrike_port;
	/* all times are in milliseconds */
	struct
	{
		unsigned	debug_sections_disk_read_time;
		unsigned	all_compilation_units_processing_time;
		unsigned	debug_lines_processing_time;
		unsigned	static_storage_duration_data_reap_time;
		unsigned	static_storage_duration_display_view_build_time;
		unsigned	debugger_startup_time;
		unsigned	max_backtrace_generation_time;
		unsigned	max_context_view_generation_time;
		unsigned	max_addresses_for_file_retrieval_time;
		unsigned	max_source_code_view_build_time;
		unsigned	max_local_data_objects_view_build_time;
		unsigned	max_time_for_retrieving_breakpoint_addresses_for_line;
		unsigned	max_time_for_retrieving_unfiltered_breakpoint_addresses_for_line;
	}
	profiling;
};

Q_DECLARE_METATYPE(MainWindow::TreeWidgetNodeData)

#endif // MAINWINDOW_HXX
