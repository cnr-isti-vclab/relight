#include "brdfframe.h"
#include "relightapp.h"
#include "reflectionview.h"
#include "helpbutton.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>

//export various planes.

BrdfMedianRow::BrdfMedianRow(BrdfParameters &_parameters, QFrame *parent):
	PlanRow(parent), parameters(_parameters) {

	label->label->setText("Median image");
	buttons->addWidget(new QLabel("Light percentage:"));
	buttons->addWidget(median_slider = new QSlider);
	buttons->addWidget(median_text = new QLabel);
	int percent = int(parameters.median_percentage);
	median_slider->setSliderPosition(percent);
	median_text->setText(QString::number(percent));
	connect(median_slider, &QSlider::valueChanged, [this](int v) { median_text->setText(QString::number(v)); });
}

BrdfFrame::BrdfFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new QLabel("<h2>BRDF creation</h2>"));
	content->addSpacing(30);

	median_row = new BrdfMedianRow(parameters, this);
	content->addWidget(median_row);
	content->addStretch(1);

	{
		QHBoxLayout *save_row = new QHBoxLayout;

		{
			QLabel *label = new QLabel("");
			label->setFixedWidth(200);
			save_row->addWidget(label, 0, Qt::AlignLeft);
		}
		save_row->addStretch(1);

		{
			QFrame *buttons_frame = new QFrame;
			buttons_frame->setMinimumWidth(860);

			{
				QHBoxLayout *buttons_layout = new QHBoxLayout(buttons_frame);

				zoom_view = new ZoomOverview(qRelightApp->project().crop, 200);
				buttons_layout->addWidget(zoom_view);

				buttons_layout->addStretch(1);
				QPushButton *save = new QPushButton("Export", this);
				save->setIcon(QIcon::fromTheme("save"));
				save->setProperty("class", "large");
				save->setMinimumWidth(200);
				//connect(save, &QPushButton::clicked, [this]() { this->save(); });

				buttons_layout->addWidget(save);
			}

			save_row->addWidget(buttons_frame);
		}
		save_row->addStretch(1);


		content->addLayout(save_row);

	}


	content->addStretch();
}
