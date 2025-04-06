#include "networking.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QHostInfo>


Networking::Networking(QObject *parent) : QObject(parent) {
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::Any, 0);

    connect(udpSocket, &QUdpSocket::readyRead, this, &Networking::handleIncomingDatagrams);

    //send initial route rumor
    QTimer::singleShot(5000, this, &Networking::sendRouteRumor);
}

void Networking::handleIncomingDatagrams() {
    processIncomingDatagrams(nullptr);
}
QSet<QHostAddress> Networking::getPeers() const {
    return peers;
}
void Networking::sendDatagram(const QByteArray &datagram, int sequenceNumber) {
    messageBuffer[sequenceNumber] = datagram;
    for (const auto &peer : peers) {
        udpSocket->writeDatagram(datagram, peer, 45454);
    }
}
void Networking::broadcastDiscovery() {
    qDebug() << "Broadcasting peer discovery...";

    QVariantMap discoveryMap;
    discoveryMap["Type"] = "DISCOVERY";

    QByteArray discoveryMessage = QJsonDocument(QJsonObject::fromVariantMap(discoveryMap)).toJson();

    udpSocket->writeDatagram(discoveryMessage, QHostAddress::Broadcast, 45454);
}
void Networking::runGossip() {
    qDebug() << "running Gossip Protocol...";

    for (const auto &peer : peers) {
        for (auto it = messageBuffer.begin(); it != messageBuffer.end(); ++it) {
            QByteArray gossipMessage = it.value();
            udpSocket->writeDatagram(gossipMessage, peer, 45454);
            qDebug() << "📡Gossip message sent to " << peer.toString();
        }
    }
}

void Networking::forwardMessage(const QByteArray &datagram, const QHostAddress &sender) {
    QJsonDocument doc = QJsonDocument::fromJson(datagram);
    QVariantMap messageMap = doc.object().toVariantMap();

    if (messageMap.contains("HopLimit")) {
        int hopLimit = messageMap["HopLimit"].toInt();
        if (hopLimit > 0) {
            messageMap["HopLimit"] = hopLimit - 1;
            QByteArray newDatagram = QJsonDocument(QJsonObject::fromVariantMap(messageMap)).toJson();
            for (const auto &peer : peers) {
                if (peer != sender) {
                    udpSocket->writeDatagram(newDatagram, peer, 45454);
                }
            }
        } else {
            qDebug() << "message discarded: Hop limit reached.";
        }
    }
}

void Networking::setNoForwardMode(bool mode) {
    noforwardMode = mode;
    qDebug() << "No-Forward Mode set to:" << mode;
}



void Networking::processIncomingDatagrams(QTextEdit *chatLog) {
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress sender;
        quint16 senderPort;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        qDebug() << "Received datagram from:" << sender.toString()
                 << "| Port: " << senderPort
                 << "| Data: " << datagram;

        QJsonDocument doc = QJsonDocument::fromJson(datagram);
        if (doc.isNull()) {
            qDebug() << "❌ Error parsing JSON!";
            continue;
        }


        QVariantMap messageMap = doc.object().toVariantMap();
        QString type = messageMap["Type"].toString();
        qDebug() << "Message Type: " << type;

        if (type == "CHAT") {
            QString origin = messageMap["Origin"].toString();
            int seqNum = messageMap["SequenceNumber"].toInt();
            QString chatText = messageMap["ChatText"].toString();

            qDebug() << " Chat Message Received: " << chatText
                     << "| Origin: " << origin
                     << "| SeqNum: " << seqNum;

            if (vectorClock.isNewMessage(origin, seqNum)) {
                vectorClock.updateClock(origin, seqNum);
                if (chatLog) chatLog->append(origin + ": " + chatText);
                qDebug() << "message displayed in chat: " << chatText;
            } else {
                qDebug() << "duplicate message ignored.";
            }

            updateRoutingTable(origin, sender, senderPort, messageMap);
            forwardMessage(datagram, sender);

        } else if (type == "DISCOVERY") {
            if (!peers.contains(sender)) {
                peers.insert(sender);
                qDebug() << "🟢 New peer discovered: " << sender.toString();
            }


            QVariantMap response;
            response["Type"] = "DISCOVERY_RESPONSE";
            QByteArray responseData = QJsonDocument(QJsonObject::fromVariantMap(response)).toJson();
            udpSocket->writeDatagram(responseData, sender, senderPort);
            qDebug() << "Sent DISCOVERY_RESPONSE to " << sender.toString();

        } else if (type == "DISCOVERY_RESPONSE") {
            if (!peers.contains(sender)) {
                peers.insert(sender);
                qDebug() << "added new peer from response: " << sender.toString();
            }

        } else if (type == "PRIVATE_MESSAGE") {
            QString dest = messageMap["Dest"].toString();
            QString privateMessage = messageMap["ChatText"].toString();
            int hopLimit = messageMap["HopLimit"].toInt();

            if (dest == QHostInfo::localHostName()) {
                if (chatLog) chatLog->append("🔒 Private: " + privateMessage);
                qDebug() << "received private message: " << privateMessage;
            } else if (hopLimit > 0) {
                messageMap["HopLimit"] = hopLimit - 1;
                sendDatagram(QJsonDocument(QJsonObject::fromVariantMap(messageMap)).toJson(), sequenceNumber++);
                qDebug() << "forwarding private message to " << dest << " with hop limit: " << hopLimit;
            }

        } else if (type == "ROUTE_RUMOR") {
            QString origin = messageMap["Origin"].toString();
            updateRoutingTable(origin, sender, senderPort, messageMap);
            qDebug() << "updated route for: " << origin << " via " << sender.toString();
        }
        type = messageMap["Type"].toString();


        if (type == "FILE_REQUEST") {
            emit fileRequestReceived(messageMap);
        } else if (type == "BLOCK_REPLY") {
            emit blockReplyReceived(messageMap);
        }
        if (type == "SEARCH_RESPONSE") {
            emit searchReplyReceived(messageMap);
        }


    }
}



void Networking::sendRouteRumor() {
    QVariantMap msg;
    msg["Type"] = "ROUTE_RUMOR";
    msg["Origin"] = QHostInfo::localHostName();
    msg["SeqNo"] = vectorClock.getClock()[msg["Origin"].toString()] + 1;
    msg["LastIP"] = udpSocket->localAddress().toString();
    msg["LastPort"] = udpSocket->localPort();

    QByteArray datagram = QJsonDocument(QJsonObject::fromVariantMap(msg)).toJson();
    for (const auto &peer : peers) {
        udpSocket->writeDatagram(datagram, peer, 45454);
    }
    QTimer::singleShot(60000, this, &Networking::sendRouteRumor);
}

void Networking::addPeer(const QHostAddress &peer) {
    if (!peers.contains(peer)) {
        peers.insert(peer);
        qDebug() << "added new peer manually: " << peer.toString();
    }
}


void Networking::updateRoutingTable(const QString &origin, const QHostAddress &sender, quint16 senderPort, const QVariantMap &message) {
    if (!routingTable.contains(origin) || vectorClock.getClock()[origin] < message["SeqNo"].toInt()) {
        routingTable[origin] = {sender, senderPort};

        if (message.contains("LastIP") && message.contains("LastPort")) {
            QHostAddress publicIP(message["LastIP"].toString());
            quint16 publicPort = message["LastPort"].toUInt();
            routingTable[origin] = {publicIP, publicPort};  //storinng public NAT address
        }

        qDebug() << "updated route for:" << origin
                 << "local IP:" << sender.toString()
                 << "public IP:" << routingTable[origin].first.toString()
                 << "port:" << routingTable[origin].second;
    }
}


int Networking::getNextSequenceNumber() {
    return sequenceNumber++;
}


void Networking::sendPrivateMessage(const QString &dest, const QString &message) {
    if (!routingTable.contains(dest)) {
        qDebug() << "No route to destination!";
        return;
    }

    QVariantMap msg;
    msg["Type"] = "PRIVATE_MESSAGE";
    msg["Origin"] = QHostInfo::localHostName();
    msg["Dest"] = dest;
    msg["ChatText"] = message;
    msg["HopLimit"] = 10;

    QHostAddress targetIP = routingTable[dest].first;
    quint16 targetPort = routingTable[dest].second;

    QByteArray datagram = QJsonDocument(QJsonObject::fromVariantMap(msg)).toJson();
    udpSocket->writeDatagram(datagram, targetIP, targetPort);
}

