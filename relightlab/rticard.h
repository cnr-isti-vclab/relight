#ifndef RTICARD_H
#define RTICARD_H

#include <QFrame>


class RtiCard: public QFrame {
public:
	RtiCard(QString title, QString text, QWidget *parent = nullptr);

protected:
	//manage on click event
	void mousePressEvent(QMouseEvent *event) override;

private:
	bool selected = false;
};


#endif // RTICARD_H
