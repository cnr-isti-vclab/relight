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
#include <QSizePolicy>
#include <QVector>
#include <QStringList>
#include <QJsonObject>
#include <QJsonValue>
#include <QListWidget>
#include <QMetaObject>

#include "../src/task.h"
#include "relightapp.h"
#include "../src/network/httpserver.h"
#include "processqueue.h"

namespace {
struct InfoEntry {
	QString label;
	QString value;
};

QToolButton *makeButton(QWidget *parent, const QString &icon, const QString &tooltip, bool checkable = false) {
	QToolButton *button = new QToolButton(parent);
	button->setIcon(QIcon::fromTheme(icon));
	button->setToolTip(tooltip);
	button->setAutoRaise(true);
	button->setCursor(Qt::PointingHandCursor);
	button->setCheckable(checkable);
	return button;
}

QString boolToText(bool value) {
	return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString humanizeKey(const QString &key) {
	if(key.isEmpty())
		return key;
	QString current;
	QStringList words;
	for(QChar ch: key) {
		const bool isBoundary = ch.isUpper() || ch.isDigit();
		if(isBoundary && !current.isEmpty()) {
			words << current;
			current.clear();
		}
		current.append(ch);
	}
	if(!current.isEmpty())
		words << current;
	for(QString &word: words) {
		if(word.isEmpty())
			continue;
		word = word.toLower();
		word[0] = word[0].toUpper();
	}
	return words.join(QLatin1Char(' '));
}

QString valueToString(const QJsonValue &value) {
	if(value.isBool())
		return boolToText(value.toBool());
	return value.toVariant().toString();
}

QString formatCrop(const QJsonObject &crop) {
	if(crop.isEmpty())
		return QString();
	QStringList parts;
	const struct { const char *key; const char *label; } fields[] = {
		{"left", "x"},
		{"top", "y"},
		{"width", "w"},
		{"height", "h"},
		{"angle", "angle"}
	};
	for(const auto &field: fields) {
		if(crop.contains(field.key))
			parts << QString("%1=%2").arg(field.label).arg(crop.value(field.key).toVariant().toString());
	}
	return parts.join(", ");
}

void addValueEntry(QVector<InfoEntry> &entries, const QString &label, const QJsonValue &value) {
	QString text = valueToString(value);
	if(text.isEmpty())
		return;
	entries.push_back({label, text});
}

void addTextEntry(QVector<InfoEntry> &entries, const QString &label, const QString &value) {
	if(value.isEmpty())
		return;
	entries.push_back({label, value});
}

QVector<InfoEntry> formatGenericParameters(const QJsonObject &params) {
	QVector<InfoEntry> entries;
	for(auto it = params.begin(); it != params.end(); ++it) {
		if(it.key() == QLatin1String("taskType") || it.key() == QLatin1String("crop") || it.key() == QLatin1String("path"))
			continue;
		if(it.value().isObject())
			continue;
		addValueEntry(entries, humanizeKey(it.key()), it.value());
	}
	addTextEntry(entries, QStringLiteral("Crop"), formatCrop(params.value("crop").toObject()));
	return entries;
}


QString formatTaskInfoHtml(const Task *task) {
	const QJsonObject infoObj = task->info();
	const QJsonObject params = infoObj.value("parameters").toObject();
	QVector<InfoEntry> param_entries = formatGenericParameters(params);
	QVector<InfoEntry> entries = formatGenericParameters(infoObj);

	entries.append(param_entries);
	//entries.prepend({QStringLiteral("Type"), type});

	QString html = QStringLiteral("<table style=\"border-collapse:collapse;\">");
	for(const InfoEntry &entry: entries) {
		html += QStringLiteral("<tr><td style=\"padding:0 12px 4px 0;font-weight:bold;white-space:nowrap;vertical-align:top;\">%1</td><td style=\"padding:0 0 4px 0;vertical-align:top;\">%2</td></tr>")
			.arg(entry.label.toHtmlEscaped(), entry.value.toHtmlEscaped());
	}
	html += QStringLiteral("</table>");
	return html;
}
}

QueueItem::QueueItem(Task *_task, QListWidget *parent, bool history): QObject(parent), QListWidgetItem(parent), historyEntry(history) {
	task = _task;
	style[Task::ON_QUEUE] = "background-color:#323232; color:#b1b1b1";
	style[Task::RUNNING]  = "background-color:#328232; color:#b1b1b1";
	style[Task::PAUSED]   = "background-color:#828232; color:#b1b1b1";
	style[Task::STOPPED]  = "background-color:#823232; color:#b1b1b1";
//	style[Task::DONE]     = "background-color:#323282; color:#b1b1b1";
	style[Task::FAILED]   = "background-color:#823232; color:#b1b1b1";


	connect(task, SIGNAL(progress(QString,int)), this, SLOT(progress(QString,int)));

	widget = new QWidget;

	QGridLayout *grid = new QGridLayout();
	grid->setContentsMargins(12, 10, 12, 10);
	grid->setHorizontalSpacing(10);
	grid->setVerticalSpacing(4);
	grid->setColumnStretch(0, 1);
	grid->setColumnStretch(1, 1);
	grid->setColumnStretch(2, 0);

	title = new QLabel(QStringLiteral("<b>%1</b> %2").arg(task->label, task->output));
	title->setTextInteractionFlags(Qt::TextSelectableByMouse);
	grid->addWidget(title, 0, 0, 1, 2);

	pathLabel = new QLabel(task->output);
	pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	grid->addWidget(pathLabel, 1, 0, 1, 2);

	status = new QLabel();
	status->setMinimumWidth(250);
	grid->addWidget(status, 2, 0, 1, 1);

	progressbar = new QProgressBar();
	progressbar->setValue(0);
	progressbar->setTextVisible(false);
	grid->addWidget(progressbar, 2, 1, 1, 2);

	infoLabel = new QLabel();
	infoLabel->setWordWrap(true);
	infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	infoLabel->setTextFormat(Qt::RichText);
	infoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	infoLabel->hide();
	grid->addWidget(infoLabel, 3, 0, 1, 3);

	detailLabel = new QLabel();
	detailLabel->setWordWrap(true);
	detailLabel->hide();
	grid->addWidget(detailLabel, 4, 0, 1, 3);


	QHBoxLayout *actions = new QHBoxLayout();
	actions->setSpacing(4);
	grid->addLayout(actions, 0, 2, 1, 1, Qt::AlignRight);

	infoButton = makeButton(widget, "info", "Show task info", true);
	connect(infoButton, &QToolButton::clicked, this, &QueueItem::toggleDetails);
	actions->addWidget(infoButton);

	castButton = makeButton(widget, "cast", "Cast result");
	connect(castButton, &QToolButton::clicked, this, &QueueItem::casting);
	actions->addWidget(castButton);

	folderButton = makeButton(widget, "folder", "Open output folder");
	connect(folderButton, &QToolButton::clicked, this, &QueueItem::openFolder);
	actions->addWidget(folderButton);

	actions->addSpacing(12);

	removeButton = makeButton(widget, "trash-2", "Remove from queue");
	connect(removeButton, &QToolButton::clicked, this, &QueueItem::handleRemove);
	actions->addWidget(removeButton);

	widget->setLayout(grid);

	setSizeHint(widget->minimumSizeHint());
	parent->setItemWidget(this, widget);
	if(historyEntry)
		setBackground(parent->palette().base());

	updateContent();
}

void QueueItem::progress(QString text, int percent) {
	status->setText(text);
	if(progressbar)
		progressbar->setValue(percent);
}

void QueueItem::updateContent() {
	ProcessQueue &queue = ProcessQueue::instance();
	Q_UNUSED(queue);

	QString statusText;
	switch(task->status) {
	case Task::PAUSED:
		statusText = "Paused";
		break;
	case Task::ON_QUEUE:
		statusText = "Waiting";
		break;
	case Task::RUNNING:
		statusText = "Running";
		break;
	case Task::DONE:
		statusText = "Done";
		if(progressbar)
			progressbar->setValue(100);
		break;
	case Task::STOPPED:
		statusText = "Stopped";
		if(progressbar)
			progressbar->setValue(0);
		break;
	case Task::FAILED:
		statusText = task->error.isEmpty() ? "Failed" : task->error;
		if(progressbar)
			progressbar->setValue(0);
		break;
	}
	status->setText(statusText);

	refreshInfoLabel();

	refreshActions();
	refreshDetails();
	if(style.count(task->status))
		widget->setStyleSheet(style[task->status]);
	else
		widget->setStyleSheet(QString());
}

void openFile(const QString &filePath) {
	QMimeDatabase db;
	QMimeType type = db.mimeTypeForFile(filePath);

	if(type.isValid() && type.name() != "application/octet-stream") {
#ifdef Q_OS_WIN
		QProcess::startDetached("explorer", { QDir::toNativeSeparators(filePath) });
#elif defined(Q_OS_MAC)
		QProcess::startDetached("open", { filePath });
#elif defined(Q_OS_LINUX)
		QProcess::startDetached("xdg-open", { filePath });
#else
		QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
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

void QueueItem::handleRemove() {
	int answer = QMessageBox::question(widget, "Remove task",
								   QString("Remove '%1' from the queue?").arg(task->label),
								   QMessageBox::Yes | QMessageBox::No);
	if(answer != QMessageBox::Yes)
		return;

	if(historyEntry) {
		qRelightApp->project().removeTaskFromHistory(task->uuid.toString());
		if(QListWidget *list = listWidget()) {
			if(QWidget *frame = list->parentWidget())
				QMetaObject::invokeMethod(frame, "updateLists", Qt::QueuedConnection);
		}
	} else {
		ProcessQueue &queue = ProcessQueue::instance();
		if(queue.task == task)
			queue.stop();
		else
			queue.removeTask(task);
	}
}

void QueueItem::toggleDetails() {
	if(!infoButton)
		return;
	detailsVisible = infoButton->isChecked();
	refreshInfoLabel();
	refreshDetails();
}

void QueueItem::refreshActions() {
	const bool isRunning = task->status == Task::RUNNING;
	const bool isPaused = task->status == Task::PAUSED;
	const bool isQueued = task->status == Task::ON_QUEUE;
	const bool isDone = task->status == Task::DONE;

	progressbar->setVisible(!historyEntry && (isRunning || isQueued || isPaused));

	const bool canRemove = !historyEntry && !isRunning;

	removeButton->setVisible(true);
	removeButton->setEnabled(historyEntry || canRemove);
	if(historyEntry)
		removeButton->setToolTip("Remove from history");
	else
		removeButton->setToolTip(canRemove ? "Remove from queue" : "Stop task before removing");

	QFileInfo info(task->output);
	const bool hasOutput = info.exists();
	folderButton->setVisible(true);
	folderButton->setEnabled(hasOutput);
	bool canCast = hasOutput;
	castButton->setVisible(true);
	castButton->setEnabled(hasOutput && (isDone || historyEntry));
	castButton->setToolTip(hasOutput ? "Cast result" : "Casting available once processing finishes");
	infoButton->setVisible(true);
}

void QueueItem::refreshInfoLabel() {
	if(!infoLabel)
		return;
	const QString infoHtml = formatTaskInfoHtml(task);
	infoHasContent = !infoHtml.isEmpty();
	infoLabel->setText(infoHtml);
	updateSizeHintGeometry();
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
	const bool canShowInfo = hasDetails || infoHasContent;
	if(infoButton)
		infoButton->setEnabled(canShowInfo);
	if(!canShowInfo) {
		detailLabel->hide();
		if(infoButton)
			infoButton->setChecked(false);
		detailsVisible = false;
		if(infoLabel)
			infoLabel->hide();
		updateSizeHintGeometry();
		return;
	}
	detailLabel->setVisible(hasDetails && detailsVisible);
	if(infoLabel)
		infoLabel->setVisible(infoHasContent && detailsVisible);
	updateSizeHintGeometry();
}

void QueueItem::updateSizeHintGeometry() {
	if(!widget)
		return;
	widget->adjustSize();
	setSizeHint(widget->sizeHint());
}
