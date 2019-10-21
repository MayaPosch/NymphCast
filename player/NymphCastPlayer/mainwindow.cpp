#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <nymph/nymph.h>
//#include "../../../NymphRPC/src/nymph.h"
#include <iostream>
#include <vector>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
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
    // Connect to localhost NymphRPC server, standard port.
    int handle;
        std::string result;
        if (!NymphRemoteServer::connect("127.0.0.1", 4004, handle, 0, result)) {
            std::cout << "Connecting to remote server failed: " << result << std::endl;
            NymphRemoteServer::disconnect(handle, result);
            NymphRemoteServer::shutdown();
            return;
        }
        
        // Send message and wait for response.
        std::vector<NymphType*> values;
        values.push_back(new NymphString("NymphClient_21xb"));
        NymphType* returnValue = 0;
        if (!NymphRemoteServer::callMethod(handle, "connect", values, returnValue, result)) {
            std::cout << "Error calling remote method: " << result << std::endl;
            NymphRemoteServer::disconnect(handle, result);
            NymphRemoteServer::shutdown();
            return;
        }
        
        if (returnValue->type() != NYMPH_BOOL) {
            std::cout << "Return value wasn't a boolean. Type: " << returnValue->type() << std::endl;
            NymphRemoteServer::disconnect(handle, result);
            NymphRemoteServer::shutdown();
            return;
        }
    
    // Successful connect.
    connected = true;
}


void MainWindow::quit() {
    exit(0);
}
