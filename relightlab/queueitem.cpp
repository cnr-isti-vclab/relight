#include "queueitem.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QMessageBox>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMimeDatabase>
#include <QProcess>
#include <QMutexLocker>
#include <QUrl>
#include <QDir>

#include "task.h"
#include "../relight/httpserver.h"
#include "processqueue.h"

#include <iostream>
using namespace std;

QueueItem::QueueItem(Task *_task, QListWidget *parent): QListWidgetItem(parent) {
	style[Task::ON_QUEUE] = "background-color:#323232; color:#b1b1b1";
	style[Task::RUNNING]  = "background-color:#328232; color:#b1b1b1";
	style[Task::PAUSED]   = "background-color:#323232; color:#b1b1b1";
	style[Task::STOPPED]  = "background-color:#823232; color:#b1b1b1";
	style[Task::DONE]     = "background-color:#323282; color:#b1b1b1";
	style[Task::FAILED]   = "background-color:#823232; color:#b1b1b1";

	task = _task;
	connect(task, SIGNAL(progress(QString,int)), this, SLOT(progress(QString,int)));

	id = task->id;
	widget = new QWidget;
	widget->setObjectName("task");

	QGridLayout *grid = new QGridLayout();

	QLabel *label = new QLabel("<b>" + task->label + "<b>");
	grid->addWidget(label, 0, 0, 1, 2);

	QFont font = label->font();
	font.setPointSize(10);

	QLabel *output = new QLabel(task->output);
	output->setFont(font);
	grid->addWidget(output, 1, 0, 1, 2);


	status = new QLabel();
	status->setMinimumWidth(250);
//	status->hide();
	grid->addWidget(status, 2, 0, 1, 1);

	progressbar = new QProgressBar();
	progressbar->setValue(0);
//	progressbar->hide();
	grid->addWidget(progressbar, 2, 1, 1, 1);


	cast = new QPushButton();
	cast->setIcon(QIcon::fromTheme("cast"));
	cast->setEnabled(false);
	grid->addWidget(cast, 0, 2, 3, 1);

	connect(cast, SIGNAL(clicked(bool)), this, SLOT(casting()));

	folder = new QPushButton();
	folder->setIcon(QIcon::fromTheme("folder"));
	folder->setEnabled(true);
	grid->addWidget(folder, 0, 3, 3, 1);

	connect(folder, SIGNAL(clicked(bool)), this, SLOT(openFolder()));

	widget->setLayout(grid);

	setSizeHint(widget->minimumSizeHint());
	parent->setItemWidget(this, widget);

	update();
}

void QueueItem::progress(QString text, int percent) {
	status->setText(text);
	progressbar->setValue(percent);
}

void QueueItem::update() {
	ProcessQueue &queue = ProcessQueue::instance();

	switch(task->status) {
	case Task::PAUSED:
		status->setText("Paused");
		break;
	case Task::ON_QUEUE:
		status->setText("On queue");
		break;
	case Task::RUNNING:
		status->setText("Starting...");
		break;
	case Task::DONE:
		status->setText("Done");
		progressbar->setValue(100);
		{
			QFileInfo info(task->output);
			if(info.isDir())
				cast->setEnabled(true);
		}
		cast->setEnabled(true);
		break;
	case Task::STOPPED:
		status->setText("Stopped");
		break;
	case Task::FAILED:
		status->setText(task->error);
		progressbar->setValue(0);
	}
	widget->setStyleSheet(style[task->status]);
}

void QueueItem::setSelected(bool selected) {
	if(selected)
		widget->setStyleSheet("background-color:#ffa02f; color: #ffffff;");
	else
		widget->setStyleSheet(style[task->status]);
}

void openFile(const QString& filePath) {
	QMimeDatabase db;
	QMimeType type = db.mimeTypeForFile(filePath);

	if (type.isValid() && type.name() != "application/octet-stream") {
#ifdef Q_OS_WIN
		QProcess::startDetached("explorer", { QDir::toNativeSeparators(filePath) });
#elif defined(Q_OS_MAC)
		QProcess::startDetached("open", { filePath });
#elif defined(Q_OS_LINUX)
		QProcess::startDetached("xdg-open", { filePath });
#else
		QDesktopServices::openUrl(QUrl::fromLocalFile(filePath)); // Fallback
#endif
	} else {
		QMessageBox::warning(nullptr, "No Associated Application",
							 "No default application is set for this file type.");
	}
}

void QueueItem::casting() {
	switch(task->mime) {
	case Task::IMAGE:
	case Task::RTI:
	case Task::PTM:
		openFile(task->output);
		break;
	case Task::RELIGHT:
		try {
			HttpServer &server = HttpServer::instance();
			server.stop();
			server.port = 8880;
			server.start(task->output);
			server.show();
		} catch(QString error) {
			QMessageBox::critical(nullptr, "Could not cast!", error);
		}
		break;
	default:
		break;
	}
}

void QueueItem::openFolder() {
	QFileInfo fileInfo(task->output);
	QString path = fileInfo.isDir() ? fileInfo.absoluteFilePath() : fileInfo.absolutePath();

	QUrl folderUrl = QUrl::fromLocalFile(path);
	QDesktopServices::openUrl(folderUrl);
}
