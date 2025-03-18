#ifndef HOMEWIDGET_H
#define HOMEWIDGET_H

#include <QFrame>
#include <QTextBrowser>

class HomeFrame: public QFrame {
public:
	HomeFrame();
};

class HelpBrowser: public QTextBrowser {
	Q_OBJECT
public:
	HelpBrowser(QWidget *parent = nullptr): QTextBrowser(parent) {
		setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	}

	QStringList history;
	int current = -1;
public slots:
	void next();
	void previous();
	//unfortunately QTextBrowser offer no direct access to history and setHtml erases it.
	void doSetSource(const QUrl &url, QTextDocument::ResourceType type = QTextDocument::UnknownResource);
};

#endif // HOMEWIDGET_H
