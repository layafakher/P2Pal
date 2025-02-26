#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QUdpSocket>
#include <QTimer>
#include <QSet>
#include "networking.h"
#include "vectorclock.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendMessage();
    void processPendingDatagrams();
    void discoverPeers();
    void runGossipProtocol();
    void addPeer();
    void runAntiEntropy();


private:
    QTimer *discoveryTimer;
    QTimer *gossipTimer;
    QTimer *antiEntropyTimer;

    Ui::MainWindow *ui;
    QTextEdit *chatLog;
    QLineEdit *inputField;
    QPushButton *addPeerButton;
    QUdpSocket *udpSocket;
    QSet<QHostAddress> peers;
    int sequenceNumber;
    QString localIdentifier;
    Networking *network;
    VectorClock vectorClock;
};

#endif
