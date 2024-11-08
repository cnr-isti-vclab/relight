#ifndef QUEUEFRAME_H
#define QUEUEFRAME_H

#include <QFrame>

class QItemSelection;
class QGridLayout;
class QListWidget;
class QAction;
class QToolBar;
class Task;

class QueueFrame: public QFrame {
	Q_OBJECT
public:
	QueueFrame(QWidget *parent = nullptr);

	void setToolsStatus();
	void removeTask(Task *task);

public slots:
	void selectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
	void update();

	void start();
	void pause();
	void stop();

	void sendToTop();
	void sendToBottom();
	void remove();

private:
	QToolBar *toolbar;
	QAction *actionStart;
	QAction *actionPause;
	QAction *actionStop;
	QAction *actionToTop;
	QAction *actionToBottom;
	QAction *actionRemove;
	QAction *actionOpenFolder;
	QAction *actionInfo;

	QWidget *centralwidget;
	QGridLayout *gridLayout;
	QListWidget *list;

};

#endif // QUEUEFRAME_H
