#include "vectorclock.h"

void VectorClock::updateClock(const QString &origin, int sequenceNumber) {
    if (!clock.contains(origin) || clock[origin] < sequenceNumber) {
        clock[origin] = sequenceNumber;
    }
}

bool VectorClock::isNewMessage(const QString &origin, int sequenceNumber) {
    return !clock.contains(origin) || clock[origin] < sequenceNumber;
}
QMap<QString, int> VectorClock::getClock() const {
    return clock;
}
