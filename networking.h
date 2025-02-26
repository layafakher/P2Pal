#ifndef NETWORKING_H
#define NETWORKING_H

#include <QObject>
#include <QUdpSocket>
#include <QTextEdit>
#include <QSet>
#include <QHostAddress>
#include "vectorclock.h"

class Networking : public QObject {
    Q_OBJECT

public:
    explicit Networking(QObject *parent = nullptr);
    void sendDatagram(const QByteArray &datagram, int sequenceNumber);

    void processIncomingDatagrams(QTextEdit *chatLog);
    void broadcastDiscovery();
    void runGossip();
    void sendAcknowledgment(const QHostAddress &receiver, int sequenceNumber);
    void removeAcknowledgedMessage(int sequenceNumber);
    int getNextSequenceNumber();
    void runAntiEntropy();


private slots:
    void handleIncomingDatagrams();

private:
    QMap<QString, QSet<int>> peerMessages;

    QUdpSocket *udpSocket;
    QSet<QHostAddress> peers;
    VectorClock vectorClock;
    int sequenceNumber = 1;
    QMap<int, QByteArray> messageBuffer;
};

#endif
