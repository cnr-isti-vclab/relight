#ifndef ALIGNROW_H
#define ALIGNROW_H

#include "task.h"
#include <QWidget>




class Align;
class QLabel;
class QProgressBar;
class ReflectionView;
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
	Align *align;
	int rowHeight = 92;
	QLabel *thumb;
	ReflectionView *reflections;
	QLabel *status = nullptr;
	QProgressBar *progress = nullptr;
	FindAlignment *find_alignment = nullptr;

	AlignRow(Align *align, QWidget *parent = nullptr);
	void findAlignment(bool update = true);
	void stopFinding();
	
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
