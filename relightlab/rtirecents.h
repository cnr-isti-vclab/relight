#ifndef RTIRECENTS_H
#define RTIRECENTS_H

#include "../relight/rtitask.h"

#include <QFrame>
#include <vector>

std::vector<RtiParameters> recentRtis();
void addRecentRti(const RtiParameters &params);

class RtiRecents: public QFrame {
public:
	RtiRecents(QFrame *parent = nullptr);
};

#endif // RTIRECENTS_H
