/*
	custom_remotes.cpp - Custom Remote receiver management dialogue.
	
*/


#include "custom_remotes.h"

#include "ui_custom_remotes.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>


// --- CONSTRUCTOR ---
CustomRemotesDialog::CustomRemotesDialog(QWidget* parent) : QDialog(parent), ui(new Ui::CustomRemotesDialog) {
	ui->setupUi(this);
	
	// Set up UI connections.
	connect(ui->addRemoteButton, SIGNAL(clicked()), this, SLOT(addRemote()));
	connect(ui->removeRemoteButton, SIGNAL(clicked()), this, SLOT(removeRemote()));
	
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(acceptClose()));
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}


// --- DESTRUCTOR ---
CustomRemotesDialog::~CustomRemotesDialog() {
	delete ui;
}


// --- SET REMOTE LIST ---
void CustomRemotesDialog::setRemoteList(std::vector<NCRemoteInstance> &remotes) {
	this->remotes = remotes;
	
	// Update UI.
	ui->remotesListWidget->clear();
	for (uint32_t i = 0; i < remotes.size(); ++i) {
		QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(remotes[i].remote.name 
											+ " (" + remotes[i].remote.ipv4 + ")"),
													ui->remotesListWidget);
		item->setData(Qt::UserRole, QVariant(i));
	}
}


// --- ADD REMOTE ---
// Ask for name of new group. Create new entry using the currently selected remote(s).
void CustomRemotesDialog::addRemote() {
	// Create new remote entry.
	// The values are read in from the dialogue fields.
	// TODO: validate values.
	NymphCastRemote ncr;
	ncr.name = ui->remoteNameEdit->text().toStdString();
	ncr.ipv4 = ui->remoteHostEdit->text().toStdString();
	//ncr.ipv6 = ui->remoteIPv6Edit->text().toStdString();
	ncr.port = ui->portSpinBox->value();
	
	// Add to list.
	QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(ncr.name + " (" 
											+ ncr.ipv4 + ")"),
													ui->remotesListWidget);
	item->setData(Qt::UserRole, QVariant(remotes.size()));
	
	NCRemoteInstance remote;
	remote.remote = ncr;
	remotes.push_back(remote);
}


// --- REMOVE REMOTE ---
void CustomRemotesDialog::removeRemote() {
	// Confirm a remote is selected.
	QList<QListWidgetItem *> items = ui->remotesListWidget->selectedItems();
	
	// Return if no items selected.
	if (items.size() < 1) { return; }
	
	// Confirm delete.
	if (QMessageBox::question(this, tr("Confirm delete"), 
									tr("This will delete the selected remotes. Proceed?")) == QMessageBox::No) {
		// Group deletion cancelled. Return.
		return;
	}
	
	// Remove remote from the list and collection.
	for (uint32_t i = 0; i < (uint32_t) items.size(); ++i) {
		remotes.erase(remotes.begin() + items[i]->data(Qt::UserRole).toUInt());
	}
	
	// Update UI.
	ui->remotesListWidget->clear();
	for (uint32_t i = 0; i < remotes.size(); ++i) {
		QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(remotes[i].remote.name 
											+ " (" + remotes[i].remote.ipv4 + ")"),
													ui->remotesListWidget);
		item->setData(Qt::UserRole, QVariant(i));
	}
}


// --- ACCEPT CLOSE ---
void CustomRemotesDialog::acceptClose() {
	emit saveRemotesList(remotes);
	accept();
}


// --- CLOSE EVENT ---
// Handle the window being closed by the user.
void CustomRemotesDialog::closeEvent(QCloseEvent *event) {
	// Signal main UI with updated groups list.
	emit saveRemotesList(remotes);
    event->accept();
}
