#include "rtiparameters.h"

namespace {

QString basisToString(Rti::Type basis) {
    switch(basis) {
    case Rti::PTM: return QStringLiteral("PTM");
    case Rti::HSH: return QStringLiteral("HSH");
    case Rti::RBF: return QStringLiteral("RBF");
    case Rti::BILINEAR: return QStringLiteral("BLN");
    case Rti::NEURAL: return QStringLiteral("NEURAL");
    case Rti::DMD: return QStringLiteral("DMD");
    case Rti::SH: return QStringLiteral("SH");
    case Rti::H: return QStringLiteral("H");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

QString colorSpaceToString(Rti::ColorSpace space) {
    switch(space) {
    case Rti::RGB: return QStringLiteral("RGB");
    case Rti::LRGB: return QStringLiteral("LRGB");
    case Rti::YCC: return QStringLiteral("YCC");
    case Rti::MRGB: return QStringLiteral("MRGB");
    case Rti::MYCC: return QStringLiteral("MYCC");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

QString formatToString(RtiParameters::Format format) {
    switch(format) {
    case RtiParameters::RTI: return QStringLiteral("RTI");
    case RtiParameters::WEB: return QStringLiteral("WEB");
    case RtiParameters::IIP: return QStringLiteral("IIP");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

QString webLayoutToString(RtiParameters::WebLayout layout) {
    switch(layout) {
    case RtiParameters::PLAIN: return QStringLiteral("PLAIN");
    case RtiParameters::DEEPZOOM: return QStringLiteral("DEEPZOOM");
    case RtiParameters::TARZOOM: return QStringLiteral("TARZOOM");
    case RtiParameters::ITARZOOM: return QStringLiteral("ITARZOOM");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

QString colorProfileModeToString(ColorProfileMode mode) {
    switch(mode) {
    case COLOR_PROFILE_PRESERVE: return QStringLiteral("COLOR_PROFILE_PRESERVE");
    case COLOR_PROFILE_SRGB: return QStringLiteral("COLOR_PROFILE_SRGB");
    case COLOR_PROFILE_DISPLAY_P3: return QStringLiteral("COLOR_PROFILE_DISPLAY_P3");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

}

QString RtiParameters::summary() const {
    const QString s_basis = basisToString(basis);
    const QString s_colorspace = colorSpaceToString(colorspace);
    QString s_planes = QString::number(nplanes);
    if(nchroma) {
        s_planes += "." + QString::number(nchroma);
    }
    QString s_format;
    switch(format) {
    case RtiParameters::RTI:
        s_format = basis == Rti::PTM ? QStringLiteral(".ptm") : QStringLiteral(".rti");
        break;
    case WEB:
        s_format = QStringLiteral("web: %1").arg(webLayoutToString(web_layout).toLower());
        break;
    case IIP:
        s_format =  QStringLiteral("IIIF: tiff");
        break;
    }

    QString txt = QString("%1%3 (%2) %4").arg(s_basis).arg(s_colorspace).arg(s_planes).arg(s_format);
    return txt;
}

QJsonObject RtiParameters::toJson() const {
    QJsonObject obj = baseJson();
    obj["taskType"] = "RTI";
    obj["basis"] = basisToString(basis);
    obj["colorSpace"] = colorSpaceToString(colorspace);
    obj["planeCount"] = nplanes;
    if(nchroma > 0)
        obj["chromaPlanes"] = nchroma;
    obj["format"] = formatToString(format);
    obj["webLayout"] = webLayoutToString(web_layout);
    obj["lossless"] = lossless;
    obj["iiifManifest"] = iiif_manifest;
    obj["openlime"] = openlime;
    obj["colorProfileMode"] = colorProfileModeToString(colorProfileMode);
    return obj;
}
