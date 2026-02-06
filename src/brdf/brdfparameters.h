#ifndef BRDF_PARAMETERS_H
#define BRDF_PARAMETERS_H

#include "../taskparameters.h"

class BrdfParameters : public TaskParameters {
public:
    QString input_path;
    enum AlbedoMethod { NONE, MEDIAN, MEAN };
    AlbedoMethod albedo = MEDIAN;
    float median_percentage = 66;

	QString albedo_path = "albedo";

    QString summary() const override;
    QJsonObject toJson() const override;
};

#endif // BRDF_PARAMETERS_H
