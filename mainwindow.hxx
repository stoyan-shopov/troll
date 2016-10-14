#ifndef MAINWINDOW_HXX
#define MAINWINDOW_HXX

#include <QMainWindow>
#include "libtroll.hxx"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
	const qint64 debug_aranges_offset = 0x04f7e0, debug_aranges_len = 0x000d70;
	const qint64 debug_info_offset = 0x01b729, debug_info_len = 0x01c299;
	const qint64 debug_abbrev_offset = 0x0379c2, debug_abbrev_len = 0x007f32;
	
	void dump_debug_tree(std::vector<struct Die> & dies, int level);
	DwarfData * dwdata;
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	
private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_HXX
