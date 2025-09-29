#ifndef RtiPlanRow_H
#define RtiPlanRow_H

#include <QFrame>
#include "../src/rti.h"

class HelpLabel;
class QHBoxLayout;
class QVBoxLayout;

class PlanRow: public QFrame {
	Q_OBJECT
public:
	PlanRow(QFrame *parent = nullptr);

	HelpLabel *label = nullptr;
	QFrame *planFrame = nullptr;
	QVBoxLayout *planLayout = nullptr;
	QHBoxLayout *buttons = nullptr;
};



#endif // RtiPlanRow_H
