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

protected:
	HelpLabel *label = nullptr;
	QHBoxLayout *buttons = nullptr;
};

class RtiBasisRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiBasisRow(QFrame *parent = nullptr);
	void init(Rti::Type basis);
signals:
	void basisChanged(Rti::Type basis);
private:
	Rti::Type basis;
	QLabelButton *ptm, *hsh, *rbf, *bln;
};

class RtiColorSpaceRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiColorSpaceRow(QFrame *parent = nullptr);
	void init(Rti::ColorSpace colorspace);
signals:
	void colorspaceChanged(Rti::ColorSpace colorspace);
private:
	Rti::ColorSpace colorspace;
	QLabelButton *rgb, *lrgb, *mrgb, *ycc;
};

class RtiPlanesRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiPlanesRow(QFrame *parent = nullptr);
	void init(int nplanes, int nchroma);
signals:
	void nplanesChanged(int nplanes, int nchroma);
private:
	int nplanes;
	int nchroma;
	QComboBox *nplanesbox, *nchromabox;
};

class RtiFormatRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiFormatRow(QFrame *parent = nullptr);
	void init(RtiParameters::Format format);
signals:
	void formatChanged(RtiParameters::Format format);
private:
	RtiParameters::Format format;
	QLabelButton *rti, *web, *iip;
};

class RtiQualityRow: public RtiPlanRow {
	Q_OBJECT
public:
	RtiQualityRow(QFrame *parent = nullptr);
	void init(int quality); //0 stands for lossless.
signals:
	void formatChanged(RtiParameters::Format format);
private:
	int quality = 95;
	QSpinBox *qualitybox;
	QLabelButton *rti, *img, *deepzoom;
};


class RtiPlan: public QFrame {
	Q_OBJECT
public:
	RtiPlan(QWidget *parent = nullptr);
public slots:
	void basisChanged(Rti::Type basis);
	void colorspaceChanged(Rti::ColorSpace colorspace);
	void nplanesChanged(int nplanes, int nchroma);
	void formatChanged(RtiParameters::Format format);
private:
	RtiBasisRow *basis_row = nullptr;
	RtiColorSpaceRow *colorspace_row = nullptr;
	RtiPlanesRow *planes_row = nullptr;
	RtiFormatRow *format_row = nullptr;
};

#endif // RTIPLAN_H
