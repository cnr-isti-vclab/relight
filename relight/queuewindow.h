#ifndef QUEUEWINDOW_H
#define QUEUEWINDOW_H

#include <QMainWindow>
#include "processqueue.h"

namespace Ui {
	class QueueWindow;
}

class QItemSelection;
class QueueWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit QueueWindow(QWidget *parent);
	~QueueWindow();

	void setToolsStatus();

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
	Ui::QueueWindow *ui;
};

#endif // QUEUEWINDOW_H
