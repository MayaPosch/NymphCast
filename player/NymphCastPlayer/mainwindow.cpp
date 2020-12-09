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
#include <QSettings>
#include <QStringList>
#include <QStandardPaths>
#include <QDir>


// Static declarations
uint32_t MainWindow::serverHandle;
NymphCastClient MainWindow::client;

	
Q_DECLARE_METATYPE(NymphPlaybackStatus);


MainWindow::MainWindow(QWidget *parent) :	 QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);
	
	// Register custom types.
	qRegisterMetaType<NymphPlaybackStatus>("NymphPlaybackStatus");
	qRegisterMetaType<uint32_t>("uint32_t");
	
	// Set application options.
	QCoreApplication::setApplicationName("NymphCast Player");
	QCoreApplication::setApplicationVersion("v0.1-alpha");
	QCoreApplication::setOrganizationName("Nyanko");
	
	// Set configured or default stylesheet. Read out current value.
	// Skip stylesheet if file isn't found.
	QSettings settings;
	if (!settings.contains("stylesheet")) {
		settings.setValue("stylesheet", "default.css");
	}
	
	QString sFile = settings.value("stylesheet", "default.css").toString();
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
	connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(connectServer()));
	connect(ui->actionDisconnect, SIGNAL(triggered()), this, SLOT(disconnectServer()));
	connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
	connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
	connect(ui->actionFile, SIGNAL(triggered()), this, SLOT(castFile()));
	connect(ui->actionURL, SIGNAL(triggered()), this, SLOT(castUrl()));
	
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
	connect(ui->positionSlider, SIGNAL(sliderReleased()), this, SLOT(seek()));
    
    // Remotes tab.
	connect(ui->refreshRemotesToolButton, SIGNAL(clicked()), this, SLOT(remoteListRefresh()));
	connect(ui->connectToolButton, SIGNAL(clicked()), this, SLOT(remoteConnectSelected()));
	connect(ui->disconnectToolButton, SIGNAL(clicked()), this, SLOT(remoteDisconnectSelected()));
	
	// Apps tab.
	connect(ui->updateRemoteAppsButton, SIGNAL(clicked()), this, SLOT(appsListRefresh()));
	connect(ui->remoteAppLineEdit, SIGNAL(returnPressed()), this, SLOT(sendCommand()));
    
    // Apps (GUI) tab.
    connect(ui->appTabGuiHomeButton, SIGNAL(clicked()), this, SLOT(appsHome()));
    //connect(ui->appTabGuiTextBrowser, SIGNAL(anchorClicked(QUrl)), this, SLOT(anchorClicked(QUrl)));
    connect(ui->appTabGuiTextBrowser, SIGNAL(linkClicked(QUrl)), this, SLOT(anchorClicked(QUrl)));
    
    using namespace std::placeholders; 
    ui->appTabGuiTextBrowser->setResourceHandler(std::bind(&MainWindow::loadResource, this, _1));
	
	connect(this, SIGNAL(playbackStatusChange(uint32_t, NymphPlaybackStatus)), 
			this, SLOT(setPlaying(uint32_t, NymphPlaybackStatus)));
	
	// NymphCast client SDK callbacks.
	using namespace std::placeholders;
	client.setStatusUpdateCallback(std::bind(&MainWindow::statusUpdateCallback,	this, _1, _2));
	
#if defined(Q_OS_ANDROID)
	// On Android platforms we read in the media files into the playlist as they are in standard
	// locations. This is also a work-around for QTBUG-83372: 
	// https://bugreports.qt.io/browse/QTBUG-83372
	
	// First, disable the 'add' and 'remove' buttons as these won't be used on Android.
	ui->addButton->setEnabled(false);
	ui->removeButton->setEnabled(false);
	
	// Next, read the local media files and add them to the list, sorting music and videos.
	QStringList audio = QStandardPaths::locateAll(QStandardPaths::MusicLocation, QString());
	QStringList video = QStandardPaths::locateAll(QStandardPaths::MoviesLocation, QString());
	
	for (int i = 0; i < audio.size(); ++i) {
		// Add item to the list.
		QListWidgetItem *newItem = new QListWidgetItem;
		newItem->setText(audio[i]);
		newItem->setData(Qt::UserRole, QVariant(audio[i]));
		ui->mediaListWidget->addItem(newItem);
	}
	
	for (int i = 0; i < video.size(); ++i) {
		// Add item to the list.
		QListWidgetItem *newItem = new QListWidgetItem;
		newItem->setText(video[i]);
		newItem->setData(Qt::UserRole, QVariant(video[i]));
		ui->mediaListWidget->addItem(newItem);
	}	
	
#endif
}

MainWindow::~MainWindow() {
	delete ui;
}


// --- STATUS UPDATE CALLBACK ---
void MainWindow::statusUpdateCallback(uint32_t handle, NymphPlaybackStatus status) {
	// Send the data along to the slot on the GUI thread.
	emit playbackStatusChange(handle, status);
}


// --- SET PLAYING ---
void MainWindow::setPlaying(uint32_t /*handle*/, NymphPlaybackStatus status) {
	if (status.playing) {
		// Remote player is active. Read out 'status.status' to get the full status.
		ui->playToolButton->setEnabled(false);
		ui->stopToolButton->setEnabled(true);
		
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
		// Remote player is not active.
		ui->playToolButton->setEnabled(true);
		ui->stopToolButton->setEnabled(false);
		
		ui->durationLabel->setText("0:00 / 0:00");
		ui->positionSlider->setValue(0);
		ui->volumeSlider->setValue(0);
	}
}


void MainWindow::connectServer() {
	if (connected) { return; }
	
	// Ask for the IP address of the server.
	QString ip = QInputDialog::getText(this, tr("NymphCast Receiver"), tr("Please provide the NymphCast receiver IP address."));
	if (ip.isEmpty()) { return; }	
	
	// Connect to localhost NymphRPC server, standard port.
	if (!client.connectServer(ip.toStdString(), serverHandle)) {
		QMessageBox::warning(this, tr("Failed to connect"), tr("The selected server could not be connected to."));
		return;
	}
	
	// Update server name label.
	ui->remoteLabel->setText("Connected to " + ip);
	
	// Successful connect.
	connected = true;
}


// --- CONNECT SERVER IP ---
void MainWindow::connectServerIP(std::string ip) {
	if (connected) { return; }
	
	// Connect to localhost NymphRPC server, standard port.
	if (!client.connectServer(ip, serverHandle)) {
		QMessageBox::warning(this, tr("Failed to connect"), tr("The selected server could not be connected to."));
		return;
	}
	
	// TODO: update server name label.
	ui->remoteLabel->setText("Connected to " + QString::fromStdString(ip));
	
	// Successful connect.
	connected = true;
}


// --- DISCONNECT SERVER ---
void MainWindow::disconnectServer() {
	if (!connected) { return; }
	
	client.disconnectServer(serverHandle);
	
	ui->remoteLabel->setText("Disconnected.");
	
	connected = false;
}


// --- REMOTE LIST REFRESH ---
// Refresh the list of remote servers on the network.
void MainWindow::remoteListRefresh() {
	// Get the current list.
	remotes = client.findServers();
	
	// Update the list with any changed items.
	// Target the 'remotesListWidget' widget.
	ui->remotesListWidget->clear(); // FIXME: just resetting the whole thing for now.
	for (int i = 0; i < remotes.size(); ++i) {
		//new QListWidgetItem(remotes[i].ipv4 + " (" + remotes[i].name + ")", ui->remotesListWidget);
		QListWidgetItem *newItem = new QListWidgetItem;
		newItem->setText(QString::fromStdString(remotes[i].ipv4 + " (" + remotes[i].name + ")"));
		//newItem->setData(Qt::UserRole, QVariant(QString::fromStdString(remotes[i].ipv4)));
		newItem->setData(Qt::UserRole, QVariant(i));
		ui->remotesListWidget->insertItem(i, newItem);
	}
	
}


// --- REMOTE CONNECT SELECTED ---
// Connect to the selected remote server.
void MainWindow::remoteConnectSelected() {
	if (connected) { return; }
	
	//QListWidgetItem* item = ui->remotesListWidget->currentItem();
	//QString ip = item->data(Qt::UserRole).toString();
	QList<QListWidgetItem*> items = ui->remotesListWidget->selectedItems();
	
	if (items.size() == 0) { 
		QMessageBox::warning(this, tr("No selection"), tr("No remotes were selected."));
		return; 
	}
	
	// The first (index 0) remote is connected to as the master remote. Any further remotes are
	// sent to the master remote as slave remotes.
	
	// Connect to the server.
	//connectServerIP(ip.toStdString());
	int ref = items[0]->data(Qt::UserRole).toInt();
	connectServerIP(remotes[ref].ipv4);
	
	if (!connected || items.size() < 2) { return; }
	
	std::vector<NymphCastRemote> slaves;
	for (int i = 1; i < items.size(); ++i) {
		slaves.push_back(remotes[i]);
	}
	
	client.addSlaves(serverHandle, slaves);
}


// --- REMOTE DISCONNECT SELECTED ---
void MainWindow::remoteDisconnectSelected() {
	// FIXME: Redirect to the plain disconnectServer() function for now.
	// With the multi-server functionality implemented, this should disconnect the selected remote.
	disconnectServer();
}


// --- CAST FILE ---
void MainWindow::castFile() {
	if (!connected) { return; }
	
	// Open file.
	QString filename = QFileDialog::getOpenFileName(this, tr("Open media file"));
	if (filename.isEmpty()) { return; }
	
	client.castFile(serverHandle, filename.toStdString());
}


// --- CAST URL ---
void MainWindow::castUrl() {
	if (!connected) { return; }
	
	// Open file.
	QString url = QInputDialog::getText(this, tr("Cast URL"), tr("Copy in the URL to cast."));
	if (url.isEmpty()) { return; }
	
	client.castUrl(serverHandle, url.toStdString());
}


// --- ADD FILE ---
void MainWindow::addFile() {
	// Select file from filesystem, add to playlist.
	QString filename = QFileDialog::getOpenFileName(this, tr("Open media file"));
	if (filename.isEmpty()) { return; }
	
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
}


// --- REMOVE FILE ---
void MainWindow::removeFile() {
	// Remove currently selected filename(s) from the playlist.
	QListWidgetItem* item = ui->mediaListWidget->currentItem();
	ui->mediaListWidget->removeItemWidget(item);
	delete item;
}


// --- PLAY ---
void MainWindow::play() {
	if (!connected) { return; }
	
	// Start playing the currently selected track if it isn't already playing. 
	// Else pause or unpause playback.
	if (playingTrack) {
		client.playbackStart(serverHandle);
	}
	else {
		QListWidgetItem* item = ui->mediaListWidget->currentItem();
		QString filename = item->data(Qt::UserRole).toString();

#if defined(Q_OS_ANDROID)
		// Ensure we convert the path to 
#endif
		
		client.castFile(serverHandle, filename.toStdString());
	}
	
}


// --- STOP ---
void MainWindow::stop() {
	//
	if (!connected) { return; }
	
	client.playbackStop(serverHandle);
}


// --- PAUSE ---
void MainWindow::pause() {
	//
	if (!connected) { return; }
	
	client.playbackPause(serverHandle);
}


// --- FORWARD ---
void MainWindow::forward() {
	//
	if (!connected) { return; }
	
	client.playbackForward(serverHandle);
}


// --- REWIND ---
void MainWindow::rewind() {
	//
	if (!connected) { return; }
	
	client.playbackRewind(serverHandle);
}


// --- SEEK ---
void MainWindow::seek() {
	if (!connected) { return; }
	
	// Read out location on seek bar.
	uint8_t location = ui->positionSlider->value();
	
	client.playbackSeek(serverHandle, location);
}


// --- MUTE ---
void MainWindow::mute() {
	//
	if (!connected) { return; }
	
	if (!muted) {
		client.volumeSet(serverHandle, 0);
		muted = true;
	}
	else {
		client.volumeSet(serverHandle, ui->volumeSlider->value());
		muted = false;
	}
}


// --- ADJUST VOLUME ---
void MainWindow::adjustVolume(int value) {
	if (value < 0 || value > 128) { return; }
	
	client.volumeSet(serverHandle, value);
}


// --- APPS LIST REFRESH ---
void MainWindow::appsListRefresh() {
	if (!connected) { return; }
	
	// Get list of apps from the remote server.
	std::string appList = client.getApplicationList(serverHandle);
	
	// Update local list.
	ui->remoteAppsComboBox->clear();
	QStringList appItems = (QString::fromStdString(appList)).split("\n", Qt::SkipEmptyParts);
	ui->remoteAppsComboBox->addItems(appItems);
}


// --- SEND COMMAND ---
void MainWindow::sendCommand() {
	if (!connected) { return; }
	
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
	
	std::string response = client.sendApplicationMessage(serverHandle, appId, message);
	
	// Append the response to the output field.
	ui->remoteAppTextEdit->appendPlainText(QString::fromStdString(response));
	ui->remoteAppTextEdit->appendPlainText("\n");
}


// --- APPS HOME ---
void MainWindow::appsHome() {
    if (!connected) { return; }
    
    // Request the starting Apps page from the remote.
    QString page = QString::fromStdString(client.loadResource(serverHandle, std::string(), 
                                                                               "apps.html"));
    
    
    // Set the received HTML into the target widget.
    ui->appTabGuiTextBrowser->setHtml(page);
}


// --- ANCHOR CLICKED ---
void MainWindow::anchorClicked(const QUrl &link) {
    // Parse URL string for the command desired.
    QStringList list = link.path().split("/");
    
    // Process command.
    if (list.size() < 1) { return; }
    
    if (list[0] == "start") {
        // Start an app here, which should be listed in the second slot.
        if (list.size() < 2) { return; }
        
        // Try to load the index page for the specified app.
        QString page = QString::fromStdString(client.loadResource(serverHandle, 
                                                                  list[1].toStdString(), 
                                                                   "index.html"));
        
        if (page.isEmpty()) { 
            QMessageBox::warning(this, tr("Failed to start"), tr("The selected app could not be started."));
            return; 
        }
        
        // Set the received HTML into the target widget.
        ui->appTabGuiTextBrowser->setHtml(page);
    }
    else {
        // Assume the first entry contains an app name, followed by commands.
        // TODO: validate app names here.
    }
}


// --- LOAD RESOURCE ---
QByteArray MainWindow::loadResource(const QUrl &name) {
    // Parse the URL for the desired resource.
    QFileInfo dir(name.path());
    QString qAppId = dir.path();
    std::string filename = name.fileName().toStdString();
    
    // FIXME: Hack to deal with weird QLiteHtml behaviour with relative URLs.
    if (qAppId.startsWith("/")) {
        qAppId.remove(0, 1);
    }
    
    std::string appId = qAppId.toStdString();
    
    QByteArray page = QByteArray::fromStdString(client.loadResource(serverHandle, appId, filename));
    return page;
}


// --- ABOUT ---
void MainWindow::about() {
	QMessageBox::about(this, tr("About NymphCast Player."), tr("NymphCast Player is a simple demonstration player for the NymphCast client SDK."));
}


// --- QUIT ---
void MainWindow::quit() {
	exit(0);
}
