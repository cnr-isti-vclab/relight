#ifndef QUEUEFRAME_H
#define QUEUEFRAME_H

#include <QFrame>
#include <QVector>
#include <memory>

class QItemSelection;
class QGridLayout;
class QListWidget;
class QAction;
class QToolBar;
class Task;
class HistoryTask;

class QueueFrame: public QFrame {
	Q_OBJECT
public:
	QueueFrame(QWidget *parent = nullptr);
	void removeTask(Task *task);
	void init() { updateLists(); }
public slots:
	void updateLists();
	void taskFinished(Task *task);
	void startQueue();
	void pauseQueue();
	void stopQueue();

private:
	void rebuildActiveList();
	void rebuildHistoryList();
	void updateToolbarState();
	void applyActionStyle(QAction *action, const QString &color, bool highlight);

	QWidget *centralwidget;
	QGridLayout *gridLayout;
	QListWidget *activeList;
	QListWidget *historyList;
	QToolBar *toolbar = nullptr;
	QAction *actionStart = nullptr;
	QAction *actionPause = nullptr;
	QAction *actionStop = nullptr;
	std::vector<HistoryTask *> historyEntries; //keep track of historytasks which wont be managed by processqueue.

};

#endif // QUEUEFRAME_H
