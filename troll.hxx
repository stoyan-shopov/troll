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
#include <QTableWidget>
#include <QFileSystemWatcher>
#include "libtroll.hxx"
#include "sforth.hxx"
#include "target-corefile.hxx"
#include "blackstrike.hxx"
#include "blackmagic.hxx"
#include "cortexm0.hxx"
#include "dwarf-evaluator.hxx"
#include "registercache.hxx"
#include <QSerialPort>
#include <QSyntaxHighlighter>
#include <QTimer>
#include "s-record.hxx"
#include "disassembly.hxx"
#include "breakpoint-cache.hxx"
#include <elfio/elfio.hpp>

enum
{
	BLACKMAGIC_USB_VENDOR_ID	=	0x1d50,
	PT_ARM_EXIDX			=	0x70000001,
};

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
	qint64 debug_aranges_index;
	qint64 debug_info_index;
	qint64 debug_abbrev_index;
	qint64 debug_frame_index;
	qint64 debug_ranges_index;
	qint64 debug_str_index;
	qint64 debug_line_index;
	qint64 debug_loc_index;

	QByteArray debug_aranges, debug_info, debug_abbrev, debug_frame, debug_ranges, debug_str, debug_line, debug_loc;
	
	void dump_debug_tree(std::vector<struct Die> & dies, int level);
	QTimer		polishing_timer;
	DwarfData * dwdata;
	Disassembly 	* disassembly;
	Highlighter	* highlighter;
	Sforth	* sforth;
	Target	* target;
	RegisterCache	register_cache;
	DwarfUnwinder	* dwundwind;
	CortexM0	* cortexm0;
	DwarfEvaluator	* dwarf_evaluator;
	Memory		target_memory_contents;
	BreakpointCache	breakpoints;
	BreakpointCache	run_to_cursor_breakpoints;
	
	struct SourceCodeDisplayData
	{
		//QVector<uint32_t> enabled_breakpoint_positions, disabled_breakpoint_positions;
		QMap<uint32_t /* address */, int /* line position in text document */> address_positions_in_document;
		QMap<int /* line number */, int /* line position in text document */> line_positions_in_document;
	}
	src;
public:
	struct TreeWidgetNodeData
	{
		uint32_t	pointer_type_die_offset;
	};
private:
	QFileSystemWatcher	elf_file_modification_watcher;
	QTreeWidgetItem * itemForNode(const struct DwarfData::DataNode & node, const QByteArray & data = QByteArray(), int data_pos = 0, int numeric_base = 10, const QString & numeric_prefix = QString());
	QString last_source_filename, last_directory_name, last_compilation_directory;
	QString current_source_code_file_displayed;
	int last_highlighted_line;
	uint32_t last_source_highlighted_address;
	void displaySourceCodeFile(QString source_filename, QString directory_name, QString compilation_directory, int highlighted_line, uint32_t address = -1);
	void refreshSourceCodeView(int center_line = -1);
	void backtrace(void);
	bool readElfSections(void);
	bool loadSRecordFile(void);
	bool loadElfMemorySegments(void);
	QString elf_filename;
	ELFIO::elfio elf;
	void updateRegisterView(void);
	std::string typeStringForDieOffset(uint32_t die_offset);
	void dumpData(uint32_t address, const QByteArray & data);
	void updateBreakpointsView(void);
	void colorizeSourceCodeView(void);
	void populateSourceFilesView(bool show_only_files_with_generated_machine_code);
	void showDisassembly(void);
	void attachBlackmagicProbe(Target * blackmagic);
	void detachBlackmagicProbe(void);

	enum
	{
		INVALID_EXECUTION_STATE = 0,
		FREE_RUNNING,
		STEPPING_OVER_BREAKPOINT_AND_THEN_RESUMING,
		HALTED,
		SOURCE_LEVEL_SINGLE_STEPPING,
	}
	execution_state;
	uint32_t address_of_step_over_breakpoint;

	void switchActionOn(QAction * action) { action->setEnabled(true); action->setVisible(true); }
	void switchActionOff(QAction * action) { action->setEnabled(false); action->setVisible(false); }
	QTableWidgetItem	* currently_evaluated_local_data_object;
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	
private slots:
	void elfFileModified(QString name);
	void dwarfEntryValueComputed(struct DwarfEvaluator::DwarfExpressionValue entry_value);
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
	
	void on_lineEditStaticDataObjects_textChanged(const QString &arg1);
	
	void on_lineEditSubprograms_textChanged(const QString &arg1);
	
	void on_lineEditStaticDataObjects_returnPressed();
	
	void on_lineEditSubprograms_returnPressed();
	
	void on_actionSource_step_triggered();
	
	void on_pushButtonCreateBookmark_clicked();
	
	void on_tableWidgetBookmarks_doubleClicked(const QModelIndex &index);
	
	void on_pushButtonRemoveBookmark_clicked();
	
	void on_pushButtonMoveBookmarkUp_clicked();
	
	void on_pushButtonMoveBookmarkDown_clicked();
	
	void on_actionView_windows_triggered();
	
	void on_actionRun_dwarf_tests_triggered();
	
	void on_treeWidgetBreakpoints_itemDoubleClicked(QTreeWidgetItem *item, int column);
	
	void on_treeWidgetBreakpoints_itemChanged(QTreeWidgetItem *item, int column);
	
	void on_checkBoxShowOnlyFilesWithMachineCode_stateChanged(int is_checked);
	
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
