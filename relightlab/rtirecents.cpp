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

		content->addWidget(new QLabelButton(p.summary()));
	}
}
