#include "planrow.h"
#include "helpbutton.h"

#include <QLabel>
#include <QHBoxLayout>


PlanRow::PlanRow(QFrame *parent): QFrame(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);

	label = new HelpLabel("", "");
	label->setMinimumWidth(200);
	layout->addWidget(label, 0, Qt::AlignLeft);
	//layout->setSpacing(20);

	layout->addStretch(1);

	buttonsFrame = new QFrame;
	buttonsFrame->setMinimumWidth(860);
	buttonsFrame->setMaximumWidth(1280);
	buttonsFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	buttonsFrame->setFrameStyle(QFrame::Box);

	layout->addWidget(buttonsFrame, 4);

	buttons = new QHBoxLayout(buttonsFrame);
//	buttons->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	layout->addStretch(1);
}
