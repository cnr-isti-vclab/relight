#ifndef CALFLATFIELDFRAME_H
#define CALFLATFIELDFRAME_H

#include <QFrame>

class QListWidget;
class QLabel;
class QComboBox;

class CalFlatfieldFrame: public QFrame {
	Q_OBJECT
public:
	explicit CalFlatfieldFrame(QWidget *parent = nullptr);

public slots:
	void loadImages();
	void clearImages();
	void fitOrderChanged(int index);

private:
	QListWidget *ff_list   = nullptr;
	QLabel      *ff_status = nullptr;
	QComboBox   *fit_order = nullptr;
};

#endif // CALFLATFIELDFRAME_H
