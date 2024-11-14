#ifndef RTIFRAME_H
#define RTIFRAME_H

#include <QFrame>
//TODO we should separate RTI definitions from actual implementation (materials etc).
#include "../src/rti.h"

class RtiCard;
class RtiRecents;
class RtiParameters;

class RtiFrame: public QFrame {
	Q_OBJECT

public:
	RtiFrame(QWidget *parent = nullptr);
	void init();
	void exportRti(RtiParameters &parameters);

signals:
	void processStarted();

private:
	RtiRecents *recents;
};

#endif // RTIFRAME_H
