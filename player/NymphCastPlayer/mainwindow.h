#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>

#include "nymphcast_client.h"

#include <vector>


namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
	
public:
    static uint32_t serverHandle;
	static NymphCastClient client;
    
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();
	
private slots:
    // Menu
	void connectServer();
	void connectServerIP(std::string ip);
	void disconnectServer();
	void about();
	void quit();
	void castFile();
	void castUrl();
	
    // Player tab
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
	
    // Remotes tab.
	void remoteListRefresh();
	void remoteConnectSelected();
	void remoteDisconnectSelected();
	
    // Apps tab.
	void appsListRefresh();	
	void sendCommand();
    
    // Tabs (GUI) tab.
    void appsHome();
    void anchorClicked(const QUrl &link);
    
    // Shares tab.
    void scanForShares();
    void playSelectedShare();
	
signals:
	void playbackStatusChange(uint32_t handle, NymphPlaybackStatus status);
	
private:
	Ui::MainWindow *ui;
	
	std::vector<NymphCastRemote> remotes;
    std::vector<std::vector<NymphMediaFile> > mediaFiles;
	bool connected = false;
	bool muted = false;
	bool playingTrack = false;
    QStandardItemModel sharesModel;
	
    QByteArray loadResource(const QUrl &name);
	void statusUpdateCallback(uint32_t handle, NymphPlaybackStatus status);
};

#endif // MAINWINDOW_H
