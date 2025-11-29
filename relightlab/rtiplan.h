#ifndef RTIPLAN_H
#define RTIPLAN_H

#include <QFrame>
#include "../src/rti.h"
#include "../src/rti/rtitask.h"
#include "planrow.h"

class QComboBox;
class QCheckBox;
class QSpinBox;
class QLabelButton;
class HelpLabel;
class QVBoxLayout;
class QLineEdit;
class QLabel;

class RtiPlanRow: public PlanRow {
	Q_OBJECT
public:
	RtiPlanRow(RtiParameters &parameters, QFrame *parent = nullptr);

	RtiParameters &parameters;
};

class RtiBasisRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiBasisRow(RtiParameters &parameters, QFrame *parent = nullptr);
	void setBasis(Rti::Type basis, bool emitting = false);

private:
	QLabelButton *ptm, *hsh, *rbf, *bln;

signals:
	void basisChanged();
};


class RtiColorSpaceRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiColorSpaceRow(RtiParameters &parameters, QFrame *parent = nullptr);
	void setColorspace(Rti::ColorSpace colorspace, bool emitting = false);

private:
	QLabelButton *rgb, *lrgb, *mrgb, *ycc;

signals:
	void colorspaceChanged();
};


class RtiPlanesRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiPlanesRow(RtiParameters &parameters, QFrame *parent = nullptr);
	void setNPlanes(int nplanes, bool emitting = false);
	void setNChroma(int nchroma, bool emitting = false);
	void forceNPlanes(QList<int> nplanes);
	void forceNPlanes(int nplanes);
private:
	QComboBox *nplanesbox, *nchromabox;
	int nplanes[8] = { 6, 9, 12, 15, 18, 21, 24, 27 };
	int nchromas[3] = { 1, 2, 3 };
signals:
	void nplanesChanged();
};


class RtiFormatRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiFormatRow(RtiParameters &parameters, QFrame *parent = nullptr);
	void setFormat(RtiParameters::Format format, bool emitting = false);
	void allowLegacy(bool legacy);
private:
	RtiParameters::Format format;
	QLabelButton *rti, *web, *iip;

signals:
	void formatChanged();
};


class RtiQualityRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiQualityRow(RtiParameters &parameters, QFrame *parent = nullptr);
	void setQuality(int quality, bool emitting = false); //0 stands for lossless.
	void allowLossless(bool allow);
	void setColorProfileMode(ColorProfileMode mode, bool emitting = false);
	void updateProfileInfo(const QString &profileDesc, bool isSRGB);

private:
	QCheckBox *losslessbox;
	QSpinBox *qualitybox;
	QLabel *profileLabel;
	QLabelButton *preserve, *srgb;

signals:
	void qualityChanged();
	void colorProfileModeChanged();
};


class RtiWebLayoutRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiWebLayoutRow(RtiParameters &parameters, QFrame *parent = nullptr);
	void setWebLayout(RtiParameters::WebLayout layout, bool emitting = false); //0 stands for lossless.
private:
	QLabelButton *image, *deepzoom, *tarzoom, *itarzoom;
signals:
	void layoutChanged();
};

class RtiExportRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiExportRow(RtiParameters &parameters, QFrame *parent = nullptr);
	void setPath(QString path, bool emitting = false);
public slots:
	void selectOutput();
	void verifyPath();
	void suggestPath();

private:
	QLineEdit *path_edit;
};




#endif // RTIPLAN_H
