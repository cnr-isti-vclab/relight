#ifndef TASKPARAMETERS_H
#define TASKPARAMETERS_H

#include <QString>
#include <QJsonObject>

#include "crop.h"

class TaskParameters {
public:
    virtual ~TaskParameters() = default;

    QString path;
    int quality = 95;
    Crop crop;

    virtual QString summary() const = 0;
    virtual QJsonObject toJson() const = 0;

protected:
    QJsonObject baseJson() const {
        QJsonObject obj;
        obj["path"] = path;
        obj["quality"] = quality;
        obj["crop"] = serializeCrop();
        return obj;
    }

    QJsonObject serializeCrop() const {
        QJsonObject cropObj;
        cropObj["left"] = crop.left();
        cropObj["top"] = crop.top();
        cropObj["width"] = crop.width();
        cropObj["height"] = crop.height();
        cropObj["angle"] = crop.angle;
        return cropObj;
    }
};

#endif // TASKPARAMETERS_H
