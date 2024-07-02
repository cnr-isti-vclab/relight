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
	RtiPlanRow(QFrame *parent = nullptr);
	HelpLabel *label = nullptr;
	QHBoxLayout *buttons = nullptr;
};

class RtiBasisRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiBasisRow(QFrame *parent = nullptr);
	void init(Rti::Type basis);
	Rti::Type basis;
	QLabelButton *ptm, *hsh, *rbf, *bln;
signals:
	void basisChanged(Rti::Type basis);

};

class RtiColorSpaceRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiColorSpaceRow(QFrame *parent = nullptr);
	void init(Rti::Type basis, Rti::ColorSpace colorspace);
	Rti::ColorSpace colorspace;
	QLabelButton *rgb, *lrgb, *mrgb, *ycc;

signals:
	void colorspaceChanged(Rti::ColorSpace colorspace);
};

class RtiPlanesRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiPlanesRow(QFrame *parent = nullptr);
	void init(Rti::ColorSpace colorspace, int nplanes, int nchroma);
	int nplanes;
	int nchroma;
	QComboBox *nplanesbox, *nchromabox;

signals:
	void nplanesChanged(int nplanes, int nchroma);
};

class RtiFormatRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiFormatRow(QFrame *parent = nullptr);
	void init(RtiParameters::Format format);
	RtiParameters::Format format;
	QLabelButton *rti, *web, *iip;

signals:
	void formatChanged(RtiParameters::Format format);
};

class RtiQualityRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiQualityRow(QFrame *parent = nullptr);
	void init(int quality); //0 stands for lossless.
	int quality = 95;
	QSpinBox *qualitybox;

signals:
	void qualityChanged(int quality);
};

class RtiWebLayoutRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiWebLayoutRow(QFrame *parent = nullptr);
	void init(RtiParameters::WebLayout layout); //0 stands for lossless.
	RtiParameters::WebLayout layout;
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
	void basisChanged(Rti::Type basis);
	void colorspaceChanged(Rti::ColorSpace colorspace);
	void nplanesChanged(int nplanes, int nchroma);
	void formatChanged(RtiParameters::Format format);
	void qualityChanged(int quality);
	void layoutChanged(RtiParameters::WebLayout layout);

private:
	RtiBasisRow *basis_row = nullptr;
	RtiColorSpaceRow *colorspace_row = nullptr;
	RtiPlanesRow *planes_row = nullptr;
	RtiFormatRow *format_row = nullptr;
	RtiQualityRow *quality_row = nullptr;
	RtiWebLayoutRow *layout_row = nullptr;
};

#endif // RTIPLAN_H
