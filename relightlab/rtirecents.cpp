#include "rtirecents.h"
#include "qlabelbutton.h"
#include "recentprojects.h"

#include <QSettings>
#include <QHBoxLayout>
#include <QStringList>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>

using namespace std;

vector<RtiParameters> recentRtis() {
	vector<RtiParameters> params;

	QStringList recents = QSettings().value("recent-rtis", QStringList()).toStringList();
	for(QString &s: recents) {
		QJsonDocument doc = QJsonDocument::fromJson(s.toUtf8());
		QJsonObject obj = doc.object();
		RtiParameters p;
		p.basis = (Rti::Type)obj["basis"].toInt();
		p.colorspace = (Rti::ColorSpace) obj["colorspace"].toInt();
		p.nplanes = obj["nplanes"].toInt();
		p.nchroma = obj["nchroma"].toInt();
		p.format = (RtiParameters::Format) obj["format"].toInt();

		params.push_back(p);
	}
	return params;
}

void addRecentRti(const RtiParameters &params) {
	QSettings settings;
	QStringList recents = settings.value("recent-rtis", QStringList()).toStringList();
	QJsonObject obj;
	obj["basis"] = params.basis;
	obj["colorspace"] = params.colorspace;
	obj["nplanes"] = params.nplanes;
	obj["nchroma"] = params.nchroma;
	obj["format"] = params.format;
	QJsonDocument doc(obj);
	recents.prepend(doc.toJson());
	while(recents.size() > 5) {
		recents.removeLast();
	}
	settings.setValue("recent-rtis", recents);
}


RtiRecents::RtiRecents(QFrame *parent): QFrame(parent) {
	QHBoxLayout *content = new QHBoxLayout(this);

	std::vector<RtiParameters> params = recentRtis();
	for(RtiParameters &p: params) {
		QString basisLabels[] =  { "PTM", "HSH", "RBF", "BLN", "NEURAL" };
		QString colorspaceLabels[] =  { "RGB", "LRGB", "YCC", "RGB", "YCC" };
		QString formatLabels[] = { "", "relight", "deepzoom", "tarzoom", "itarzoom", "tiff" };

		QString basis  = basisLabels[p.basis];
		QString colorspace = colorspaceLabels[p.colorspace];
		QString planes = QString::number(p.nplanes);
		if(p.nchroma) {
			planes += "." + QString::number(p.nchroma);
		}
		QString format;
		if(p.format == RtiParameters::RTI)
			format = p.basis == Rti::PTM ? ".ptm" : ".rti";
		else
			format = formatLabels[p.format];

		QString txt = QString("<h2>%1 <span style='font-size:80%'>(%2)</span> %3</h2>"
							  "<p>%4</p>").arg(basis).arg(colorspace).arg(planes).arg(format);
		content->addWidget(new QLabelButton(txt));
	}
}
