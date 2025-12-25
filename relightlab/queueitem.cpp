#include "queueitem.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QProgressBar>
#include <QMessageBox>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMimeDatabase>
#include <QProcess>
#include <QMutexLocker>
#include <QUrl>
#include <QDir>
#include <QIcon>

#include "../src/task.h"
#include "relightapp.h"
#include "../src/network/httpserver.h"
#include "processqueue.h"

namespace {
QToolButton *makeButton(QWidget *parent, const QString &icon, const QString &tooltip, bool checkable = false) {
	QToolButton *button = new QToolButton(parent);
	button->setIcon(QIcon::fromTheme(icon));
	button->setToolTip(tooltip);
	button->setAutoRaise(true);
	button->setCursor(Qt::PointingHandCursor);
	button->setCheckable(checkable);
	return button;
}
}

QueueItem::QueueItem(Task *_task, QListWidget *parent, bool history): QObject(parent), QListWidgetItem(parent), historyEntry(history) {
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
	grid->setContentsMargins(12, 10, 12, 10);
	grid->setHorizontalSpacing(10);
	grid->setVerticalSpacing(4);
	grid->setColumnStretch(0, 1);
	grid->setColumnStretch(1, 1);
	grid->setColumnStretch(2, 0);

	title = new QLabel("<b>" + task->label + "</b> " + task->output);
	title->setTextInteractionFlags(Qt::TextSelectableByMouse);
	grid->addWidget(title, 0, 0, 1, 2);


	status = new QLabel();
	status->setMinimumWidth(250);
	status->setObjectName("taskStatus");
	grid->addWidget(status, 2, 0, 1, 1);

	pathLabel = new QLabel(task->output);
	pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	grid->addWidget(pathLabel, 1, 0, 1, 2);

	progressbar = new QProgressBar();
	progressbar->setValue(0);
	progressbar->setTextVisible(false);
	grid->addWidget(progressbar, 2, 1, 1, 2);

	detailLabel = new QLabel();
	detailLabel->setWordWrap(true);
	detailLabel->hide();
	grid->addWidget(detailLabel, 3, 0, 1, 3);


	QHBoxLayout *actions = new QHBoxLayout();
	actions->setSpacing(4);
	grid->addLayout(actions, 0, 2, 1, 1, Qt::AlignRight);

	infoButton = makeButton(widget, "info", tr("Show log / errors"), true);
	connect(infoButton, &QToolButton::clicked, this, &QueueItem::toggleDetails);
	actions->addWidget(infoButton);

	castButton = makeButton(widget, "cast", tr("Cast result"));
	connect(castButton, &QToolButton::clicked, this, &QueueItem::casting);
	actions->addWidget(castButton);

	folderButton = makeButton(widget, "folder", tr("Open output folder"));
	connect(folderButton, &QToolButton::clicked, this, &QueueItem::openFolder);
	actions->addWidget(folderButton);

	actions->addSpacing(12);


	removeButton = makeButton(widget, "trash-2", tr("Remove from queue"));
	connect(removeButton, &QToolButton::clicked, this, &QueueItem::handleRemove);
	actions->addWidget(removeButton);


	widget->setLayout(grid);

	setSizeHint(widget->minimumSizeHint());
	parent->setItemWidget(this, widget);

	update();
}

void QueueItem::progress(QString text, int percent) {
	status->setText(text);
	if(progressbar)
		progressbar->setValue(percent);
}

void QueueItem::update() {
	ProcessQueue &queue = ProcessQueue::instance();
	Q_UNUSED(queue);

	QString statusText;
	switch(task->status) {
	case Task::PAUSED:
		statusText = tr("Paused");
		break;
	case Task::ON_QUEUE:
		statusText = tr("Waiting");
		break;
	case Task::RUNNING:
		statusText = tr("Running");
		break;
	case Task::DONE:
		statusText = tr("Done");
		if(progressbar)
			progressbar->setValue(100);
		break;
	case Task::STOPPED:
		statusText = tr("Stopped");
		if(progressbar)
			progressbar->setValue(0);
		break;
	case Task::FAILED:
		statusText = task->error.isEmpty() ? tr("Failed") : task->error;
		if(progressbar)
			progressbar->setValue(0);
		break;
	}
	status->setText(statusText);

	refreshActions();
	refreshDetails();
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
			server.port = qRelightApp->castingPort();
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

void QueueItem::handleStart() {
	ProcessQueue &queue = ProcessQueue::instance();
	if(historyEntry)
		return;
	queue.pushFront(task->id);
	queue.start();
}

void QueueItem::handlePause() {
	if(historyEntry)
		return;
	ProcessQueue::instance().pause();
}

void QueueItem::handleStop() {
	if(historyEntry)
		return;
	ProcessQueue::instance().stop();
}

void QueueItem::handleRemove() {
	if(historyEntry)
		return;
	int answer = QMessageBox::question(widget, tr("Remove task"),
								   tr("Remove '%1' from the queue?").arg(task->label),
								   QMessageBox::Yes | QMessageBox::No);
	if(answer != QMessageBox::Yes)
		return;
	ProcessQueue &queue = ProcessQueue::instance();
	if(queue.task == task)
		queue.stop();
	else
		queue.removeTask(task);
}

void QueueItem::toggleDetails() {
	if(!infoButton)
		return;
	detailsVisible = infoButton->isChecked();
	refreshDetails();
}

void QueueItem::refreshActions() {
	const bool isRunning = task->status == Task::RUNNING;
	const bool isPaused = task->status == Task::PAUSED;
	const bool isQueued = task->status == Task::ON_QUEUE;
	const bool isStopped = task->status == Task::STOPPED;
	const bool isDone = task->status == Task::DONE;
	const bool isFailed = task->status == Task::FAILED;

	if(progressbar)
		progressbar->setVisible(!historyEntry && (isRunning || isQueued || isPaused));

	const bool canRemove = !historyEntry && !isRunning;

	removeButton->setVisible(true);
	removeButton->setEnabled(historyEntry || canRemove);
	if(historyEntry)
		removeButton->setToolTip(tr("Remove from history"));
	else
		removeButton->setToolTip(canRemove ? tr("Remove from queue") : tr("Stop task before removing"));

	QFileInfo info(task->output);
	const bool hasOutput = info.exists();
	folderButton->setEnabled(hasOutput);
	bool canCast = hasOutput && (task->mime == Task::RELIGHT || task->mime == Task::RTI || task->mime == Task::PTM || task->mime == Task::IMAGE);
	castButton->setEnabled(canCast && (isDone || historyEntry));
	castButton->setToolTip(canCast ? tr("Cast result") : tr("Casting available once processing finishes"));
	infoButton->setVisible(true);
	infoButton->setEnabled(true);

	if(isDone || isFailed)
		status->setText(status->text());
}

void QueueItem::refreshDetails() {
	if(!detailLabel)
		return;
	QString detailText;
	if(task->status == Task::FAILED)
		detailText = task->error;
	else
		detailText = task->log;
	detailText = detailText.trimmed();
	detailLabel->setText(detailText);
	const bool hasDetails = !detailText.isEmpty();
	infoButton->setEnabled(hasDetails);
	if(!hasDetails) {
		detailLabel->hide();
		infoButton->setChecked(false);
		detailsVisible = false;
		return;
	}
	detailLabel->setVisible(detailsVisible);
}
