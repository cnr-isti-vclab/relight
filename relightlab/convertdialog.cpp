#include "convertdialog.h"
#include "helpbutton.h"
#include "qlabelbutton.h"
#include "../relight/zoom.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QButtonGroup>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>


int convertToRTI(const char *filename, const char *output);
int convertRTI(const char *file, const char *output, int quality);

ConvertDialog::ConvertDialog() {
//create two sections
	QVBoxLayout *layout = new QVBoxLayout(this);
	
	HelpLabel *input_label = new HelpLabel("<p>Input: .ptm, .rti or info.json:</p>", "convert /export");
	layout->addWidget(input_label);
	{
		{
			QHBoxLayout *input_layout = new QHBoxLayout();
			layout->addLayout(input_layout);
		
			input_path = new QLineEdit;
			input_layout->addWidget(input_path);
			connect(input_path, &QLineEdit::editingFinished,this, &ConvertDialog::verifyPath);

			QPushButton *input_button = new QPushButton("...");
			input_layout->addWidget(input_button);
			connect(input_button, &QPushButton::clicked, this, &ConvertDialog::selectInput);
		}


		{
			QHBoxLayout *output_layout = new QHBoxLayout();
			layout->addLayout(output_layout);

			output_layout->addWidget(rti = new QLabelButton("Legacy", ".rti, .ptm"));
			output_layout->addWidget(web = new QLabelButton("Web", ".json, .jpg"));
			output_layout->addWidget(iip = new QLabelButton("IIP", ".tiff"));

			QButtonGroup *group = new QButtonGroup(this);
			group->addButton(rti);
			group->addButton(web);
			group->addButton(iip);
		}

		{
			QHBoxLayout *web_layout = new QHBoxLayout();
			layout->addLayout(web_layout);

			web_layout->addWidget(img = new QLabelButton("Images", ".jpg"));
			web_layout->addWidget(deepzoom = new QLabelButton("DeepZoom", ".dzi"));
			web_layout->addWidget(tarzoom = new QLabelButton("TarZoom", ".tarzoom"));
			web_layout->addWidget(itarzoom = new QLabelButton("iTarZoom", ".itarzoom"));

			QButtonGroup *group = new QButtonGroup(this);
			group->addButton(img);
			group->addButton(deepzoom);
			group->addButton(tarzoom);
			group->addButton(itarzoom);
		}
		
		{
			convert_button = new QPushButton("Convert");
			convert_button->setToolTip("Convert to selected format");
			convert_button->setEnabled(false);
			layout->addWidget(convert_button);

			convert_button->setIcon(QIcon::fromTheme("save"));
			convert_button->setProperty("class", "large");
			connect(convert_button, &QPushButton::clicked, this, &ConvertDialog::convert);
		}

	}
}


void ConvertDialog::selectInput() {
	//allow user to select a file or folder
	QFileDialog dialog(this, "Select input file (.ptm, .rti, .json)");
	dialog.setFileMode(QFileDialog::ExistingFiles);

	dialog.setNameFilter("Relight files (*.ptm *.rti *.json);;All files (*)");
	if (dialog.exec() != QDialog::Accepted) 
		return;
	QStringList files = dialog.selectedFiles();
	if (files.isEmpty())
		return;

	QString file = files.first();

	bool legacy = file.endsWith(".ptm") || file.endsWith(".rti");
	rti->setEnabled(!legacy);
	rti->setChecked(!legacy);
	web->setEnabled(legacy);
	web->setChecked(legacy);

	img->setEnabled(legacy);
	img->setChecked(true);
	deepzoom->setEnabled(legacy);
	tarzoom->setEnabled(legacy);
	itarzoom->setEnabled(legacy);
	
	input_path->setText(files.first());
	verifyPath();
}

void ConvertDialog::convert() {
	verifyPath();

	QString path = input_path->text();
	bool legacy = path.endsWith(".ptm") || path.endsWith(".rti");

	if (rti->isChecked()) {
		relightToRti(path);
	} else if (web->isChecked()) {
		if(legacy)
			rtiToRelight(path);
		else
			relightToRelight(path);
	} else if (iip->isChecked()) {
		QMessageBox::information(this, "To be implemented!", "IIP multi plane tif format to be implemented");
	}
}

void ConvertDialog::verifyPath() {
	convert_button->setEnabled(false);
	if (!input_path || input_path->text().isEmpty()) {
		QMessageBox::warning(this, "Input required", "Please select an input file.");
		return;
	}

	QFileInfo fileInfo(input_path->text());
	if (!fileInfo.exists()) {
		QMessageBox::warning(this, "File not found", "The selected file does not exist:\n" + fileInfo.absoluteFilePath());
		return;
	}

	// Enable convert button if a valid file is selected
	convert_button->setEnabled(true);
}

void ConvertDialog::relightToRti(QString input) {

}

void ConvertDialog::relightToRelight(QString path) {

}

void ConvertDialog::rtiToRelight(QString input) {
	int quality = 95;
	try {
		if (!(input.endsWith(".ptm") || input.endsWith(".rti"))) {
			QMessageBox::warning(this, "Invalid file", "Please select a valid .ptm or .rti file.");
			return;
		}
		QString output = input.mid(0, input.lastIndexOf('.'));
		QDir output_dir(output);
		if(output_dir.exists()) {
			auto answer = QMessageBox::question(this, "Folder already exists", "Folder '" + output + "' already exists, overwrite?");
			if(answer == QMessageBox::No) {
				return;
			}
		}
		int status = convertRTI(input.toStdString().c_str(), output.toStdString().c_str(), quality);
		if (status != 0) {
			QMessageBox::critical(this, "Conversion failed", "Failed to convert to .relight format.");
			return;
		}
		QMessageBox::information(this, "Conversion successful", "Successfully converted to .relight format.");
		// apply the conversion to deepzoom, tarzoom, itarzoom if applicable
		
		std::function<bool(QString s, int d)> callback = [this](QString s, int n)->bool { return true; };

		if (deepzoom->isChecked() || tarzoom->isChecked() || itarzoom->isChecked()) {
			deepZoom(output, output, quality, 0, 256, callback);
		}

		if (tarzoom->isChecked() || itarzoom->isChecked()) {
			tarZoom(output, output, callback);
		}

		if (itarzoom->isChecked()) {
			itarZoom(output, output, callback);
		}
		// Optionally, open the output directory
		QDir dir(output);
		if (!dir.exists()) {
			QMessageBox::warning(this, "Something went wrong!", "The output folder does not exist:\n" + dir.absolutePath());
			return;
		}
		QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
			
	} catch (QString e) {
		QMessageBox::critical(this, "Error", QString("An error occurred: %1").arg(e));
	}

}

