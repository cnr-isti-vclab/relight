#include "historytask.h"

#include <QDateTime>
#include <QUuid>
#include <QJsonValue>

HistoryTask::HistoryTask(const QJsonObject &entry, QObject *parent): Task(parent), snapshot(entry) {
	uuid = QUuid(entry.value("uuid").toString());
	id = entry.value("id").toInt();
	label = entry.value("label").toString();
	output = entry.value("output").toString();
	status = static_cast<Status>(entry.value("status").toInt(static_cast<int>(Task::DONE)));
	const QJsonValue mimeValue = entry.value("mime");
	mime = Task::mimeFromString(mimeValue.toString());
	startedAt = QDateTime::fromString(entry.value("startedAt").toString(), Qt::ISODateWithMs);
	log = entry.value("log").toString();
	error = entry.value("error").toString();
	visible = entry.contains("visible") ? entry.value("visible").toBool(true) : true;
}

QJsonObject HistoryTask::info() const {
	return snapshot;
}
