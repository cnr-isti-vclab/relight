#ifndef HOMEWIDGET_H
#define HOMEWIDGET_H

#include <QFrame>
#include <QTextBrowser>

class HomeFrame: public QFrame {
public:
	HomeFrame();
};

class HelpBrowser: public QTextBrowser {
public:
	HelpBrowser(QWidget *parent = nullptr): QTextBrowser(parent) {}
public slots:
	void doSetSource(const QUrl &url, QTextDocument::ResourceType type = QTextDocument::UnknownResource);
};

#endif // HOMEWIDGET_H
