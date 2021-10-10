#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <vector>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QCoreApplication>
#include <QTime>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QStringList>
#include <QStandardPaths>
#include <QDir>
#include <QDataStream>

#include "remotes.h"

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

static int pfd[2];
static pthread_t thr;
static const char* tag = "NymphCastPlayer";

#include <android/log.h>

static void* thread_func(void*) {
    ssize_t rdsz;
    char buf[128];
    while ((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        //if (buf[rdsz - 1] == '\n') --rdsz; // Remove newline if it exists.
        buf[rdsz] = 0;  // add null-terminator
        __android_log_write(ANDROID_LOG_DEBUG, tag, buf);
    }
    
    return 0;
}


int start_logger(const char* app_name) {
    tag = app_name;

    /* make stdout line-buffered and stderr unbuffered */
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    if (pthread_create(&thr, 0, thread_func, 0) == -1) { return -1; }    
    pthread_detach(thr);
    return 0;
}

#endif


// Static declarations
NymphCastClient MainWindow::client;

	
Q_DECLARE_METATYPE(NymphPlaybackStatus);
Q_DECLARE_METATYPE(NCRemoteInstance);
Q_DECLARE_METATYPE(NCRemoteGroup);


MainWindow::MainWindow(QWidget *parent) :	 QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);
	
	// Register custom types.
	qRegisterMetaType<NymphPlaybackStatus>("NymphPlaybackStatus");
	qRegisterMetaType<NymphPlaybackStatus>("NCRemoteInstance");
	qRegisterMetaType<NymphPlaybackStatus>("NCRemoteGroup");
	qRegisterMetaType<uint32_t>("uint32_t");
	
	// Set application options.
	QCoreApplication::setOrganizationName("Nyanko");
	QCoreApplication::setApplicationName("NymphCastPlayer");
	QCoreApplication::setApplicationVersion("v0.1-alpha");
	
	// Set location for user data.
	appDataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	
	// Set configured or default stylesheet. Read out current value.
	// Skip stylesheet if file isn't found.
	QSettings settings;
	if (!settings.contains("stylesheet")) {
		settings.setValue("stylesheet", "default.css");
	}
	
	QString sFile = settings.value("stylesheet", "default.css").toString();
	
	if (!QFile::exists(sFile)) {
		// Use stylesheet from resource bundle.
		std::cout << "Using stylesheet from resources." << std::endl;
		sFile = ":/css/default.css";
	}
	
	QFile file(sFile);
	if (file.exists()) {
		file.open(QIODevice::ReadOnly);
		QString ssheet = QString::fromLocal8Bit(file.readAll());
		setStyleSheet(ssheet);
	}
	else {
		std::cerr << "Stylesheet file " << sFile.toStdString() << " not found." << std::endl;
	}
	
	// Set up UI connections.
	// Menu
	connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
	connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
	connect(ui->actionFile, SIGNAL(triggered()), this, SLOT(castFile()));
	connect(ui->actionURL, SIGNAL(triggered()), this, SLOT(castUrl()));
	
	// UI
	connect(ui->editRemotesButton, SIGNAL(clicked()), this, SLOT(openRemotesDialog()));
	connect(ui->refreshRemotesButton, SIGNAL(clicked()), this, SLOT(remoteListRefresh()));
	connect(ui->remotesComboBox, SIGNAL(currentIndexChanged(int)), 
															this, SLOT(playerRemoteChanged(int)));
	
	// Tabs
    // Player tab.
	connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addFile()));
	connect(ui->removeButton, SIGNAL(clicked()), this, SLOT(removeFile()));
    connect(ui->beginToolButton, SIGNAL(clicked()), this, SLOT(rewind()));
	connect(ui->endToolButton, SIGNAL(clicked()), this, SLOT(forward()));
	connect(ui->playToolButton, SIGNAL(clicked()), this, SLOT(play()));
	connect(ui->stopToolButton, SIGNAL(clicked()), this, SLOT(stop()));
	connect(ui->pauseToolButton, SIGNAL(clicked()), this, SLOT(pause()));
    connect(ui->soundToolButton, SIGNAL(clicked()), this, SLOT(mute()));
	connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(adjustVolume(int)));
	connect(ui->positionSlider, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
	connect(ui->cycleSubtitleButton, SIGNAL(clicked()), this, SLOT(cycleSubtitles()));
	connect(ui->cycleAudioButton, SIGNAL(clicked()), this, SLOT(cycleAudio()));
	connect(ui->cycleVideoButton, SIGNAL(clicked()), this, SLOT(cycleVideo()));
	
	// Apps tab.
	connect(ui->updateRemoteAppsButton, SIGNAL(clicked()), this, SLOT(appsListRefresh()));
	connect(ui->remoteAppLineEdit, SIGNAL(returnPressed()), this, SLOT(sendCommand()));
    
    // Apps (GUI) tab.
    connect(ui->appTabGuiHomeButton, SIGNAL(clicked()), this, SLOT(appsHome()));
    connect(ui->appTabGuiTextBrowser, SIGNAL(linkClicked(QUrl)), this, SLOT(anchorClicked(QUrl)));
    
    using namespace std::placeholders; 
    ui->appTabGuiTextBrowser->setResourceHandler(std::bind(&MainWindow::loadResource, this, _1));
    
    // Shares tab.
    connect(ui->sharesScanButton, SIGNAL(clicked()), this, SLOT(scanForShares()));
    connect(ui->sharesPlayButton, SIGNAL(clicked()), this, SLOT(playSelectedShare()));
	
	// General NC library.
	connect(this, SIGNAL(playbackStatusChange(uint32_t, NymphPlaybackStatus)), 
			this, SLOT(setPlaying(uint32_t, NymphPlaybackStatus)));
			
	// Set up playback controls.
	ui->pauseToolButton->setVisible(false);
	ui->beginToolButton->setVisible(false);
	ui->stopToolButton->setEnabled(false);
	ui->endToolButton->setVisible(false);
	
	// NymphCast client SDK callbacks.
	using namespace std::placeholders;
	client.setStatusUpdateCallback(std::bind(&MainWindow::statusUpdateCallback,	this, _1, _2));
    
    // Set values.
    ui->sharesTreeView->setModel(&sharesModel);
	
	// Ensure this path exists.
	QDir dir(appDataLocation);
	if (!dir.exists()) {
		dir.mkpath(".");
	}
	
#if defined(Q_OS_ANDROID)
	// Set the 'Android' Qt style.
	// FIXME: looks pretty horrid during testing. Disabling for now.
	//QApplication::setStyle(QStyleFactory::create("Android"));
	
	// Ensure we got all permissions.
	for (const QString &permission : permissions){
        QtAndroid::PermissionResult result = QtAndroid::checkPermission(permission);
        if (result == QtAndroid::PermissionResult::Denied) {
            QtAndroid::PermissionResultMap resultHash = 
									QtAndroid::requestPermissionsSync(QStringList({permission}));
            if (resultHash[permission] == QtAndroid::PermissionResult::Denied) {
                return;
			}
        }
    }
	
    // We need to redirect stdout/stderr. This requires starting a new thread here.
    start_logger(tag);
    
	// On Android platforms we read in the media files into the playlist as they are in standard
	// locations. This is also a work-around for QTBUG-83372: 
	// https://bugreports.qt.io/browse/QTBUG-83372
	
	// First, disable the 'add' and 'remove' buttons as these won't be used on Android.
	ui->addButton->setEnabled(false);
    ui->addButton->setVisible(false);
	ui->removeButton->setEnabled(false);
    ui->removeButton->setVisible(false);
	
	if(!QAndroidJniObject::isClassAvailable("com/nyanko/nymphcastplayer/NymphCast")) {
		qDebug() << "Java class is missing.";
		return;
	}
	
	// Next, read the local media files and add them to the list, sorting music and videos.
	//QStringList audio = QStandardPaths::locateAll(QStandardPaths::MusicLocation, QString());
	//QStringList video = QStandardPaths::locateAll(QStandardPaths::MoviesLocation, QString());
	QAndroidJniObject audioObj = QAndroidJniObject::callStaticObjectMethod(
                            "com/nyanko/nymphcastplayer/NymphCast",
							"loadAudio",
							"(Landroid/content/Context;)Ljava/util/ArrayList;",
							QtAndroid::androidContext().object());
		
	for (int i = 0; i < audioObj.callMethod<jint>("size"); ++i) {
		// Add item to the list.
		QAndroidJniObject track = audioObj.callObjectMethod("get", "(I)Ljava/lang/Object;", i);
		const QString title = track.callObjectMethod("getTitle", "()Ljava/lang/String;").toString();
		const QString album = track.callObjectMethod("getAlbum", "()Ljava/lang/String;").toString();
		const QString artist = track.callObjectMethod("getArtist", "()Ljava/lang/String;").toString();
		const QString path = track.callObjectMethod("getPath", "()Ljava/lang/String;").toString();
		QListWidgetItem *newItem = new QListWidgetItem;
		newItem->setText(artist + " - " + title);
		newItem->setData(Qt::UserRole, QVariant(path));
		ui->mediaListWidget->addItem(newItem);
		
		// Debug
		std::cout << path.toStdString() << std::endl;
	}
	
	QAndroidJniObject videoObj = QAndroidJniObject::callStaticObjectMethod(
                            "com/nyanko/nymphcastplayer/NymphCast",
							"loadVideo",
							"(Landroid/content/Context;)Ljava/util/ArrayList;",
							QtAndroid::androidContext().object());
		
	for (int i = 0; i < videoObj.callMethod<jint>("size"); ++i) {
		// Add item to the list.
		QAndroidJniObject track = videoObj.callObjectMethod("get", "(I)Ljava/lang/Object;", i);
		const QString title = track.callObjectMethod("getTitle", "()Ljava/lang/String;").toString();
		const QString album = track.callObjectMethod("getAlbum", "()Ljava/lang/String;").toString();
		const QString artist = track.callObjectMethod("getArtist", "()Ljava/lang/String;").toString();
		const QString path = track.callObjectMethod("getPath", "()Ljava/lang/String;").toString();
		QListWidgetItem *newItem = new QListWidgetItem;
		newItem->setText(artist + " - " + title);
		newItem->setData(Qt::UserRole, QVariant(path));
		ui->mediaListWidget->addItem(newItem);
		
		// Debug
		std::cout << path.toStdString() << std::endl;
	}
	/* for (int i = 0; i < video.size(); ++i) {
		// Add item to the list.
		QListWidgetItem *newItem = new QListWidgetItem;
		newItem->setText(video[i]);
		newItem->setData(Qt::UserRole, QVariant(video[i]));
		ui->mediaListWidget->addItem(newItem);
	}	 */
#else
	// Reload stored file paths, if any.
	QFile playlist;
	playlist.setFileName(appDataLocation + "/filepaths.conf");
	playlist.open(QIODevice::ReadOnly);
	QTextStream textStream(&playlist);
	QString line;
	while (!(line = textStream.readLine()).isNull()) {
		QFileInfo finf(line);
		QListWidgetItem *newItem = new QListWidgetItem;
		newItem->setText(finf.fileName());
		newItem->setData(Qt::UserRole, QVariant(line));
		ui->mediaListWidget->addItem(newItem);
	}
	
	playlist.close();
#endif

	// Obtain initial list of available remotes.
	remoteListRefresh();
	
	// Load any saved groups.
	loadGroups();
	addGroupsToRemotes();
	
	// Restore UI preferences.
	ui->singlePlayCheckBox->setChecked(settings.value("ui/singlePlayCheckBox", true).toBool());
	ui->repeatQueueCheckBox->setChecked(settings.value("ui/repeatQueueCheckBox", false).toBool());
}

MainWindow::~MainWindow() {
	QSettings settings;
	settings.setValue("ui/singlePlayCheckBox", ui->singlePlayCheckBox->isChecked());
	settings.value("ui/repeatQueueCheckBox", ui->repeatQueueCheckBox->isChecked());
	
	delete ui;
}


// --- STATUS UPDATE CALLBACK ---
void MainWindow::statusUpdateCallback(uint32_t handle, NymphPlaybackStatus status) {
	// Send the data along to the slot on the GUI thread.
	emit playbackStatusChange(handle, status);
}


// --- SET PLAYING ---
void MainWindow::setPlaying(uint32_t handle, NymphPlaybackStatus status) {
	// Check that the current remote (handle) is selected in the combobox. 
	// If not, just store the updated status.
	
	// Obtain selected ID.
	int index = ui->remotesComboBox->currentIndex();
	uint32_t id = ui->remotesComboBox->currentData().toUInt();
	
	bool init = false;
	if (index > separatorIndex) {
		NCRemoteGroup& group = groups[ui->remotesComboBox->currentData().toUInt()];
		if (group.remotes.size() < 1) { return; }
		
		if (group.remotes[0].init) {
			init = true;
			group.remotes[0].init = false;
		}
		
		group.remotes[0].status = status;
		if (groups[id].remotes[0].handle != handle) {
			return;
		}
	}
	else if (index < separatorIndex) {
		remotes[index].status = status;
		if (remotes[index].handle != handle) {
			return;
		}
		
		if (remotes[index].init) {
			init = true;
			remotes[index].init = false;
		}
	}
	
	updatePlayerUI(status, init);
}
	
	
// --- UPDATE PLAYER UI ---
void MainWindow::updatePlayerUI(NymphPlaybackStatus status, bool init) {
	// Update the UI.
	bool paused = false;
	if (status.status == NYMPH_PLAYBACK_STATUS_PLAYING) {
		ui->remoteStatusLabel->setText("Playing: " + QString::fromStdString(status.artist));
	}
	else if (status.status == NYMPH_PLAYBACK_STATUS_PAUSED) {
		ui->remoteStatusLabel->setText("Paused.");
		paused = true;
	}
	else {
		ui->remoteStatusLabel->setText("Stopped.");
	}

	// Manage muted state.
	if (muted && status.volume > 0) { 
		muted = false;
		ui->soundToolButton->setIcon(QIcon(":/icons/icons/high-volume.png"));
	}
	else if (!muted && status.volume == 0) {
		muted = true;
		ui->soundToolButton->setIcon(QIcon(":/icons/icons/mute.png"));
	}
	
	if (init) {
		// Initial callback for this remote on connect. Just set safe defaults.
		std::cout << "Initial remote status callback on connect." << std::endl;
		
		playingTrack = false;
		ui->playToolButton->setEnabled(true);
		ui->playToolButton->setVisible(true);
		ui->stopToolButton->setEnabled(false);
		ui->pauseToolButton->setEnabled(false);
		ui->pauseToolButton->setVisible(false);
		
		ui->durationLabel->setText("0:00 / 0:00");
		ui->positionSlider->setValue(0);
		
		ui->volumeSlider->setValue(status.volume);
			
	}
	else if (paused) {
		std::cout << "Paused playback..." << std::endl;
		
		// Remote player is paused. Resume using play button.
		ui->playToolButton->setEnabled(true);
		ui->playToolButton->setVisible(true);
		ui->stopToolButton->setEnabled(true);
		ui->pauseToolButton->setEnabled(false);
		ui->pauseToolButton->setVisible(false);
	}
	else if (status.playing) {
        std::cout << "Status: Set playing..." << std::endl;
		
		// Set to true in case we're playing from a share.
		playingTrack = true;
		
		// Remote player is active. Read out 'status.status' to get the full status.
		ui->playToolButton->setEnabled(false);
		ui->playToolButton->setVisible(false);
		ui->stopToolButton->setEnabled(true);
		ui->pauseToolButton->setEnabled(true);
		ui->pauseToolButton->setVisible(true);
		
		// Set position & duration.
		QTime position(0, 0);
		position = position.addSecs((int64_t) status.position);
		QTime duration(0, 0);
		duration = duration.addSecs(status.duration);
		ui->durationLabel->setText(position.toString("hh:mm:ss") + " / " + 
														duration.toString("hh:mm:ss"));
														
		ui->positionSlider->setValue((status.position / status.duration) * 100);
		
		ui->volumeSlider->setValue(status.volume);
	}
	else {
        std::cout << "Status: Set not playing..." << std::endl;
		
		// Remote player is not active.
		ui->playToolButton->setEnabled(true);
		ui->playToolButton->setVisible(true);
		ui->stopToolButton->setEnabled(false);
		ui->pauseToolButton->setEnabled(false);
		ui->pauseToolButton->setVisible(false);
		
		ui->durationLabel->setText("0:00 / 0:00");
		ui->positionSlider->setValue(0);
		
		ui->volumeSlider->setValue(status.volume);
		
		if (singleCast) {
			singleCast = false;
			playingTrack = false;
		}
		else if (playingTrack) {
            playingTrack = false;
            std::cout << "Status: Playing track, check autoplay..." << std::endl;
			// We finished playing the currently selected track.
			// If auto-play is on, play the next track.
            if (ui->singlePlayCheckBox->isChecked() == false) {
                std::cout << "Status: Move to next track..." << std::endl;
                // Next track.
                int crow = ui->mediaListWidget->currentRow();
                if (++crow == ui->mediaListWidget->count()) {
					if (ui->repeatQueueCheckBox->isChecked() == false) {
						// Don't repeat playlist. End playback.
						std::cout << "Repeat Queue checkbox is false. End playback." << std::endl;
						return;
					}
					
                    // Restart at top.
                    crow = 0;
                }
                
                std::cout << "Status: Start playing track index " << crow << "..." << std::endl;
                
                ui->mediaListWidget->setCurrentRow(crow);
                play();
            }
			else if (ui->repeatQueueCheckBox->isChecked() == true) {
				// Play the currently selected song. If it's the same item as last play, it repeats.
				std::cout << "Play currently selected track (again)." << std::endl;
				play();
			}
		}
	}
}


// --- CONNECT REMOTE ---
bool MainWindow::connectRemote(NCRemoteInstance &instance) {
	if (instance.connected) { return true; }
	
	// Connect to NymphRPC server, standard port.
	if (!client.connectServer(instance.remote.ipv4, 0, instance.handle)) {
		QMessageBox::warning(this, tr("Failed to connect"), tr("The selected server could not be connected to."));
		return false;
	}
	
	// Successful connect.
	instance.connected = true;
	
	return true;
}


// --- DISCONNECT REMOTE ---
bool MainWindow::disconnectRemote(NCRemoteInstance &instance) {
	if (!instance.connected) { return true; }
	
	client.disconnectServer(instance.handle);
	
	instance.connected = false;
	
	return true;
}


// --- REMOTE LIST REFRESH ---
// Refresh the list of remote servers on the network.
void MainWindow::remoteListRefresh() {
	// Get the current list.
	std::vector<NymphCastRemote> list = client.findServers();
	
	// Update the list with any changed items.
	remotes.clear();
	ui->remotesComboBox->clear();
	for (uint32_t i = 0; i < list.size(); ++i) {
		QListWidgetItem *newItem = new QListWidgetItem;
		newItem->setText(QString::fromStdString(list[i].ipv4 + " (" + list[i].name + ")"));
		newItem->setData(Qt::UserRole, QVariant(i));
		ui->remotesComboBox->insertItem(i, QString::fromStdString(list[i].ipv4 + 
													" (" + list[i].name + ")"), QVariant(i));
													
		// Add remote to 'remotes' list.
		NCRemoteInstance nci;
		nci.remote = list[i];
		remotes.push_back(nci);
	}
	
	// Insert group separator for the relevant combo boxes.
	ui->remotesComboBox->insertSeparator(remotes.size());
	separatorIndex = remotes.size();
	
	// Add groups.
	for (uint32_t i = 0; i < groups.size(); ++i) {
		ui->remotesComboBox->addItem(QString::fromStdString(groups[i].name + 
													" (group)"), QVariant(i));
	}
}


// --- CAST FILE ---
void MainWindow::castFile() {
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
	
	// Open file.
	QString filename = QFileDialog::getOpenFileName(this, tr("Open media file"));
	if (filename.isEmpty()) { return; }
	
	if (client.castFile(handle, filename.toStdString())) {
		// Playing back file now. Update status.
		playingTrack = true;
		singleCast = true;
	}
	else {
		QMessageBox::warning(this, tr("Failed to cast file"), tr("The selected file could not be played back."));
	}
}


// --- CAST URL ---
void MainWindow::castUrl() {
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
	
	// Get URL.
	QString url = QInputDialog::getText(this, tr("Cast URL"), tr("Copy in the URL to cast."));
	if (url.isEmpty()) { return; }
	
	std::string surl = url.toStdString();
	if (client.castUrl(handle, surl)) {
		// Playing back URL now. Update status.
		playingTrack = true;
		singleCast = true;
	}
	else {
		QMessageBox::warning(this, tr("Failed to cast URL"), tr("The URL could not be played back."));
	}
}


// --- PLAYER REMOTE CHANGED ---
void MainWindow::playerRemoteChanged(int index) {
	if (index < 0) {
		return;
	}
	
	// Obtain selected ID.
	uint32_t id = ui->remotesComboBox->currentData().toUInt();
	
	// Make sure remote is connected and update the status display.
	if (index > separatorIndex) {
		NCRemoteGroup& group = groups[id];
		if (group.remotes.size() < 1) { return; }
		
		if (!group.remotes[0].connected) {
			remoteIsConnected();
		}
		else {
			updatePlayerUI(group.remotes[0].status);
		}
	}
	else if (index < separatorIndex) {
		if (!remotes[index].connected) {
			remoteIsConnected();
		}
		else {
			updatePlayerUI(remotes[index].status);
		}
	}
}


// --- ADD FILE ---
void MainWindow::addFile() {
	// Select file from filesystem, add to playlist.
	// Use stored directory location, if any.
	QSettings settings;
	QString dir = settings.value("openFileDir").toString();
	QString filename = QFileDialog::getOpenFileName(this, tr("Open media file"), dir);
	if (filename.isEmpty()) { return; }
	
	// Update current folder.
	settings.setValue("openFileDir", filename);
	
	// Check file.
	QFileInfo finf(filename);
	if (!finf.isFile()) { 
		QMessageBox::warning(this, tr("Failed to open file"), tr("The selected file could not be opened."));
		return;
	}
	
	// Add it.
	QListWidgetItem *newItem = new QListWidgetItem;
	newItem->setText(finf.fileName());
	newItem->setData(Qt::UserRole, QVariant(filename));
	ui->mediaListWidget->addItem(newItem);
	
	// Store path of added file to reload on restart. Append to file.
	QFile playlist;
	playlist.setFileName(appDataLocation + "/filepaths.conf");
	playlist.open(QIODevice::WriteOnly | QIODevice::Append);
	QTextStream textStream(&playlist);
	textStream << filename << "\n";
	playlist.close();
}


// --- REMOVE FILE ---
void MainWindow::removeFile() {
	// Remove currently selected filename(s) from the playlist.
	QListWidgetItem* item = ui->mediaListWidget->currentItem();
	ui->mediaListWidget->removeItemWidget(item);
	delete item;
	
	// TODO: Remove stored file path.
	// FIXME: Clear and rewrite the file contents for now.
	QFile playlist;
	playlist.setFileName(appDataLocation + "filepaths.conf");
	playlist.open(QIODevice::WriteOnly | QIODevice::Truncate);
	int size = ui->mediaListWidget->count();
	QTextStream textStream(&playlist);
	for (int i = 0; i < size; ++i) {
		QListWidgetItem* item = ui->mediaListWidget->item(i);
		QString filename = item->data(Qt::UserRole).toString();
		textStream << filename << "\n";
	}
	
	playlist.close();
}


// --- REMOTE IS CONNECTED ---
// Return true if the currently selected remote in the Player tab is connected.
bool MainWindow::remoteIsConnected() {
	if (ui->remotesComboBox->count() == 0) {
		QMessageBox::warning(this, tr("No remotes available"), 
						tr("Please check that receivers are available and refresh the list."));
		return false;
	}
	else if (ui->remotesComboBox->currentIndex() == -1) {
		QMessageBox::warning(this, tr("No remote selected"), tr("Please select a target receiver."));
		return false;
	}
	
	// Obtain selected ID.
	int index = ui->remotesComboBox->currentIndex();
	
	// Check that a remote and not a group has been selected.
	if (index >= separatorIndex) {
		return false;
	}
	
	// Ensure the remote is connected.
	if (!remotes[index].connected) {
		if (!connectRemote(remotes[index])) { return false; }
	}
	
	return true;
}


// --- PLAY ---
void MainWindow::play() {
	// Play selected track back on selected remote. Connect if necessary.
	// Immediately fail if:
	// * No remotes available.
	// * No remote or group selected.
	
	// Get currently selected remote or group, connect if necessary.
	if (ui->remotesComboBox->count() == 0) {
		QMessageBox::warning(this, tr("No remotes found"), tr("Please refresh and try again."));
		return;
	}
	else if (ui->remotesComboBox->currentIndex() == -1) {
		QMessageBox::warning(this, tr("No remote selected"), tr("Please select a target receiver."));
		return;
	}
	
	// Obtain selected ID.
	int index = ui->remotesComboBox->currentIndex();
	
	// If a group is selected, pick the first remote in the group as master.
	if (index > separatorIndex) {
		// Get the group and set up the master & any slave receivers.
		NCRemoteGroup& group = groups[ui->remotesComboBox->currentData().toUInt()];
		if (group.remotes.size() < 1) {
			// Error: no remotes in group.
			QMessageBox::warning(this, tr("No remotes"), tr("Group contains no remotes."));
			return;
		}
		
		if (!group.remotes[0].connected) {
			if (!connectRemote(group.remotes[0])) {
				QMessageBox::warning(this, tr("Connect fail."), tr("Unable to connect to group."));
				return;
			}
		}
		
		// Start playing the currently selected track if it isn't already playing. 
		// Else pause or unpause playback.
		if (playingTrack) {
			client.playbackStart(group.remotes[0].handle);
		}
		else {
			QListWidgetItem* item = ui->mediaListWidget->currentItem();
			if (item == 0) { 
				QMessageBox::warning(this, tr("No file selected"), tr("Please first select a file to play."));
				return; 
			}
			
			QString filename = item->data(Qt::UserRole).toString();
			
			// Add the slave receivers.
			if (group.remotes.size() > 1) {
				std::vector<NymphCastRemote> slaves;
				for (uint32_t i = 1; i < (uint32_t) group.remotes.size(); ++i) {
					slaves.push_back(group.remotes[i].remote);
				}
				
				if (!client.addSlaves(group.remotes[0].handle, slaves)) {
					QMessageBox::warning(this, tr("Failed to add slaves"), tr("Please check that all remotes in the group are online."));
					return;
				}
			}
			
			if (client.castFile(group.remotes[0].handle, filename.toStdString())) {
				// Playing back file now. Update status.
			   playingTrack = true;
			}
		}
		
		return;
	}
	
	// Ensure the remote is connected.
	if (!remotes[index].connected) {
		if (!connectRemote(remotes[index])) { return; }
	}
	
	// Start playing the currently selected track if it isn't already playing. 
	// Else pause or unpause playback.
	if (playingTrack) {
		client.playbackStart(remotes[index].handle);
	}
	else {
		QListWidgetItem* item = ui->mediaListWidget->currentItem();
		if (item == 0) { 
			QMessageBox::warning(this, tr("No file selected"), tr("Please first select a file to play."));
			return; 
		}
		
		QString filename = item->data(Qt::UserRole).toString();
		
        if (client.castFile(remotes[index].handle, filename.toStdString())) {
            // Playing back file now. Update status.
           playingTrack = true;
        }
	}
	
}


// --- STOP ---
void MainWindow::stop() {
	if (!playingTrack) { return; }
	
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
    
    playingTrack = false;
	client.playbackStop(handle);
}


// --- PAUSE ---
void MainWindow::pause() {
	if (!playingTrack) { return; }
	
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
	
	client.playbackPause(handle);
}


// --- FORWARD ---
void MainWindow::forward() {
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
	
	client.playbackForward(handle);
}


// --- REWIND ---
void MainWindow::rewind() {
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
	
	client.playbackRewind(handle);
}


// --- SEEK ---
void MainWindow::seek(int value) {
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
	
	// Seek bar is in percentages. Pick the right function with a cast.
	client.playbackSeek(handle, (uint8_t) value);
}


// --- MUTE ---
void MainWindow::mute() {
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
	
	if (!muted) {
		client.volumeSet(handle, 0);
		muted = true;
		ui->soundToolButton->setIcon(QIcon(":/icons/icons/mute.png"));
	}
	else {
		client.volumeSet(handle, ui->volumeSlider->value());
		muted = false;
		ui->soundToolButton->setIcon(QIcon(":/icons/icons/high-volume.png"));
	}
}


// --- ADJUST VOLUME ---
void MainWindow::adjustVolume(int value) {
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
	
	if (value < 0 || value > 128) { return; }
	
	client.volumeSet(handle, value);
}


// --- CYCLE SUBTITLES ---
void MainWindow::cycleSubtitles() {
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
	
	client.cycleSubtitles(handle);
}


// --- CYCLE AUDIO ---
void MainWindow::cycleAudio() {
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
	
	client.cycleAudio(handle);
}


// --- CYCLE VIDEO ---
void MainWindow::cycleVideo() {
	uint32_t handle;
	if (!remoteEnsureConnected(handle)) { return; }
	
	client.cycleVideo(handle);
}


// --- PLAYER ENSURE CONNECTED ---
bool MainWindow::remoteEnsureConnected(uint32_t &handle) {
	// Get currently selected remote, connect if necessary.
	if (ui->remotesComboBox->count() == 0) {
		QMessageBox::warning(this, tr("No remote selected"), tr("Please select a target receiver."));
		return false;
	}
	else if (ui->remotesComboBox->currentIndex() == -1) {
		QMessageBox::warning(this, tr("No remote selected"), tr("Please select a target receiver."));
		return false;
	}
	
	// Obtain selected ID.
	int32_t index = ui->remotesComboBox->currentIndex();
	uint32_t id = ui->remotesComboBox->currentData().toUInt();
	
	if (index > separatorIndex) {
		NCRemoteGroup& group = groups[id];
		handle = group.remotes[0].handle;
		if (!group.remotes[0].connected) {
			if (!connectRemote(group.remotes[0])) { return false; }
		}
	}
	else {
		// Ensure the remote is connected.
		handle = remotes[id].handle;
		if (!remotes[id].connected) {
			if (!connectRemote(remotes[id])) { return false; }
		}
	}
	
	return true;
}


// --- APPS LIST REFRESH ---
void MainWindow::appsListRefresh() {
	uint32_t ncid;
	if (!remoteEnsureConnected(ncid)) { return; }
	
	// Get list of apps from the remote server.
	std::string appList = client.getApplicationList(remotes[ncid].handle);
	
	// Update local list.
	ui->remoteAppsComboBox->clear();
	QStringList appItems = (QString::fromStdString(appList)).split("\n", QString::SkipEmptyParts);
	ui->remoteAppsComboBox->addItems(appItems);
}


// --- SEND COMMAND ---
void MainWindow::sendCommand() {
	uint32_t ncid;
	if (!remoteEnsureConnected(ncid)) { return; }
	
	// Read the data in the line edit and send it to the remote app.
	// Get the appID from the currently selected item in the app list combobox.
	QString currentItem = ui->remoteAppsComboBox->currentText();
	
	std::string appId = currentItem.toStdString();
	std::string message = ui->remoteAppLineEdit->text().toStdString();
	
	// Append the command to the output field.
	ui->remoteAppTextEdit->appendPlainText(ui->remoteAppLineEdit->text());
	ui->remoteAppTextEdit->appendPlainText("\n");
	
	// Clear the input field.
	ui->remoteAppLineEdit->clear();
	
	std::string response = client.sendApplicationMessage(remotes[ncid].handle, appId, message);
	
	// Append the response to the output field.
	ui->remoteAppTextEdit->appendPlainText(QString::fromStdString(response));
	ui->remoteAppTextEdit->appendPlainText("\n");
}


// --- APPS HOME ---
void MainWindow::appsHome() {
	uint32_t ncid;
    if (!remoteEnsureConnected(ncid)) { return; }
    
    // Request the starting Apps page from the remote.
	std::string home = std::string();
	std::string resource = "apps.html";
    QString page = QString::fromStdString(client.loadResource(remotes[ncid].handle, home, 
                                                                               resource));
    
    
    // Set the received HTML into the target widget.
    ui->appTabGuiTextBrowser->setHtml(page);
}


// --- ANCHOR CLICKED ---
void MainWindow::anchorClicked(const QUrl &link) {
	// Debug
	std::cout << "anchorClicked: " << link.path().toStdString() << std::endl;
	
	uint32_t ncid;
    if (!remoteEnsureConnected(ncid)) { return; }
	
    // Parse URL string for the command desired.
    QStringList list = link.path().split("/", QString::SkipEmptyParts);
    
    // Process command.
    if (list.size() < 1) { return; }
    
    if (list[0] == "start") {
        // Start an app here, which should be listed in the second slot.
        if (list.size() < 2) { return; }
        
        // Try to load the index page for the specified app.
		std::string appId = list[1].toStdString();
		std::string resource = "index.html";
        QString page = QString::fromStdString(client.loadResource(remotes[ncid].handle, 
                                                                  appId, 
                                                                   resource));
        
        if (page.isEmpty()) { 
            QMessageBox::warning(this, tr("Failed to start"), tr("The selected app could not be started."));
            return; 
        }
        
        // Set the received HTML into the target widget.
        ui->appTabGuiTextBrowser->setHtml(page);
    }
    else {
		// If the first entry contains the string 'requestText', request text input from the user.
		// This string is appended to the end of the command string to the app.
		if (list.size() < 2) { return; }
		std::string cmdStr;
		std::string appStr;
		if (list[0] == "requestText") {
			if (list.size() < 3) { return; }
			QString txt = QInputDialog::getText(this, tr("Input text"), tr("Text input requested by app."));
			appStr = list[1].toStdString();
			
			// Merge index 2 until the end into a single space-separated string.
			for (int i = 2; i < list.size(); i++) {
				cmdStr += list[i].toStdString() + " ";
			}
			
			cmdStr += txt.toStdString();
		}
		else {
			// Assume the first entry contains an app name, followed by commands.
			appStr = list[0].toStdString();
			
			// TODO: merge commands until the end into a single space-separated string.
			for (int i = 1; i < list.size(); i++) {
				cmdStr += list[i].toStdString() + " ";
			}
			
			cmdStr.pop_back(); // Remove last space character.
		}
		
		// TODO: validate app names here.
		// TODO: Use client method that returns HTML.
		std::string response = client.sendApplicationMessage(remotes[ncid].handle, 
																appStr, 
																cmdStr);
																
		// Response contains the new HTML to display. Load this into the view.
		ui->appTabGuiTextBrowser->setHtml(QString::fromStdString(response));
    }
}


// --- LOAD RESOURCE ---
QByteArray MainWindow::loadResource(const QUrl &name) {
	uint32_t ncid;
    if (!remoteEnsureConnected(ncid)) { return QByteArray(); }
	
    // Parse the URL for the desired resource.
    QFileInfo dir(name.path());
    QString qAppId = dir.path();
    std::string filename = name.fileName().toStdString();
    
    // FIXME: Hack to deal with weird QLiteHtml behaviour with relative URLs.
    if (qAppId.startsWith("/")) {
        qAppId.remove(0, 1);
    }
    
    std::string appId = qAppId.toStdString();
    
    QByteArray page = QByteArray::fromStdString(client.loadResource(remotes[ncid].handle, appId, filename));
    return page;
}


// --- SCAN FOR SHARES ---
void MainWindow::scanForShares() {
    // Scan for media server instances on the network.
    std::vector<NymphCastRemote> mediaservers = client.findShares();
    if (mediaservers.empty()) {
        QMessageBox::warning(this, tr("No media servers found."), tr("No media servers found."));
        return;
    }
    
    // For each media server, request the shared file list.
    sharesModel.clear();
    mediaFiles.clear();
    QStandardItem* parentItem = sharesModel.invisibleRootItem();
    for (uint32_t i = 0; i < mediaservers.size(); ++i) {
        std::vector<NymphMediaFile> files = client.getShares(mediaservers[i]);
        if (files.empty()) { continue; }
        
        // Insert into model. Use the media server's host name as top folder, with the shared
        // files inserted underneath it.
        QStandardItem* item = new QStandardItem(QString::fromStdString(mediaservers[i].name));
        item->setSelectable(false);
        for (uint32_t j = 0; j < files.size(); ++j) {
			if (files[j].type == FILE_TYPE_IMAGE) { continue; }
            QStandardItem* fn = new QStandardItem(QString::fromStdString(files[j].name));
            QList<QVariant> ids;
			ids.append(QVariant(i));
			ids.append(QVariant(j));
            ids.append(QVariant(files[j].id));
            mediaFiles.push_back(files);
            ids.append(QVariant((uint32_t) mediaFiles.size()));
            
            fn->setData(QVariant(ids), Qt::UserRole);
            item->appendRow(fn);
        }
        
        parentItem->appendRow(item);
    }
}


// --- PLAY SELECTED SHARE --
void MainWindow::playSelectedShare() {
	
	//uint32_t handle;
    //if (!remoteEnsureConnected(handle)) { return; }
    
    // Get the currently selected file name and obtain the ID.
    QModelIndexList indexes = ui->sharesTreeView->selectionModel()->selectedIndexes();
    if (indexes.size() == 0) {
        QMessageBox::warning(this, tr("No file selected"), tr("No media files present."));
        return;
    }
    else if (indexes.size() > 1) {
        QMessageBox::warning(this, tr("No file selected."), tr("No media file selected."));
        return;
    }
    
    QMap<int, QVariant> data = sharesModel.itemData(indexes[0]);
    QList<QVariant> ids = data[Qt::UserRole].toList();
	
	int index = ui->remotesComboBox->currentIndex();
	
    std::vector<NymphCastRemote> receivers;
	if (index > separatorIndex) {
		// Get the group and set up the master & any slave receivers.
		NCRemoteGroup& group = groups[ui->remotesComboBox->currentData().toUInt()];
		if (group.remotes.size() < 1) {
			// Error: no remotes in group.
			QMessageBox::warning(this, tr("No remotes"), tr("Group contains no remotes."));
			return;
		}
			
		// Add the receivers.
		for (uint32_t i = 0; i < (uint32_t) group.remotes.size(); ++i) {
			receivers.push_back(group.remotes[i].remote);
		}
	}
	else if (index < separatorIndex) {
		uint32_t ncid = ui->remotesComboBox->currentIndex();
		receivers.push_back(remotes[ncid].remote);
	}
	else {
		// Separator was selected?!
		return;
	}
    
    // Play file via media server.
    if (!client.playShare(mediaFiles[ids[0].toInt()][ids[1].toInt()], receivers)) {
         //
    }
}


// --- OPEN REMOTES DIALOG ---
void MainWindow::openRemotesDialog() {
	rd = new RemotesDialog;
	connect(rd, SIGNAL(updateRemotesList()), this, SLOT(updateRemotesList()));
	connect(rd, SIGNAL(saveGroupsList(std::vector<NCRemoteGroup>&)),
				this, SLOT(updateGroupsList(std::vector<NCRemoteGroup>&)));
	
	// Set data.
	rd->setRemoteList(remotes);
	rd->updateGroupsList(groups);
	
	std::cout << "Set data on RD dialogue." << std::endl;
	
	// Execute dialog.
	rd->exec();
	
	std::cout << "Finished RD dialogue." << std::endl;
	
	delete rd;
}


// --- UPDATE REMOTES LIST ---
void MainWindow::updateRemotesList() {
	std::cout << "UpdateRemotesList() called." << std::endl;
	remoteListRefresh();
	rd->setRemoteList(remotes);
}


// --- UPDATE GROUPS LIST ---
void MainWindow::updateGroupsList(std::vector<NCRemoteGroup> &groups) {
	this->groups = groups;
	
	std::cout << "Called updateGroupsList." << std::endl;
	
	addGroupsToRemotes();
	
	// Save new groups to disk.
	saveGroups();
}
	

// --- ADD GROUPS TO REMOTES ---
void MainWindow::addGroupsToRemotes() {
	// Update remotes combo boxes.
	// Only update the combo boxes on the Player & Shares tabs as these use groups.
	ui->remotesComboBox->clear();
	for (uint32_t i = 0; i < remotes.size(); ++i) {
		QListWidgetItem *newItem = new QListWidgetItem;
		newItem->setText(QString::fromStdString(remotes[i].remote.ipv4 + " (" + remotes[i].remote.name + ")"));
		newItem->setData(Qt::UserRole, QVariant(i));
		ui->remotesComboBox->insertItem(i, QString::fromStdString(remotes[i].remote.ipv4 + 
													" (" + remotes[i].remote.name + ")"), QVariant(i));
	}
	
	// Insert group separator for the relevant combo boxes.
	ui->remotesComboBox->insertSeparator(remotes.size());
	separatorIndex = remotes.size();
	
	// Add groups.
	for (uint32_t i = 0; i < groups.size(); ++i) {
		ui->remotesComboBox->addItem(QString::fromStdString(groups[i].name + 
													" (group)"), QVariant(i));
	}
}


// --- ABOUT ---
void MainWindow::about() {
	QMessageBox::about(this, tr("About NymphCast Player."), tr("NymphCast Player is the reference player for NymphCast.\n\nRev.: v0.1-alpha5"));
}


// --- QUIT ---
void MainWindow::quit() {
	exit(0);
}


// --- LOAD GROUPS ---
// Load the remote groups from file, if exists.
bool MainWindow::loadGroups() {
	// Open the groups file.
	QFile file(appDataLocation + "/groups.bin");
	if (!file.exists()) { return false; }
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(this, tr("Error loading groups"), tr("Error reading the groups.bin file!"));
		return false;
	}
	
	// Read in the groups.
	QDataStream ds(&file);
	// First read in the number of groups.
	quint32 numGroups;
	ds >> numGroups;
	// Read group name, then the number of remotes in the group, followed by the remotes.
	for (uint32_t i = 0; i < numGroups; ++i) {
		QString gname;
		quint32 numRemotes;
		ds >> gname;
		ds >> numRemotes;
		
		NCRemoteGroup rg;
		rg.name = gname.toStdString();
		
		for (uint32_t j = 0; j < numRemotes; ++j) {
			QString rname;
			QString ipv4;
			QString ipv6;
			quint16 port;
			
			ds >> rname;
			ds >> ipv4;
			ds >> ipv6;
			ds >> port;
			
			NymphCastRemote ncr;
			ncr.name = rname.toStdString();
			ncr.ipv4 = ipv4.toStdString();
			ncr.ipv6 = ipv6.toStdString();
			ncr.port = (uint16_t) port;
			
			NCRemoteInstance nci;
			nci.remote = ncr;
			
			rg.remotes.push_back(nci);
		}
		
		groups.push_back(rg);
	}
	
	return true;
}


// --- SAVE GROUPS ---
// Save the remote groups to file.
bool MainWindow::saveGroups() {
	std::cout << "SaveGroups called." << std::endl;
	
	// Open the groups file.
	QFile file(appDataLocation + "/groups.bin");
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QMessageBox::warning(this, tr("Error saving groups"), tr("Error opening the groups.bin file!"));
		return false;
	}
	
	// Write the groups.
	QDataStream ds(&file);
	ds << (quint32) groups.size(); // Number of groups.
	for (uint32_t i = 0; i < groups.size(); ++i) {
		// Write group name, then the number of remotes in the group, followed by the remotes.
		ds << QString::fromStdString(groups[i].name);
		ds << (quint32) groups[i].remotes.size();
		for (uint32_t j = 0; j < groups[i].remotes.size(); ++j) {
			ds << QString::fromStdString(groups[i].remotes[j].remote.name);
			ds << QString::fromStdString(groups[i].remotes[j].remote.ipv4);
			ds << QString::fromStdString(groups[i].remotes[j].remote.ipv6);
			ds << (quint16) groups[i].remotes[j].remote.port;
		}
	}
	
	return true;
}
