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
	Task *task;

	QWidget *widget = nullptr;
	QLabel *title = nullptr;
	QLabel *pathLabel = nullptr;
	QLabel *status = nullptr;
	QProgressBar *progressbar = nullptr;
	QLabel *infoLabel = nullptr;
	QLabel *detailLabel = nullptr;

	QToolButton *infoButton = nullptr;
	QToolButton *removeButton = nullptr;
	QToolButton *castButton = nullptr;
	QToolButton *folderButton = nullptr;
	bool historyEntry = false;
	bool detailsVisible = false;
	bool infoHasContent = false;

	QueueItem(Task *task, QListWidget *parent, bool history = false);
	void updateContent();

public slots:
	void progress(QString text, int percent);
	void casting();
	void openFolder();
	void handleRemove();
	void toggleDetails();

private:
	QMap<int, QString> style;
	void refreshActions();
	void refreshInfoLabel();
	void refreshDetails();
	void updateSizeHintGeometry();
};
#endif // QUEUEITEM_H
