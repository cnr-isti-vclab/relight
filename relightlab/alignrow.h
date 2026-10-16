#ifndef ALIGNROW_H
#define ALIGNROW_H

#include "../src/align/align_task.h"
#include <QWidget>




class Align;
class QLabel;
class QProgressBar;
class QPushButton;
class AlignOverview;

class AlignRow: public QWidget {
	Q_OBJECT
public:
	int rowHeight = 92;
	Align *align = nullptr;
	QLabel *thumb = nullptr;
	AlignOverview *position = nullptr;
	QLabel *status = nullptr;
	QLabel *region = nullptr;
	QProgressBar *progress = nullptr;
	QPushButton *verify_button = nullptr;
	AlignTask *find_alignment = nullptr;

	AlignRow(Align *align, QWidget *parent = nullptr);
	virtual ~AlignRow();
	void findAlignment(bool update = true);
	void stopFinding();
	void updateRegion();
	
signals:
	void removeme(AlignRow *row);
	void updated(); //emit when status changes

public slots:
	void remove();
	void verify();
	void updateStatus(QString msg, int percent);
};

#endif // SPHEREROW_H
