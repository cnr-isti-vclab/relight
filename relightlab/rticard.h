#ifndef RTICARD_H
#define RTICARD_H

#include <QFrame>
#include "../src/rti.h"


class HelpLabel;

class RtiCard: public QFrame {
	Q_OBJECT
public:
	Rti::Type type;
	Rti::ColorSpace colorspace;
	int nplanes;
	int nchroma;

	RtiCard(Rti::Type type, Rti::ColorSpace colorspace, int nplanes, int nchroma, QWidget *parent = nullptr);
	void setCheckable(bool);

public slots:
	void setChecked(bool);
	void updateTitle();
	void rtiExport();
signals:
	void toggled(bool checked);

protected:
	//manage on click event
	void mousePressEvent(QMouseEvent *event) override;

private:
	HelpLabel *title_label = nullptr;
	bool checkable = false;
	bool checked = false;

};


#endif // RTICARD_H
