/*
	custom_remotes.h - Custom remotes management dialogue.
	
*/


#ifndef CUSTOM_REMOTES_H
#define CUSTOM_REMOTES_H


#include <QDialog>
#include <QTime>

#include <vector>
#include <string>

#include "remotes.h"


namespace Ui {
	class CustomRemotesDialog;
}


class CustomRemotesDialog : public QDialog {
	Q_OBJECT
	
	Ui::CustomRemotesDialog* ui;
	std::vector<NCRemoteInstance> remotes;
	
public:
	explicit CustomRemotesDialog(QWidget* parent = nullptr);
	~CustomRemotesDialog();
	
public slots:
	void setRemoteList(std::vector<NCRemoteInstance> &remotes);
	
private slots:
	void addRemote();
	void removeRemote();
	void acceptClose();
	
signals:
	void saveRemotesList(std::vector<NCRemoteInstance> &remotes);
    
protected:
    void closeEvent(QCloseEvent *event);
};



#endif
