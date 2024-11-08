#ifndef QUEUEITEM_H
#define QUEUEITEM_H

#include <QListWidgetItem>

class QLabel;
class QPushButton;
class QProgressBar;
class Task;

class QueueItem: public QObject, public QListWidgetItem  {
	Q_OBJECT
public:
	int id;
	Task *task;

	QWidget *widget = nullptr;
	QLabel *status = nullptr;
	QPushButton *cast = nullptr;
	QPushButton *folder = nullptr;
	QProgressBar *progressbar = nullptr;

	QueueItem(Task *task, QListWidget *parent);
	void update();
public slots:
	void setSelected(bool selected);
	void progress(QString text, int percent);
	void casting();

private:
	QMap<int, QString> style;
};
#endif // QUEUEITEM_H
