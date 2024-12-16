#ifndef BRDFFRAME_H
#define BRDFFRAME_H

#include <QFrame>

class BrdfFrame: public QFrame {
	Q_OBJECT
	public:
		BrdfFrame(QWidget *parent = nullptr);
		void clear();
		void init();

	public slots:


	private:
	};

#endif // BRDFFRAME_H
