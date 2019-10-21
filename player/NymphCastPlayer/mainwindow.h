#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
private slots:
    void openFile();
    void connectServer();
    void quit();
    
private:
    Ui::MainWindow *ui;
    
    bool connected = false;
};

#endif // MAINWINDOW_H
