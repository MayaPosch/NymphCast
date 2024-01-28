#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTimer>

#include "nymphcast_client.h"

#include <vector>

#include "remotes.h"
#include "custom_remotes.h"

#if defined(Q_OS_ANDROID)
#include <QtAndroidExtras>
#include <QAndroidJniEnvironment>
#include <QtAndroid>
#include <QStyleFactory>
#include <QVector>



const QVector<QString> permissions({"android.permission.INTERNET", 
									"android.permission.READ_EXTERNAL_STORAGE"});

class MediaItem : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString title READ title CONSTANT)
	Q_PROPERTY(QString album READ album CONSTANT)
	Q_PROPERTY(QString artist READ artist CONSTANT)
	Q_PROPERTY(QString path READ path CONSTANT)
	
	QString m_title;
	QString m_album;
	QString m_artist;
	QString m_path;
	
public:
	MediaItem(const QString title, const QString album, const QString artist, const QString path, 
																		QObject *parent = nullptr) 
		: QObject(parent), m_title(title), m_album(album), m_artist(artist), m_path(path) { }
	
	QString title() const { return m_title; }
	QString album() const { return m_album; }
	QString artist() const { return m_artist; }
	QString path() const { return m_path; }
};
#endif


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
	void openCustomRemotesDialog();
	
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
	void toggleSubtitles(int state);
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
	void updateRemotesList(std::vector<NCRemoteInstance> &remotes);
	
	void positionUpdate();
	
signals:
	void playbackStatusChange(uint32_t handle, NymphPlaybackStatus status);
	
private:
	Ui::MainWindow *ui;
	
	std::vector<NCRemoteInstance> remotes;
	std::vector<NCRemoteInstance> custom_remotes;
	std::vector<NCRemoteGroup> groups;
    std::vector<std::vector<NymphMediaFile> > mediaFiles;
	bool muted = false;
	bool playingTrack = false;
	bool singleCast = false;
    QStandardItemModel sharesModel;
	QString appDataLocation;
	int separatorIndex;
	RemotesDialog* rd;
	CustomRemotesDialog* crd;
	QTimer posTimer;
	
    QByteArray loadResource(const QUrl &name);
	void statusUpdateCallback(uint32_t handle, NymphPlaybackStatus status);
	bool remoteIsConnected();
	bool remoteEnsureConnected(uint32_t &handle);
	bool connectRemote(NCRemoteInstance &instance);
	bool disconnectRemote(NCRemoteInstance &instance);
	
	bool loadGroups();
	bool saveGroups();
	
	bool loadRemotes();
	bool saveRemotes();
};

#endif // MAINWINDOW_H
