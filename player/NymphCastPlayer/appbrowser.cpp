#include "appbrowser.h"

#include <QFileInfo>
#include "mainwindow.h"


// --- LOAD RESOURCE ---
QVariant AppBrowser::loadResource(int /*type*/, const QUrl &name) {
    // Parse the URL for the desired resource.
    QFileInfo dir(name.path());
    std::string appId = dir.path().toStdString();
    std::string filename = name.fileName().toStdString();
    
    QByteArray page = QByteArray::fromStdString(MainWindow::client.loadResource(MainWindow::serverHandle, 
                                                                          appId, filename));
    QVariant var(page);
    return var;
}
