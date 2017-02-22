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
#include "s-record.hxx"
#include "disassembly.hxx"

#define MAIN_APS	1

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
#if MAIN_APS
	qint64 debug_aranges_offset, debug_aranges_len;
	qint64 debug_info_offset, debug_info_len;
	qint64 debug_abbrev_offset, debug_abbrev_len;
	qint64 debug_frame_offset, debug_frame_len;
	qint64 debug_ranges_offset, debug_ranges_len;
	qint64 debug_str_offset, debug_str_len;
	qint64 debug_line_offset, debug_line_len;
	qint64 debug_loc_offset, debug_loc_len;
#else
	const qint64 debug_aranges_offset = 0x006b7400, debug_aranges_len = 0x00048878;
	const qint64 debug_info_offset = 0x006ffe00, debug_info_len = 0x0b3c036f;
	const qint64 debug_abbrev_offset = 0x0bac0200, debug_abbrev_len = 0x0015a080;
	const qint64 debug_frame_offset = 0x0be3ec00, debug_frame_len = 0x00000038;
	const qint64 debug_ranges_offset = 0x0c0bb800, debug_ranges_len = 0x000492e0;
	const qint64 debug_str_offset = 0x0be3ee00, debug_str_len = 0x0027c8a1;
	const qint64 debug_line_offset = 0x0bc1a400, debug_line_len = 0x002247ee;
	const qint64 debug_loc_offset = 0x0, debug_loc_len = 0x0;
#endif
	QByteArray debug_aranges, debug_info, debug_abbrev, debug_frame, debug_ranges, debug_str, debug_line, debug_loc;
	
	void dump_debug_tree(std::vector<struct Die> & dies, int level);
	DwarfData * dwdata;
	Disassembly 	* dis;
	Sforth	* sforth;
	Target	* target;
	RegisterCache	* register_cache;
	DwarfUnwinder	* dwundwind;
	CortexM0	* cortexm0;
	DwarfEvaluator	* dwarf_evaluator;
	SRecordMemoryData s_record_file;
	QTreeWidgetItem * itemForNode(const struct DwarfData::DataNode & node, const QByteArray & data = QByteArray(), int data_pos = 0, int numeric_base = 10, const QString & numeric_prefix = QString());
	QString last_source_filename, last_directory_name, last_compilation_directory;
	int last_highlighted_line;
	void displaySourceCodeFile(const QString & source_filename, const QString & directory_name, const QString &compilation_directory, int highlighted_line);
	void backtrace(void);
	bool readElfSections(void);
	bool loadSRecordFile(void);
	QString elf_filename;
	void updateRegisterView(int frame_number);
	std::string typeStringForDieOffset(uint32_t die_offset);
	void dumpData(uint32_t address, const QByteArray & data);
	void updateBreakpoints(void);
	struct Breakpoint
	{
		QString source_filename, directory_name, compilation_directory;
		int line_number;
		QVector<uint32_t> addresses;
	};
	QVector<struct Breakpoint> breakpoints;

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

#endif // MAINWINDOW_HXX
