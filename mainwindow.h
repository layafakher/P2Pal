#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QUdpSocket>
#include "networking.h"
#include <QFileSystemWatcher>
#include <QCryptographicHash>
#include <QFileSystemWatcher>
#include <QList>
#include <QDateTime>
#include <QCryptographicHash>
#include <QFileSystemWatcher>
#include <QQueue>
#include <QDateTime>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QProgressBar>



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void setNoForwardMode(bool mode);
    ~MainWindow();
    void setupFileWatcher(const QString &directory);


public slots:
    void on_searchButton_clicked();
    void handleSearchRequest(const QVariantMap &msg);
    void handleFileRequest(const QVariantMap &msg);
    void handleBlockReply(const QVariantMap &msg);
    void handleSearchReply(const QVariantMap &msg);





private slots:
    void sendMessage();
    void sendPrivateMessage();
    void processPendingDatagrams();
    void discoverPeers();
    void runGossipProtocol();
    void updatePeerList();
    void addPeer();
    void sendTo(const QPair<QHostAddress, quint16> &target, const QVariantMap &msg);




private:
    QString localIdentifier;
    Ui::MainWindow *ui;
    QTextEdit *chatLog;
    QLineEdit *inputField;
    QPushButton *addPeerButton;
    QListWidget *peerList;
    QUdpSocket *udpSocket;
    Networking *network;
    struct FileInfo {
        QString filename;
        qint64 size;
        QDateTime modified;
        QByteArray fileHash;
    };

    QList<FileInfo> fileIndex;
    QFileSystemWatcher *fileWatcher;
    QString sharedDirectory;
    QString localNodeID;
    QMap<QString, QPair<QHostAddress, quint16>> routingTable;
    void sendToNeighbors(const QVariantMap &msg);
    void updateFileIndex(const QString &directory);
    QMap<QString, QSet<int>> receivedBlocks;         //fileHash: set of received block IDs
    QMap<QString, int> totalBlocks;                  //fileHash: total block count
    QMap<QString, QTimer*> retryTimers;              //fileHash: retry timer
    QMap<QString, int> retryCount;                   //fileHash: retry attempts
    QSet<QString> activeTransfers;                   //currently running transfers
    QQueue<QString> pendingTransfers;                //queue for waiting files
    QMap<QString, QString> fileOwner;                //fileHash:; owner ID
    QString downloadDir = "./downloads";
    const int BLOCK_SIZE = 32 * 1024;
    const int MAX_TRANSFERS = 3;

    void startTransfer(const QString &fileHash, const QString &ownerID);
    void sendFileBlocks(const QString &fileHash, const QHostAddress &ip, quint16 port, const QString &requestor);
    void finalizeDownload(const QString &fileHash);
    void updateProgressBar(const QString &fileHash, int percent);
    void notifyTransferFailed(const QString &fileHash);
    void requestFileDownload(const QString &fileHash, const QString &ownerID);
    QMap<QString, QProgressBar*> progressBars;  //fileHash: progress bar


};

#endif
