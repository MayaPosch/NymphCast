/*
	remotes.h - Remote receiver management dialogue.
	
*/


#ifndef REMOTES_H
#define REMOTES_H

#include <QDialog>

#include <vector>
#include <string>

#include "nymphcast_client.h"


struct NCRemoteInstance {
	NymphCastRemote remote;
	bool connected = false;
	uint32_t handle;
};


struct NCRemoteGroup {
	std::string name;
	std::vector<NCRemoteInstance> remotes;
};


namespace Ui {
	class RemotesDialog;
}


class RemotesDialog : public QDialog {
	Q_OBJECT
	
	Ui::RemotesDialog* ui;
	std::vector<NCRemoteInstance> remotes;
	std::vector<NCRemoteGroup> groups;
	
public:
	explicit RemotesDialog(QWidget* parent = nullptr);
	~RemotesDialog();
	
public slots:
	void setRemoteList(std::vector<NCRemoteInstance> &remotes);
	void updateGroupsList(std::vector<NCRemoteGroup> &groups);
	
private slots:
	void createGroup();
	void addToGroup();
	void changeActiveGroup(int index);
	void deleteGroupRemote();
	void renameGroup();
	void deleteGroup();
	
signals:
	void updateRemotesList();
	void saveGroupsList(std::vector<NCRemoteGroup> &groups);
    
protected:
    void closeEvent(QCloseEvent *event);
};


#endif
