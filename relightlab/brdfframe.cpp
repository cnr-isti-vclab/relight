#include "brdfframe.h"

#include <QVBoxLayout>
#include <QLabel>

//export various planes.

BrdfFrame::BrdfFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new QLabel("<h2>BRDF creation</h2>"));
	content->addSpacing(20);

//	median_row =
}
