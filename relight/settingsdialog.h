#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = 0);
	~SettingsDialog();

public slots:
	void openPython();
	void openScripts();
	void setPython();
	void setScripts();
	void setPort();
	void setRam();
	void setWorkers();

private:
	bool checkPython(QString python);
	bool checkScripts(QString scripts);

	Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
