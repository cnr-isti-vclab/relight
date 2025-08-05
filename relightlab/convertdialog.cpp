#include "convertdialog.h"
#include "helpbutton.h"
#include "qlabelbutton.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QButtonGroup>

ConvertDialog::ConvertDialog() {
//create two sections
	QVBoxLayout *layout = new QVBoxLayout(this);
	
	HelpLabel *input_label = new HelpLabel("<p>Input: .ptm, .rti or relight folder:</p>", "convert /export");
	layout->addWidget(input_label);
	{
		{
			QHBoxLayout *input_layout = new QHBoxLayout();
			layout->addLayout(input_layout);
		
			QLineEdit *input_path = new QLineEdit;
			input_layout->addWidget(input_path);
			//connect(file_path, &QLineEdit::editingFinished,this, &RtiExportRow::verifyPath);

			QPushButton *input_button = new QPushButton("...");
			input_layout->addWidget(input_button);
			//connect(file_button, &QPushButton::clicked, this, &RtiExportRow::selectOutput);
		}


		{
			QHBoxLayout *output_layout = new QHBoxLayout();
			layout->addLayout(output_layout);

			QLabelButton *rti, *web, *iip;

			output_layout->addWidget(rti = new QLabelButton("Legacy", ".rti, .ptm"));
			output_layout->addWidget(web = new QLabelButton("Web", ".json, .jpg"));
			output_layout->addWidget(iip = new QLabelButton("IIP", ".tiff"));

//			connect(rti, &QAbstractButton::clicked, [this]() { setFormat(RtiParameters::RTI, true); });
//			connect(web, &QAbstractButton::clicked, [this]() { setFormat(RtiParameters::WEB, true); });
//			connect(iip, &QAbstractButton::clicked, [this]() { setFormat(RtiParameters::IIP, true); });

			QButtonGroup *group = new QButtonGroup(this);
			group->addButton(rti);
			group->addButton(web);
			group->addButton(iip);
		}
		{
			QPushButton *convert = new QPushButton("Convert");
			layout->addWidget(convert);

			convert->setIcon(QIcon::fromTheme("save"));
			convert->setProperty("class", "large");
			connect(convert, &QPushButton::clicked, [this]() { convertToRelight(); });

		}

	}
}

void ConvertDialog::convertToRelight() {

}
void ConvertDialog::convertToRti() {

}
