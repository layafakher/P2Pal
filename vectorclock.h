#ifndef VECTORCLOCK_H
#define VECTORCLOCK_H

#include <QMap>
#include <QString>

class VectorClock {
public:
    QMap<QString, int> getClock() const;
    void updateClock(const QString &origin, int sequenceNumber);
    bool isNewMessage(const QString &origin, int sequenceNumber);

private:
    QMap<QString, int> clock;
};

#endif
