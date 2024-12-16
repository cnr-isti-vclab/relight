#include "brdfframe.h"

#include <QVBoxLayout>

//export various planes.

BrdfFrame::BrdfFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addSpacing(10);
}
