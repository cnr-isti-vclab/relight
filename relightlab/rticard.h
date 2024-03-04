#ifndef RTICARD_H
#define RTICARD_H

#include <QFrame>
#include "../src/rti.h"


class RtiCard: public QFrame {
	Q_OBJECT
public:
	RtiCard(Rti::Type type, QWidget *parent = nullptr);

public slots:
	void setChecked(bool);

signals:
	void toggled(bool checked);

protected:
	//manage on click event
	void mousePressEvent(QMouseEvent *event) override;

private:
	bool checked = false;

};


#endif // RTICARD_H
