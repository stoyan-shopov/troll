#ifndef MAINWINDOW_HXX
#define MAINWINDOW_HXX

#include <QMainWindow>
#include "libtroll.hxx"
#include "sforth.hxx"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
	const qint64 debug_aranges_offset = 0x062ff7, debug_aranges_len = 0x000990;
	const qint64 debug_info_offset = 0x0200f8, debug_info_len = 0x02b7d1;
	const qint64 debug_abbrev_offset = 0x04b8c9, debug_abbrev_len = 0x008054;
	const qint64 debug_frame_offset = 0x0841c8, debug_frame_len = 0x004798;
	const qint64 debug_ranges_offset = 0x088960, debug_ranges_len = 0x000928;
	const qint64 debug_str_offset = 0x06f4d7, debug_str_len = 0x014c51;
	
	void dump_debug_tree(std::vector<struct Die> & dies, int level);
	DwarfData * dwdata;
	Sforth	* sforth;
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	
private slots:
	void on_lineEditSforthCommand_returnPressed();
	
private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_HXX
