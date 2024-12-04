#ifndef RtiPlanRow_H
#define RtiPlanRow_H

#include <QFrame>
#include "../src/rti.h"

class HelpLabel;
class QHBoxLayout;

class PlanRow: public QFrame {
	Q_OBJECT
public:
	PlanRow(QFrame *parent = nullptr);

	HelpLabel *label = nullptr;
	QHBoxLayout *buttons = nullptr;
	QFrame *buttonsFrame = nullptr;
};



#endif // RtiPlanRow_H
