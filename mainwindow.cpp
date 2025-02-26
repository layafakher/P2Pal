#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QInputDialog>
#include <QHostInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), sequenceNumber(1), network(new Networking(this)) {
    discoveryTimer = new QTimer(this);
    gossipTimer = new QTimer(this);
    antiEntropyTimer = new QTimer(this);

    ui->setupUi(this);
    localIdentifier = QHostInfo::localHostName();

    auto *layout = new QVBoxLayout(ui->centralwidget);

    chatLog = new QTextEdit(this);
    chatLog->setReadOnly(true);
    layout->addWidget(chatLog);

    inputField = new QLineEdit(this);
    layout->addWidget(inputField);

    addPeerButton = new QPushButton("Add Peer", this);
    layout->addWidget(addPeerButton);
    connect(addPeerButton, &QPushButton::clicked, this, &MainWindow::addPeer);

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::Any, 0);

    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::processPendingDatagrams);
    connect(inputField, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);

    discoveryTimer = new QTimer(this);
    connect(discoveryTimer, &QTimer::timeout, this, &MainWindow::discoverPeers);
    discoveryTimer->start(5000);

    gossipTimer = new QTimer(this);
    connect(gossipTimer, &QTimer::timeout, this, &MainWindow::runGossipProtocol);
    gossipTimer->start(2000);

    antiEntropyTimer = new QTimer(this);
    connect(antiEntropyTimer, &QTimer::timeout, this, &MainWindow::runAntiEntropy);
    antiEntropyTimer->start(5000);
}
void MainWindow::runAntiEntropy() {
    qDebug() << "Running Anti-Entropy from MainWindow...";
    network->runAntiEntropy();
}

MainWindow::~MainWindow() {
    delete ui;
    delete network;
}
void MainWindow::sendMessage() {
    QString message = inputField->text().trimmed();
    if (message.isEmpty()) return;

    QVariantMap messageMap;
    messageMap["Type"] = "CHAT";
    messageMap["ChatText"] = message;
    messageMap["Origin"] = localIdentifier;
    messageMap["SequenceNumber"] = network->getNextSequenceNumber();

    QByteArray datagram = QJsonDocument(QJsonObject::fromVariantMap(messageMap)).toJson();
    network->sendDatagram(datagram, messageMap["SequenceNumber"].toInt());

    chatLog->append("Me: " + message);
    inputField->clear();
}




void MainWindow::discoverPeers() {
    network->broadcastDiscovery();
}

void MainWindow::processPendingDatagrams() {
    network->processIncomingDatagrams(chatLog);
}

void MainWindow::runGossipProtocol() {
    network->runGossip();
}

#include <QThread>

#include <QEventLoop>
#include <QTimer>

void MainWindow::addPeer() {
    qDebug() << "Running peer discovery before manual entry...";

    network->broadcastDiscovery();


    QEventLoop loop;
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    loop.exec();

    if (!peers.isEmpty()) {
        chatLog->append("Auto-discovered peers: " + QString::number(peers.size()));
        qDebug() << "Auto-discovered peers, skipping manual entry.";
        return;
    }


    bool ok;
    QString peerAddress = QInputDialog::getText(this, "Add Peer",
                                                "No peers found. Enter peer IP address:",
                                                QLineEdit::Normal, "", &ok);
    if (ok && !peerAddress.isEmpty()) {
        peers.insert(QHostAddress(peerAddress));
        chatLog->append("Manually added peer: " + peerAddress);
    }
}

