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
    void connectServer();
	void connectServerIP(std::string ip);
	void disconnectServer();
	void about();
    void quit();
	
	void castFile();
	void castUrl();
	
	void addFile();
	void removeFile();
	
	void play();
	void stop();
	void pause();
	void forward();
	void rewind();
	void seek();
	
	void mute();
	void adjustVolume(int value);
	
	void setPlaying(uint32_t handle, NymphPlaybackStatus status);
	
	void remoteListRefresh();
	void remoteConnectSelected();
	void remoteDisconnectSelected();
	
	void appsListRefresh();
	
	void sendCommand();
	
signals:
	void playbackStatusChange(uint32_t handle, NymphPlaybackStatus status);
    
private:
    Ui::MainWindow *ui;
    
    bool connected = false;
	bool muted = false;
	bool playingTrack = false;
	uint32_t serverHandle;
	NymphCastClient client;
	
	void statusUpdateCallback(uint32_t handle, NymphPlaybackStatus status);
};

#endif // MAINWINDOW_H
