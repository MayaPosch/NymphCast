/*
	remotes.cpp - Remote receiver management dialogue.
	
*/


#include "remotes.h"

#include "ui_remotes.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>

// Debug
#include <iostream>


// --- CONSTRUCTOR ---
RemotesDialog::RemotesDialog(QWidget* parent) : QDialog(parent), ui(new Ui::RemotesDialog) {
	ui->setupUi(this);
	
	// Set up UI connections.
	connect(ui->refreshRemotesButton, SIGNAL(clicked()), this, SIGNAL(updateRemotesList()));
	connect(ui->createGroupButton, SIGNAL(clicked()), this, SLOT(createGroup()));
	connect(ui->addToGroupButton, SIGNAL(clicked()), this, SLOT(addToGroup()));
	connect(ui->groupsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeActiveGroup(int)));
	connect(ui->deleteGroupRemoteButton, SIGNAL(clicked()), this, SLOT(deleteGroupRemote()));
	connect(ui->renameGroupButton, SIGNAL(clicked()), this, SLOT(renameGroup()));
	connect(ui->deleteGroupButton, SIGNAL(clicked()), this, SLOT(deleteGroup()));
}


// --- DESTRUCTOR ---
RemotesDialog::~RemotesDialog() {
	delete ui;
}


// --- SET REMOTE LIST ---
void RemotesDialog::setRemoteList(std::vector<NCRemoteInstance> &remotes) {
	this->remotes = remotes;
	
	// Update UI.
	ui->remotesListWidget->clear();
	for (uint32_t i = 0; i < remotes.size(); ++i) {
		QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(remotes[i].remote.name),
													ui->remotesListWidget);
		item->setData(Qt::UserRole, QVariant(i));
	}
}


// --- UPDATE GROUPS LIST ---
void RemotesDialog::updateGroupsList(std::vector<NCRemoteGroup> &groups) {
	this->groups = groups;
	
	// Update UI.
	ui->groupRemotesListWidget->clear();
	ui->groupsComboBox->clear();
	for (uint32_t i = 0; i < groups.size(); ++i) {
		ui->groupsComboBox->addItem(QString::fromStdString(groups[i].name), QVariant(i));
	}
}


// --- CREATE GROUP ---
// Ask for name of new group. Create new entry using the currently selected remote(s).
void RemotesDialog::createGroup() {	
	// Ask for confirmation when creating a group with no remotes selected.
	QList<QListWidgetItem *> items = ui->remotesListWidget->selectedItems();
	if (items.size() < 1) {
		if (QMessageBox::question(this, tr("Create empty group"), 
									tr("This will create an empty group.")) == QMessageBox::No) {
			// Group creation cancelled. Return.
			return;
		}
	}
	
	// Create new group entry with selected remotes added.
	bool ok;
    QString text = QInputDialog::getText(this, tr("New group name"),
                                         tr("Group name:"), QLineEdit::Normal,
                                         QString(), &ok);
    if (ok && !text.isEmpty()) {
		// Label was entered. Use it.
        NCRemoteGroup group;
		group.name = text.toStdString();
		
		// Add the selected remotes.
		for (uint32_t i = 0; i < (uint32_t) items.size(); ++i) {
			group.remotes.push_back(remotes[items[i]->data(Qt::UserRole).toUInt()]);
		}
		
		groups.push_back(group);
	}
	
	// Add new group to combobox.
	ui->groupsComboBox->addItem(text, QVariant((uint32_t) groups.size() - 1));
}


// --- ADD TO GROUP ---
// Add currently selected remote(s) to the currently active group.
void RemotesDialog::addToGroup() {
	if (groups.size() < 1 || ui->groupsComboBox->currentIndex() == -1) { return; }
	
	// If no items are selected, show message.
	QList<QListWidgetItem *> items = ui->remotesListWidget->selectedItems();
	if (items.size() < 1) {
		QMessageBox::information(this, tr("No remotes selected."), 
									tr("First select a remote to add it."));
		return;
	}
	
	// Add any new remotes to the group.
	// For each new remote, check that it doesn't already exist in the group.
	NCRemoteGroup& group = groups[ui->groupsComboBox->currentData().toUInt()];
	for (uint32_t i = 0; i < (uint32_t) items.size(); ++i) {
		NCRemoteInstance& remote = remotes[items[i]->data(Qt::UserRole).toUInt()];
		bool found = false;
		for (uint32_t j = 0; j < group.remotes.size(); ++j) {
			if (remote.handle == group.remotes[j].handle) {
				// Remote is already in the list. Skip.
				found = true;
				break;
			}
		}
		
		if (!found) {
			// Add to UI & groups.
			group.remotes.push_back(remote);
			QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(remote.remote.name),
													ui->groupRemotesListWidget);
			item->setData(Qt::UserRole, QVariant((uint32_t) group.remotes.size() - 1));
		}
	}
}


// --- CHANGE ACTIVE GROUP ---
// If a valid active group is selected, change the contents of the 'Group remotes' list box.
void RemotesDialog::changeActiveGroup(int index) {
	// Update UI.
	ui->groupRemotesListWidget->clear();
	std::vector<NCRemoteInstance>& rem = groups[index].remotes;
	for (uint32_t i = 0; i < rem.size(); ++i) {
		QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(rem[i].remote.name),
													ui->groupRemotesListWidget);
		item->setData(Qt::UserRole, QVariant(i));
	}
	
}


// --- DELETE GROUP REMOTE ---
// Delete the selected remotes from the active group remote list.
void RemotesDialog::deleteGroupRemote() {
	if (groups.size() < 1 || ui->groupsComboBox->currentIndex() == -1) { return; }
	
	QList<QListWidgetItem *> items = ui->groupRemotesListWidget->selectedItems();
	
	// Return if no items selected.
	if (items.size() < 1) { return; }
	
	// Remove each item from the group remotes list.
	NCRemoteGroup& group = groups[ui->groupsComboBox->currentData().toUInt()];
	for (uint32_t i = 0; i < (uint32_t) items.size(); ++i) {
		group.remotes.erase(group.remotes.begin() + items[i]->data(Qt::UserRole).toUInt());
	}
	
	// Update UI.
	ui->groupRemotesListWidget->clear();
	std::vector<NCRemoteInstance>& rem = group.remotes;
	for (uint32_t i = 0; i < rem.size(); ++i) {
		QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(rem[i].remote.name),
													ui->groupRemotesListWidget);
		item->setData(Qt::UserRole, QVariant(i));
	}
}


// --- RENAME GROUP ---
// Rename the currently active group.
void RemotesDialog::renameGroup() {
	if (groups.size() < 1 || ui->groupsComboBox->currentIndex() == -1) { return; }
	
	bool ok;
    QString text = QInputDialog::getText(this, tr("Rename group"),
                                         tr("New name:"), QLineEdit::Normal,
                                         QString(), &ok);
    if (ok && !text.isEmpty()) {
		NCRemoteGroup& group = groups[ui->groupsComboBox->currentData().toUInt()];
		group.name = text.toStdString();
	}
	
	// Update UI.
	ui->groupsComboBox->clear();
	for (uint32_t i = 0; i < groups.size(); ++i) {
		ui->groupsComboBox->addItem(QString::fromStdString(groups[i].name), QVariant(i));
	}
}


// --- DELETE GROUP ---
void RemotesDialog::deleteGroup() {
	if (groups.size() < 1 || ui->groupsComboBox->currentIndex() == -1) { return; }
	
	// Delete the currently active group.
	if (QMessageBox::question(this, tr("Confirm group delete"), 
									tr("This will delete the current group. Proceed?")) == QMessageBox::No) {
		// Group deletion cancelled. Return.
		return;
	}
	
	// Delete the group.
	uint32_t idx = ui->groupsComboBox->currentData().toUInt();
	int boxidx = ui->groupsComboBox->currentIndex();
	ui->groupsComboBox->removeItem(boxidx);
	groups.erase(groups.begin() + idx);
	
	// Update the groups combobox & clear list.
	ui->groupRemotesListWidget->clear();
	ui->groupsComboBox->clear();
	for (uint32_t i = 0; i < groups.size(); ++i) {
		ui->groupsComboBox->addItem(QString::fromStdString(groups[i].name), QVariant(i));
	}
}


// --- CLOSE EVENT ---
// Handle the window being closed by the user.
void RemotesDialog::closeEvent(QCloseEvent *event) {
	// Signal main UI with updated groups list.
	std::cout << "Emitting closeEvent: Saving group..." << std::endl;
	emit saveGroupsList(groups);
    event->accept();
}
