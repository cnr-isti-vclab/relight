#ifndef RTIPLAN_H
#define RTIPLAN_H

#include <QFrame>
#include "../src/rti.h"
#include "../relight/rtitask.h"

class QComboBox;
class QSpinBox;
class QLabelButton;
class HelpLabel;
class QHBoxLayout;

class RtiPlanRow: public QFrame {
	Q_OBJECT
public:
	RtiPlanRow(RtiParameters &parameters, QFrame *parent = nullptr);

	RtiParameters &parameters;
	HelpLabel *label = nullptr;
	QHBoxLayout *buttons = nullptr;
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

private:
	QComboBox *nplanesbox, *nchromabox;
	int nimages[7] = { 3, 4, 5, 6, 7, 8, 9 };
	int nchromas[3] = { 1, 2, 3 };
signals:
	void nplanesChanged();
};


class RtiFormatRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiFormatRow(RtiParameters &parameters, QFrame *parent = nullptr);
	void setFormat(RtiParameters::Format format, bool emitting = false);

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

private:
	QSpinBox *qualitybox;

signals:
	void qualityChanged();
};


class RtiWebLayoutRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiWebLayoutRow(RtiParameters &parameters, QFrame *parent = nullptr);
	void setWebLayout(RtiParameters::WebLayout layout); //0 stands for lossless.

signals:
	void layoutChanged(RtiParameters::WebLayout layout);

};


class RtiPlan: public QFrame {
	Q_OBJECT
public:
	RtiPlan(QWidget *parent = nullptr);
	void setParameters(RtiParameters parameters);

	RtiParameters parameters;

public slots:
	void basisChanged();
	void colorspaceChanged();
	void nplanesChanged();
	void formatChanged();
	void qualityChanged();
	void layoutChanged();

private:
	RtiBasisRow *basis_row = nullptr;
	RtiColorSpaceRow *colorspace_row = nullptr;
	RtiPlanesRow *planes_row = nullptr;
	RtiFormatRow *format_row = nullptr;
	RtiQualityRow *quality_row = nullptr;
	RtiWebLayoutRow *layout_row = nullptr;
};

#endif // RTIPLAN_H
