#ifndef NETWORKING_H
#define NETWORKING_H

#include <QObject>
#include <QUdpSocket>
#include <QTextEdit>
#include <QSet>
#include <QHostAddress>
#include <QTimer>
#include "vectorclock.h"

class Networking : public QObject {
    Q_OBJECT

public:
    explicit Networking(QObject *parent = nullptr);
    void sendDatagram(const QByteArray &datagram, int sequenceNumber);
    void processIncomingDatagrams(QTextEdit *chatLog);
    void broadcastDiscovery();
    void runGossip();
    int getNextSequenceNumber();
    void setNoForwardMode(bool mode);
    QSet<QHostAddress> getPeers() const;
    void runAntiEntropy();
    void addPeer(const QHostAddress &peer);
    void forwardMessage(const QByteArray &datagram, const QHostAddress &sender);
    void sendPrivateMessage(const QString &dest, const QString &message);
    void sendRouteRumor();
    void updateRoutingTable(const QString &origin, const QHostAddress &sender, quint16 senderPort, const QVariantMap &message);
    constexpr static quint16 DEFAULT_PEER_PORT = 45454;

signals:
    void fileRequestReceived(const QVariantMap &msg);
    void blockReplyReceived(const QVariantMap &msg);
    void searchReplyReceived(const QVariantMap &msg);




private slots:
    void handleIncomingDatagrams();

private:
    QUdpSocket *udpSocket;
    QSet<QHostAddress> peers;
    VectorClock vectorClock;
    int sequenceNumber = 1;
    QMap<int, QByteArray> messageBuffer;
    QMap<QString, QPair<QHostAddress, quint16>> routingTable;  //DSDV Routing Table
    bool noforwardMode = false;
};

#endif
