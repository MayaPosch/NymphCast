#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	
    QApplication a(argc, argv);	
    MainWindow w;
    w.show();
    
    return a.exec();
}
