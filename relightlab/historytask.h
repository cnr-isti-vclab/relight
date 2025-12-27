#ifndef HISTORYTASK_H
#define HISTORYTASK_H

#include "../src/task.h"

#include <QJsonObject>

class HistoryTask : public Task {
public:
	explicit HistoryTask(const QJsonObject &entry, QObject *parent = nullptr);
	QJsonObject info() const override;

private:
	QJsonObject snapshot;
};

#endif // HISTORYTASK_H
