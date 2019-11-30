#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "nymphcast_client.h"


namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
private slots:
    void openFile();
    void connectServer();
	void disconnectServer();
	void about();
    void quit();
	
	void castFile();
	void castUrl();
	
	void addFile();
	void removeFile();
	
	void findServer();
	
	void play();
	void stop();
	void pause();
	void forward();
	void rewind();
	
	void mute();
	void adjustVolume(int value);
	
	void remoteListRefresh();
	void remoteConnectSelected();
	void remoteDisconnectSelected();
	
	void sendCommand();
    
private:
    Ui::MainWindow *ui;
    
    bool connected = false;
	bool muted = false;
	uint32_t serverHandle;
	NymphCastClient client;
};

#endif // MAINWINDOW_H
