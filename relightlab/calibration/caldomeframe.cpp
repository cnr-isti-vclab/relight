#include "caldomeframe.h"
#include "../../src/calibration/calibrationsession.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QFrame>
#include <QScrollArea>

CalDomeFrame::CalDomeFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);

	QScrollArea *scroll = new QScrollArea;
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	outer->addWidget(scroll);

	QWidget *body = new QWidget;
	scroll->setWidget(body);

	QVBoxLayout *layout = new QVBoxLayout(body);
	layout->setContentsMargins(16, 16, 16, 16);
	layout->setSpacing(16);

	// ---- Instructions ---------------------------------------------------
	QLabel *intro = new QLabel(
		"<b>Dome calibration workflow</b><br><br>"
		"Follow these steps to calibrate your dome:<br>"
		"<ol>"
		"<li><b>Dome</b> \u2013 enter a label and notes for this dome configuration.</li>"
		"<li><b>Distortion</b> \u2013 supply lens-distortion coefficients so that "
		    "sphere highlight positions can be corrected before computing light "
		    "directions.</li>"
		"<li><b>Lights</b> \u2013 choose the light-position model (Directional / "
		    "Spherical / 3-D), then load an <tt>.lp</tt> file or derive "
		    "directions from a reflective sphere. For Spherical and 3-D modes, "
		    "enter the matching dome geometry.</li>"
		"<li><b>Flatfield</b> \u2013 optionally correct per-LED vignetting with a "
		    "gain map.</li>"
		"</ol>"
		"When finished, press <b>Save dome\u2026</b> to write the configuration to disk."
	);
	intro->setWordWrap(true);
	intro->setTextFormat(Qt::RichText);
	intro->setContentsMargins(8, 8, 8, 8);
	intro->setStyleSheet("QLabel { background: palette(base); border: 1px solid palette(mid); border-radius: 4px; }");
	layout->addWidget(intro);

	// ---- Dome identity --------------------------------------------------
	QGroupBox *id_box = new QGroupBox("Dome identity");
	QFormLayout *id_form = new QFormLayout(id_box);
	id_form->setRowWrapPolicy(QFormLayout::WrapLongRows);

	label_edit = new QLineEdit;
	label_edit->setPlaceholderText("e.g. \"Lab dome A\"");
	id_form->addRow("Label:", label_edit);

	notes_edit = new QTextEdit;
	notes_edit->setAcceptRichText(false);
	notes_edit->setPlaceholderText("Optional notes (camera, setup date, environment\u2026)");
	notes_edit->setFixedHeight(90);
	id_form->addRow("Notes:", notes_edit);

	layout->addWidget(id_box);
	layout->addStretch(1);

	// ---- Wire up live editing ------------------------------------------
	connect(label_edit, &QLineEdit::textChanged,  this, &CalDomeFrame::uiToSession);
	connect(notes_edit, &QTextEdit::textChanged,  this, &CalDomeFrame::uiToSession);
}

void CalDomeFrame::setSession(CalibrationSession *s) {
	session = s;
	sessionToUi();
}

void CalDomeFrame::sessionToUi() {
	if (!session) return;
	label_edit->setText(session->dome.label);
	notes_edit->setPlainText(session->dome.notes);
}

void CalDomeFrame::uiToSession() {
	if (!session) return;
	session->dome.label = label_edit->text();
	session->dome.notes = notes_edit->toPlainText();
}
