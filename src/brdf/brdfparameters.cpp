#include "brdfparameters.h"

QString BrdfParameters::summary() const {
    QString ret;
    if(albedo == MEDIAN)
        ret = "Median";
    return ret;
}

QJsonObject BrdfParameters::toJson() const {
    QJsonObject obj = baseJson();
    obj["inputPath"] = input_path;
    obj["albedoMethod"] = static_cast<int>(albedo);
    obj["medianPercentage"] = median_percentage;
    obj["albedoPath"] = albedo_path;
    return obj;
}
