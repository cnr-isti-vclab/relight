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
	void removeTask(Task *task);

public slots:
	void update();
	void startQueue();
	void pauseQueue();
	void stopQueue();

private:
	void rebuildActiveList(const QList<Task *> &tasks);
	void rebuildHistoryList(const QList<Task *> &tasks);

	QWidget *centralwidget;
	QGridLayout *gridLayout;
	QListWidget *activeList;
	QListWidget *historyList;
	QToolBar *toolbar = nullptr;
	QAction *actionStart = nullptr;
	QAction *actionPause = nullptr;
	QAction *actionStop = nullptr;

};

#endif // QUEUEFRAME_H
