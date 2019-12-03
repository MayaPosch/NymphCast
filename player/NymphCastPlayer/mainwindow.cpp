#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <vector>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QCoreApplication>


MainWindow::MainWindow(QWidget *parent) :     QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
	
	// Set application options.
	QCoreApplication::setApplicationName("NymphCastPlayer");
	QCoreApplication::setApplicationVersion("v0.1-alpha");
	QCoreApplication::setOrganizationName("Nyanko");
	
	// Set up UI connections.
	// Menu
	connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(connectServer()));
	connect(ui->actionDisconnect, SIGNAL(triggered()), this, SLOT(disconnectServer()));
	connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
	connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
	connect(ui->actionFile, SIGNAL(triggered()), this, SLOT(castFile()));
	connect(ui->actionURL, SIGNAL(triggered()), this, SLOT(castUrl()));
	
	// UI
	connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addFile()));
	connect(ui->removeButton, SIGNAL(clicked()), this, SLOT(removeFile()));
	connect(ui->refreshRemotesToolButton, SIGNAL(clicked()), this, SLOT(remoteListRefresh()));
	connect(ui->connectToolButton, SIGNAL(clicked()), this, SLOT(remoteConnectSelected()));
	connect(ui->disconnectToolButton, SIGNAL(clicked()), this, SLOT(remoteDisconnectSelected()));
	
	connect(ui->beginToolButton, SIGNAL(clicked()), this, SLOT(rewind()));
	connect(ui->endToolButton, SIGNAL(clicked()), this, SLOT(forward()));
	connect(ui->playToolButton, SIGNAL(clicked()), this, SLOT(play()));
	connect(ui->stopToolButton, SIGNAL(clicked()), this, SLOT(stop()));
	connect(ui->pauseToolButton, SIGNAL(clicked()), this, SLOT(pause()));
	
	connect(ui->soundToolButton, SIGNAL(clicked()), this, SLOT(mute()));
	connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(adjustVolume(int)));
	
	connect(ui->remoteAppLineEdit, SIGNAL(returnPressed()), this, SLOT(sendCommand()));
	
}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::openFile() {
    // Start session.
    
    // Parse session parameters.
    
    // Start sending data.
    
}


void MainWindow::connectServer() {
	if (connected) { return; }
	
	// Ask for the IP address of the server.
	QString ip = QInputDialog::getText(this, tr("NymphCast Receiver"), tr("Please provide the NymphCast receiver IP address."));
	if (ip.isEmpty()) { return; }	
	
    // Connect to localhost NymphRPC server, standard port.
    client.connectServer(ip.toStdString(), serverHandle);
    
	// TODO: update server name label.
	ui->remoteLabel->setText("Connected to " + ip);
    
    // Successful connect.
    connected = true;
}


// --- CONNECT SERVER IP ---
void MainWindow::connectServerIP(std::string ip) {
	// Connect to localhost NymphRPC server, standard port.
    client.connectServer(ip, serverHandle);
    
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
}


// --- REMOTE LIST REFRESH ---
// Refresh the list of remote servers on the network.
void MainWindow::remoteListRefresh() {
	// Get the current list.
	std::vector<NymphCastRemote> remotes = client.findServers();
	
	// Update the list with any changed items.
	// Target the 'remotesListWidget' widget.
	ui->remotesListWidget->clear(); // FIXME: just resetting the whole thing for now.
	for (int i = 0; i < remotes.size(); ++i) {
		//new QListWidgetItem(remotes[i].ipv4 + " (" + remotes[i].name + ")", ui->remotesListWidget);
		QListWidgetItem *newItem = new QListWidgetItem;
		newItem->setText(QString::fromStdString(remotes[i].ipv4 + " (" + remotes[i].name + ")"));
		newItem->setData(Qt::UserRole, QVariant(QString::fromStdString(remotes[i].ipv4)));
		ui->remotesListWidget->insertItem(i, newItem);
	}
	
}


// --- REMOTE CONNECT SELECTED ---
// Connect to the selected remote server.
void MainWindow::remoteConnectSelected() {
	// TODO: Check that the selected server hasn't already been connected to.
	QListWidgetItem* item = ui->remotesListWidget->currentItem();
	QString ip = item->data(Qt::UserRole).toString();
	
	// Connect to the server.
	connectServerIP(ip.toStdString());
}


// --- REMOTE DISCONNECT SELECTED ---
void MainWindow::remoteDisconnectSelected() {
	//
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
	if (!finf.isFile()) { return; }
	
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


// --- FIND SERVER ---
// X obsolete?
void MainWindow::findServer() {
	// Open the mDNS dialogue to select a NymphCast receiver.
	
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
	if (value < 0 || value < 100) { return; }
	
	client.volumeSet(serverHandle, value);
}


// --- SEND COMMAND ---
void MainWindow::sendCommand() {
	if (!connected) { return; }
	
	// Read the data in the line edit and send it to the remote app.
	// FIXME: hardcode the soundcloud app here for prototype purposes.
	std::string appId = "soundcloud";
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


// --- ABOUT ---
void MainWindow::about() {
	QMessageBox::about(this, tr("About NymphCast Player."), tr("NymphCast Player is a simple demonstration player for the NymphCast client SDK."));
}


// --- QUIT ---
void MainWindow::quit() {
    exit(0);
}
