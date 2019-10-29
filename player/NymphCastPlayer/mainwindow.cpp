#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <vector>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent) :     QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
	
	// Set up UI connections.
	// Menu
	connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(connectServer()));
	connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
	connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
	connect(ui->actionFile, SIGNAL(triggered()), this, SLOT(castFile()));
	connect(ui->actionURL, SIGNAL(triggered()), this, SLOT(castUrl()));
	
	// UI
	connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addFile()));
	connect(ui->removeButton, SIGNAL(clicked()), this, SLOT(removeFile()));
	connect(ui->connectToolButton, SIGNAL(clicked()), this, SLOT(connectServer())); // TODO: change to findserver()
	
	connect(ui->beginToolButton, SIGNAL(clicked()), this, SLOT(rewind()));
	connect(ui->endToolButton, SIGNAL(clicked()), this, SLOT(forward()));
	connect(ui->playToolButton, SIGNAL(clicked()), this, SLOT(play()));
	connect(ui->stopToolButton, SIGNAL(clicked()), this, SLOT(stop()));
	connect(ui->pauseToolButton, SIGNAL(clicked()), this, SLOT(pause()));
	
	connect(ui->soundToolButton, SIGNAL(clicked()), this, SLOT(mute()));
	connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(adjustVolume(int)));
	
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
	ui->remoteLabel->setText("Connected.");
    
    // Successful connect.
    connected = true;
}


// --- DISCONNECT SERVER ---
void MainWindow::disconnectServer() {
	if (!connected) { return; }
	
	client.disconnectServer(serverHandle);
	
	ui->remoteLabel->setText("Disconnected.");
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
	
}


// --- REMOVE FILE ---
void MainWindow::removeFile() {
	// Remove currently selected filename(s) from the playlist.
	
}


// --- FIND SERVER ---
void MainWindow::findServer() {
	// Open the mDNS dialogue to select a NymphCast receiver.
	
}


// --- PLAY ---
void MainWindow::play() {
	if (!connected) { return; }
	
	client.playbackStart(serverHandle);
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


// --- ABOUT ---
void MainWindow::about() {
	QMessageBox::about(this, tr("About NymphCast Player."), tr("NymphCast Player is a simple demonstration player for the NymphCast client SDK."));
}


// --- QUIT ---
void MainWindow::quit() {
    exit(0);
}
