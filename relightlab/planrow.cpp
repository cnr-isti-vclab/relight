#include "planrow.h"
#include "helpbutton.h"

#include <QLabel>
#include <QHBoxLayout>


PlanRow::PlanRow(QFrame *parent): QFrame(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);

	label = new HelpLabel("", "");
	label->setMinimumWidth(200);
	layout->addWidget(label, 0, Qt::AlignLeft);

	layout->addStretch(1);

	{
		planFrame = new QFrame;
		planFrame->setMinimumWidth(860);
		planFrame->setMaximumWidth(1280);
		planFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		planFrame->setFrameStyle(QFrame::Box);

		planLayout = new QVBoxLayout(planFrame);
		planLayout->setSpacing(24);
		{
			buttons = new QHBoxLayout;
			planLayout->addLayout(buttons);
		}
		layout->addWidget(planFrame, 4);
	}

	layout->addStretch(1);
}
