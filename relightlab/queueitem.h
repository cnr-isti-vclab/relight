#ifndef QUEUEITEM_H
#define QUEUEITEM_H

#include <QListWidgetItem>

class QLabel;
class QToolButton;
class QProgressBar;
class Task;

class QueueItem: public QObject, public QListWidgetItem  {
	Q_OBJECT
public:
	int id;
	Task *task;

	QWidget *widget = nullptr;
	QLabel *title = nullptr;
	QLabel *pathLabel = nullptr;
	QLabel *status = nullptr;
	QLabel *detailLabel = nullptr;
	QToolButton *infoButton = nullptr;
	QToolButton *removeButton = nullptr;
	QToolButton *castButton = nullptr;
	QToolButton *folderButton = nullptr;
	QProgressBar *progressbar = nullptr;
	bool historyEntry = false;
	bool detailsVisible = false;

	QueueItem(Task *task, QListWidget *parent, bool history = false);
	void update();

public slots:
	void progress(QString text, int percent);
	void casting();
	void openFolder();
	void handleStart();
	void handlePause();
	void handleStop();
	void handleRemove();
	void toggleDetails();

private:
	QMap<int, QString> style;
	void refreshActions();
	void refreshDetails();
};
#endif // QUEUEITEM_H
