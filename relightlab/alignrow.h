#ifndef ALIGNROW_H
#define ALIGNROW_H

#include "task.h"
#include <QWidget>




class Align;
class QLabel;
class QProgressBar;
class QPushButton;
class AlignOverview;
class QGraphicsPixmapItem;

class FindAlignment: public Task {
public:
	Align *align;
	bool update_positions;

	FindAlignment(Align *align, bool update = true);
	virtual void run() override;

};

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
	QPushButton *edit_button = nullptr;
	QPushButton *verify_button = nullptr;
	FindAlignment *find_alignment = nullptr;

	AlignRow(Align *align, QWidget *parent = nullptr);
	void findAlignment(bool update = true);
	void stopFinding();
	void updateRegion();
	
signals:
	void removeme(AlignRow *row);
	void updated(); //emit when status changes

public slots:
	void edit();
	void remove();
	void verify();
	void updateStatus(QString msg, int percent);
};

#endif // SPHEREROW_H
