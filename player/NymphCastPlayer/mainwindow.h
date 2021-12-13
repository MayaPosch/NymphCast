#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTimer>

#include "nymphcast_client.h"

#include <vector>

#include "remotes.h"


namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
	
public:
	static NymphCastClient client;
    
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();
	
private slots:
    // Menu
	void about();
	void quit();
	void castFile();
	void castUrl();
	void loadPlaylist();
	void savePlaylist();
	void clearPlaylist();
	
    // Player tab
	void playerRemoteChanged(int index);
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
	void cycleSubtitles();
	void cycleAudio();
	void cycleVideo();
	
	void setPlaying(uint32_t handle, NymphPlaybackStatus status);
	void updatePlayerUI(NymphPlaybackStatus status, NCRemoteInstance* ris);
	
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
	void remoteListRefresh();
	void openRemotesDialog();
	void updateRemotesList();
	void updateGroupsList(std::vector<NCRemoteGroup> &groups);
	void addGroupsToRemotes();
	
	void positionUpdate();
	
signals:
	void playbackStatusChange(uint32_t handle, NymphPlaybackStatus status);
	
private:
	Ui::MainWindow *ui;
	
	std::vector<NCRemoteInstance> remotes;
	std::vector<NCRemoteGroup> groups;
    std::vector<std::vector<NymphMediaFile> > mediaFiles;
	bool muted = false;
	bool playingTrack = false;
	bool singleCast = false;
    QStandardItemModel sharesModel;
	QString appDataLocation;
	int separatorIndex;
	RemotesDialog* rd;
	QTimer posTimer;
	
    QByteArray loadResource(const QUrl &name);
	void statusUpdateCallback(uint32_t handle, NymphPlaybackStatus status);
	bool remoteIsConnected();
	bool remoteEnsureConnected(uint32_t &handle);
	bool connectRemote(NCRemoteInstance &instance);
	bool disconnectRemote(NCRemoteInstance &instance);
	
	bool loadGroups();
	bool saveGroups();
};

#endif // MAINWINDOW_H
