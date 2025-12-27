#include "brdfparameters.h"

namespace {

QString albedoMethodToString(BrdfParameters::AlbedoMethod method) {
    switch(method) {
    case BrdfParameters::NONE: return QStringLiteral("NONE");
    case BrdfParameters::MEDIAN: return QStringLiteral("MEDIAN");
    case BrdfParameters::MEAN: return QStringLiteral("MEAN");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

}

QString BrdfParameters::summary() const {
    QString ret;
    if(albedo == MEDIAN)
        ret = "Median";
    return ret;
}

QJsonObject BrdfParameters::toJson() const {
    QJsonObject obj = baseJson();
    obj["taskType"] = "BRDF";
    obj["inputPath"] = input_path;
    obj["albedoMethod"] = albedoMethodToString(albedo);
    obj["medianPercentage"] = median_percentage;
    obj["albedoPath"] = albedo_path;
    return obj;
}
