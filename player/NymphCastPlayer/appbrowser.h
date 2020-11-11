#ifndef APPBROWSER_H
#define APPBROWSER_H

#include <QTextBrowser>

class AppBrowser : public QTextBrowser {
    Q_OBJECT
public:
    AppBrowser(QWidget* parent) : QTextBrowser(parent) {};
    
    QVariant loadResource(int type, const QUrl &name) override;
};

#endif // APPBROWSER_H
