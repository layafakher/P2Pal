#include "networking.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

Networking::Networking(QObject *parent) : QObject(parent) {
    udpSocket = new QUdpSocket(this);

    bool bound = udpSocket->bind(QHostAddress::Any, 0);

    if (!bound) {
        qDebug() << "❌ Failed to bind UDP socket:" << udpSocket->errorString();
    } else {
        qDebug() << "✅ UDP socket bound to port:" << udpSocket->localPort();
    }

    connect(udpSocket, &QUdpSocket::readyRead, this, &Networking::handleIncomingDatagrams);
}

int Networking::getNextSequenceNumber() {
    return sequenceNumber++;
}

void Networking::sendDatagram(const QByteArray &datagram, int sequenceNumber) {
    messageBuffer[sequenceNumber] = datagram;
    for (const auto &peer : peers) {
        udpSocket->writeDatagram(datagram, peer, 45454);
    }
}


void Networking::handleIncomingDatagrams() {
    processIncomingDatagrams(nullptr);
}
void Networking::processIncomingDatagrams(QTextEdit *chatLog) {
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        qDebug() << "📩 Received datagram from:" << sender.toString()
                 << "| Port: " << senderPort
                 << "| Raw Data:" << datagram;

        QJsonDocument doc = QJsonDocument::fromJson(datagram);
        if (doc.isNull()) {
            qDebug() << "❌ Error parsing JSON!";
            continue;
        }

        QVariantMap messageMap = doc.object().toVariantMap();
        QString type = messageMap["Type"].toString();

        qDebug() << "📨 Message Type: " << type;


        if (type == "DISCOVERY") {
            qDebug() << "🔍 Peer discovery request received from: " << sender.toString();

            if (!peers.contains(sender)) {
                peers.insert(sender);
                qDebug() << "✅ Added peer: " << sender.toString();
            }


            QVariantMap responseMap;
            responseMap["Type"] = "DISCOVERY_RESPONSE";
            QByteArray responseMessage = QJsonDocument(QJsonObject::fromVariantMap(responseMap)).toJson();
            udpSocket->writeDatagram(responseMessage, sender, senderPort);
            qDebug() << "📤 Sent DISCOVERY_RESPONSE to " << sender.toString();
        }
        else if (type == "DISCOVERY_RESPONSE") {
            qDebug() << "📥 Discovery response received from: " << sender.toString();

            if (!peers.contains(sender)) {
                peers.insert(sender);
                qDebug() << "✅ Added new peer from response: " << sender.toString();
            }
        }


        else if (type == "CHAT") {
            QString origin = messageMap["Origin"].toString();
            int seqNum = messageMap["SequenceNumber"].toInt();

            if (!peerMessages[origin].contains(seqNum)) {
                peerMessages[origin].insert(seqNum);
                chatLog->append(origin + ": " + messageMap["ChatText"].toString());
            }

            sendAcknowledgment(sender, seqNum);
        }


        else if (type == "ACK") {
            int ackNum = messageMap["SequenceNumber"].toInt();
            removeAcknowledgedMessage(ackNum);
        }
    }
}
void Networking::runAntiEntropy() {
    qDebug() << "🔍 Running Anti-Entropy Check...";

    QMap<QString, int> currentClock = vectorClock.getClock();

    for (const auto &peer : peers) {
        for (auto it = currentClock.begin(); it != currentClock.end(); ++it) {
            QString origin = it.key();
            int lastSeenSeq = it.value();

            QVariantMap requestMap;
            requestMap["Type"] = "SYNC_REQUEST";
            requestMap["Origin"] = origin;
            requestMap["LastSeen"] = lastSeenSeq;

            QByteArray syncMessage = QJsonDocument(QJsonObject::fromVariantMap(requestMap)).toJson();
            udpSocket->writeDatagram(syncMessage, peer, 45454);

            qDebug() << "📤 Sent SYNC_REQUEST to " << peer.toString()
                     << " for Origin: " << origin
                     << " LastSeenSeq: " << lastSeenSeq;
        }
    }
}





void Networking::broadcastDiscovery() {
    qDebug() << "🔍 Broadcasting peer discovery...";

    QVariantMap discoveryMap;
    discoveryMap["Type"] = "DISCOVERY";

    QByteArray discoveryMessage = QJsonDocument(QJsonObject::fromVariantMap(discoveryMap)).toJson();

    // Send broadcast
    qint64 bytesSent = udpSocket->writeDatagram(discoveryMessage, QHostAddress::Broadcast, 45454);

    if (bytesSent == -1) {
        qDebug() << "❌ Error broadcasting discovery: " << udpSocket->errorString();
    } else {
        qDebug() << "✅ Discovery broadcast sent, bytes: " << bytesSent;
    }
}



void Networking::runGossip() {
    qDebug() << "🔄 Running Gossip Protocol...";

    for (const auto &peer : peers) {
        for (auto it = messageBuffer.begin(); it != messageBuffer.end(); ++it) {
            QByteArray gossipMessage = it.value();

            qint64 bytesSent = udpSocket->writeDatagram(gossipMessage, peer, 45454);

            if (bytesSent == -1) {
                qDebug() << "❌ Gossip message send error:" << udpSocket->errorString();
            } else {
                qDebug() << "✅ Gossip message resent to " << peer.toString() << ", bytes: " << bytesSent;
            }
        }
    }
}



void Networking::sendAcknowledgment(const QHostAddress &receiver, int sequenceNumber) {
    QVariantMap ackMap;
    ackMap["Type"] = "ACK";
    ackMap["SequenceNumber"] = sequenceNumber;

    QByteArray ackDatagram = QJsonDocument(QJsonObject::fromVariantMap(ackMap)).toJson();
    udpSocket->writeDatagram(ackDatagram, receiver, 45454);
}

void Networking::removeAcknowledgedMessage(int sequenceNumber) {

}
