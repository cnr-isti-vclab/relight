#include "rtiparameters.h"

QString RtiParameters::summary() const {
    QString basisLabels[] =  { "PTM", "HSH", "RBF", "BLN", "NEURAL" };
    QString colorspaceLabels[] =  { "RGB", "LRGB", "YCC", "RGB", "YCC" };
    QString formatLabels[] = { "web: images", "web: deepzoom", "web: tarzoom", "web: itarzoom"};

    QString s_basis  = basisLabels[basis];
    QString s_colorspace = colorspaceLabels[colorspace];
    QString s_planes = QString::number(nplanes);
    if(nchroma) {
        s_planes += "." + QString::number(nchroma);
    }
    QString s_format;
    switch(format) {
    case RtiParameters::RTI:
        s_format = basis == Rti::PTM ? ".ptm" : ".rti";
        break;
    case WEB:
        s_format = formatLabels[web_layout];
        break;
    case IIP:
        s_format =  "IIIF: tiff";
        break;
    }

    QString txt = QString("%1%3 (%2) %4").arg(s_basis).arg(s_colorspace).arg(s_planes).arg(s_format);
    return txt;
}

QJsonObject RtiParameters::toJson() const {
    QJsonObject obj = baseJson();
    obj["basis"] = static_cast<int>(basis);
    obj["colorspace"] = static_cast<int>(colorspace);
    obj["nplanes"] = nplanes;
    obj["nchroma"] = nchroma;
    obj["format"] = static_cast<int>(format);
    obj["webLayout"] = static_cast<int>(web_layout);
    obj["lossless"] = lossless;
    obj["iiifManifest"] = iiif_manifest;
    obj["openlime"] = openlime;
    obj["colorProfileMode"] = static_cast<int>(colorProfileMode);
    return obj;
}
