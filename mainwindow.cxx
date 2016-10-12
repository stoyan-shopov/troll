#include "mainwindow.hxx"
#include "ui_mainwindow.h"

#include "libtroll.hxx"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
}

MainWindow::~MainWindow()
{
	delete ui;
}
