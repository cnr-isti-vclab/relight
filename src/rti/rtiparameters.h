#ifndef RTI_PARAMETERS_H
#define RTI_PARAMETERS_H

#include "../taskparameters.h"
#include "../rti.h"
#include "../colorprofile.h"

class RtiParameters : public TaskParameters {
public:
    enum Format { RTI = 0, WEB = 1, IIP = 2 };
    enum WebLayout { PLAIN = 0, DEEPZOOM = 1, TARZOOM = 2, ITARZOOM = 3 };

    Rti::Type basis = Rti::PTM;
    Rti::ColorSpace colorspace = Rti::RGB;
    int nplanes = 18;
    int nchroma = 0;

    Format format = WEB;
    WebLayout web_layout = PLAIN;

    bool lossless = false;

    bool iiif_manifest = false;
    bool openlime = true;

    ColorProfileMode colorProfileMode = COLOR_PROFILE_PRESERVE;

    QString summary() const override;
    QJsonObject toJson() const override;
};

#endif // RTI_PARAMETERS_H
