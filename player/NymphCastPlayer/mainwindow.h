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
	
	struct NCRemoteInstance {
		NymphCastRemote remote;
		bool connected = false;
		uint32_t handle;
	};
	
public:
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
	void adjustVolume();
	
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
	
	// All tabs.
	void openRemotesDialog();
	
signals:
	void playbackStatusChange(uint32_t handle, NymphPlaybackStatus status);
	
private:
	Ui::MainWindow *ui;
	
	std::vector<NCRemoteInstance> remotes;
    std::vector<std::vector<NymphMediaFile> > mediaFiles;
	bool muted = false;
	bool playingTrack = false;
	bool singleCast = false;
    QStandardItemModel sharesModel;
	
    QByteArray loadResource(const QUrl &name);
	void statusUpdateCallback(uint32_t handle, NymphPlaybackStatus status);
	bool playerIsConnected();
	bool playerEnsureConnected(uint32_t &id);
	bool sharesIsConnected();
	bool sharesEnsureConnected(uint32_t &id);
	bool appsEnsureConnected(uint32_t &id);
	bool appsGuiEnsureConnected(uint32_t &id);
	bool connectRemote(NCRemoteInstance &instance);
	bool disconnectRemote(NCRemoteInstance &instance);
};

#endif // MAINWINDOW_H
