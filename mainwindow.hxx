#ifndef MAINWINDOW_HXX
#define MAINWINDOW_HXX

#include <QMainWindow>
#include <QTreeWidget>
#include "libtroll.hxx"
#include "sforth.hxx"
#include "target.hxx"
#include "cortexm0.hxx"

#define MAIN_APS	0

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
#if MAIN_APS
	const qint64 debug_aranges_offset = 0x062ff7, debug_aranges_len = 0x000990;
	const qint64 debug_info_offset = 0x0200f8, debug_info_len = 0x02b7d1;
	const qint64 debug_abbrev_offset = 0x04b8c9, debug_abbrev_len = 0x008054;
	const qint64 debug_frame_offset = 0x0841c8, debug_frame_len = 0x004798;
	const qint64 debug_ranges_offset = 0x088960, debug_ranges_len = 0x000928;
	const qint64 debug_str_offset = 0x06f4d7, debug_str_len = 0x014c51;
	const qint64 debug_line_offset = 0x063987, debug_line_len = 0x00bb50;
	const qint64 debug_loc_offset = 0x05391d, debug_loc_len = 0x00f6da;
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
	
	void dump_debug_tree(std::vector<struct Die> & dies, int level);
	DwarfData * dwdata;
	Sforth	* sforth;
	Target	* target;
	DwarfUnwinder	* dwundwind;
	CortexM0	* cortexm0;
	QTreeWidgetItem * itemForNode(const struct DwarfData::DataNode & node);
	void backtrace(void);
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	
private slots:
	void on_lineEditSforthCommand_returnPressed();
	
private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_HXX
