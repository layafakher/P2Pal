#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QInputDialog>
#include <qjsonobject.h>
#include <QHostInfo>
#include <QRandomGenerator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QThread>
#include <QMessageBox>





MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), network(new Networking(this)) {
    ui->setupUi(this);
    localIdentifier = QHostInfo::localHostName();  //getting system hostname
    localNodeID = localIdentifier;
    connect(network, &Networking::fileRequestReceived, this, &MainWindow::handleFileRequest);
    connect(network, &Networking::blockReplyReceived, this, &MainWindow::handleBlockReply);
    connect(network, &Networking::searchReplyReceived, this, &MainWindow::handleSearchReply);

    chatLog = new QTextEdit(this);
    chatLog->setReadOnly(true);

    inputField = new QLineEdit(this);
    connect(inputField, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);

    peerList = new QListWidget(this);
    connect(peerList, &QListWidget::itemDoubleClicked, this, &MainWindow::sendPrivateMessage);

    auto *layout = new QVBoxLayout(ui->centralwidget);
    layout->addWidget(chatLog);
    layout->addWidget(peerList);
    layout->addWidget(inputField);
    addPeerButton = new QPushButton("Add Peer", this);
    layout->addWidget(addPeerButton);
    connect(addPeerButton, &QPushButton::clicked, this, &MainWindow::addPeer);

    ui->searchResultsTable->setColumnCount(5);
    QStringList headers = { "Filename", "Size (KB)", "Source Node", "Download", "Progress" };
    ui->searchResultsTable->setHorizontalHeaderLabels(headers);


}
void MainWindow::setNoForwardMode(bool mode) {
    network->setNoForwardMode(mode);
}

void MainWindow::addPeer() {
    bool ok;
    QString peerAddress = QInputDialog::getText(this, "Add Peer",
                                                "Enter peer IP address:",
                                                QLineEdit::Normal, "", &ok);
    if (ok && !peerAddress.isEmpty()) {
        QHostAddress peer(peerAddress);
        network->addPeer(peer);
        chatLog->append("manually added peer: " + peerAddress);
    }
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

    qDebug() << "Sending message to peers: " << messageMap;

    network->sendDatagram(datagram, messageMap["SequenceNumber"].toInt());

    chatLog->append("Me: " + message);
    inputField->clear();
}


void MainWindow::processPendingDatagrams() {
    network->processIncomingDatagrams(chatLog);
}

void MainWindow::discoverPeers() {
    network->broadcastDiscovery();
}

void MainWindow::runGossipProtocol() {
    network->runGossip();
}

void MainWindow::updatePeerList() {
    peerList->clear();
    for (const auto &peer : network->getPeers()) {
        peerList->addItem(peer.toString());
    }
}


void MainWindow::sendPrivateMessage() {
    QString dest = peerList->currentItem()->text();
    if (dest.isEmpty()) return;

    QDialog *privateChatDialog = new QDialog(this);
    privateChatDialog->setWindowTitle("Chat with " + dest);
    privateChatDialog->resize(400, 300);

    QTextEdit *chatLog = new QTextEdit(privateChatDialog);
    chatLog->setReadOnly(true);
    QLineEdit *messageInput = new QLineEdit(privateChatDialog);
    QPushButton *sendButton = new QPushButton("Send", privateChatDialog);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(chatLog);
    layout->addWidget(messageInput);
    layout->addWidget(sendButton);
    privateChatDialog->setLayout(layout);

    connect(sendButton, &QPushButton::clicked, [=]() {
        network->sendPrivateMessage(dest, messageInput->text());
        chatLog->append("Me: " + messageInput->text());
        messageInput->clear();
    });

    privateChatDialog->exec();
}


void MainWindow::setupFileWatcher(const QString &directory) {
    sharedDirectory = directory;
    fileWatcher = new QFileSystemWatcher(this);
    fileWatcher->addPath(directory);
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::updateFileIndex);
    updateFileIndex(directory);
}

void MainWindow::updateFileIndex(const QString &directory) {
    fileIndex.clear();
    QDir dir(directory);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files);
    for (const QFileInfo &info : fileList) {
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) continue;
        QByteArray hash = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
        file.close();
        fileIndex.append({info.fileName(), info.size(), info.lastModified(), hash});
    }
}
void MainWindow::on_searchButton_clicked() {
    QString query = ui->searchLineEdit->text().trimmed();
    if (query.isEmpty()) return;

    QVariantMap msg;
    msg["Origin"] = localNodeID;
    msg["Search"] = query;
    msg["Budget"] = 20;
    msg["HopLimit"] = 10;
    sendToNeighbors(msg);
}
void MainWindow::handleSearchRequest(const QVariantMap &msg) {
    QStringList keywords = msg["Search"].toString().split(" ");
    QStringList matchNames, matchIDs;
    QList<qint64> matchSizes;

    for (const FileInfo &f : fileIndex) {
        bool match = std::all_of(keywords.begin(), keywords.end(), [&](const QString &kw) {
            return f.filename.contains(kw, Qt::CaseInsensitive);
        });
        if (match) {
            matchNames << f.filename;
            matchSizes << f.size;
            matchIDs << QString(f.fileHash.toHex());
        }
    }

    if (!matchNames.isEmpty()) {
        QVariantMap reply;
        reply["Type"] = "SEARCH_RESPONSE";
        reply["Origin"] = localNodeID;
        reply["Dest"] = msg["Origin"].toString();
        reply["MatchNames"] = matchNames;
        reply["MatchSizes"] = QVariant::fromValue(matchSizes);
        reply["MatchIDs"] = matchIDs;
        sendTo(routingTable[reply["Dest"].toString()], reply);
    }
}

void MainWindow::sendTo(const QPair<QHostAddress, quint16> &target, const QVariantMap &msg) {
    QByteArray datagram = QJsonDocument(QJsonObject::fromVariantMap(msg)).toJson();
    udpSocket->writeDatagram(datagram, target.first, target.second);
}

void MainWindow::sendToNeighbors(const QVariantMap &msg) {
    QByteArray datagram = QJsonDocument(QJsonObject::fromVariantMap(msg)).toJson();

    for (const QHostAddress &peer : network->getPeers()) {
        udpSocket->writeDatagram(datagram, peer, Networking::DEFAULT_PEER_PORT);

    }
}

void MainWindow::requestFileDownload(const QString &fileHash, const QString &ownerID) {
    if (activeTransfers.size() < MAX_TRANSFERS) {
        activeTransfers.insert(fileHash);

        QVariantMap req;
        req["Type"] = "FILE_REQUEST";
        req["Origin"] = localNodeID;
        req["Dest"] = ownerID;
        req["Request"] = fileHash;

        sendTo(routingTable[ownerID], req);
        fileOwner[fileHash] = ownerID;
    } else {
        pendingTransfers.enqueue(fileHash);
        fileOwner[fileHash] = ownerID;
    }
}

void MainWindow::handleFileRequest(const QVariantMap &msg) {
    QString fileHash = msg["Request"].toString();
    QString requestor = msg["Origin"].toString();

    if (!routingTable.contains(requestor)) return;

    QHostAddress ip = routingTable[requestor].first;
    quint16 port = routingTable[requestor].second;

    sendFileBlocks(fileHash, ip, port, requestor);

}
void MainWindow::sendFileBlocks(const QString &fileHash, const QHostAddress &ip, quint16 port, const QString &requestor)
 {
    QString filePath = sharedDirectory + "/" + fileHash;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    int total = (file.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (int blockID = 0; blockID < total; blockID++) {
        file.seek(blockID * BLOCK_SIZE);
        QByteArray data = file.read(BLOCK_SIZE);

        QVariantMap block;
        block["Type"] = "BLOCK_REPLY";
        block["Origin"] = localNodeID;
        block["Dest"] = requestor;
        block["BlockReply"] = fileHash;
        block["BlockID"] = blockID;
        block["TotalBlocks"] = total;
        block["BlockData"] = data;

        sendTo({ip, port}, block);
        QThread::msleep(50);
    }
}

 void MainWindow::handleBlockReply(const QVariantMap &msg) {
     QString hash = msg["BlockReply"].toString();
     int id = msg["BlockID"].toInt();
     int total = msg["TotalBlocks"].toInt();
     QByteArray data = msg["BlockData"].toByteArray();

     totalBlocks[hash] = total;
     receivedBlocks[hash].insert(id);

     QFile part(downloadDir + "/" + hash + ".part");
     if (part.open(QIODevice::ReadWrite)) {
         part.seek(id * BLOCK_SIZE);
         part.write(data);
         part.close();
     }

     updateProgressBar(hash, (receivedBlocks[hash].size() * 100) / total);

     if (receivedBlocks[hash].size() == total) finalizeDownload(hash);
 }

 void MainWindow::finalizeDownload(const QString &hash) {
     QFile file(downloadDir + "/" + hash + ".part");
     if (!file.open(QIODevice::ReadOnly)) return;

     QByteArray actual = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
     file.close();

     if (actual.toHex() == hash.toUtf8()) {
         file.rename(downloadDir + "/" + hash + ".done");
         updateProgressBar(hash, 100);
     } else {
         notifyTransferFailed(hash);
     }

     activeTransfers.remove(hash);
     if (!pendingTransfers.isEmpty()) {
         QString next = pendingTransfers.dequeue();
         startTransfer(next, fileOwner[next]);
         activeTransfers.insert(next);
     }
 }


 void MainWindow::startTransfer(const QString &fileHash, const QString &ownerID) {
     requestFileDownload(fileHash, ownerID);

     QTimer *timer = new QTimer(this);
     retryTimers[fileHash] = timer;
     retryCount[fileHash] = 0;

     connect(timer, &QTimer::timeout, this, [this, fileHash]() {
         if (++retryCount[fileHash] <= 3) {
             requestFileDownload(fileHash, fileOwner[fileHash]);
         } else {
             notifyTransferFailed(fileHash);
         }
     });

     timer->start(5000);
 }

 void MainWindow::handleSearchReply(const QVariantMap &msg) {
     QStringList names = msg["MatchNames"].toStringList();
     QList<qint64> sizes = msg["MatchSizes"].value<QList<qint64>>();
     QStringList hashes = msg["MatchIDs"].toStringList();
     QString ownerID = msg["Origin"].toString();

     for (int i = 0; i < names.size(); ++i) {
         QString fileName = names[i];
         qint64 sizeKB = sizes[i] / 1024;
         QString fileHash = hashes[i];

         int row = ui->searchResultsTable->rowCount();
         ui->searchResultsTable->insertRow(row);

         //Col 0: Filename
         QTableWidgetItem *fileItem = new QTableWidgetItem(fileName);
         fileItem->setData(Qt::UserRole, fileHash);
         ui->searchResultsTable->setItem(row, 0, fileItem);

         //cool1: Size
         ui->searchResultsTable->setItem(row, 1, new QTableWidgetItem(QString::number(sizeKB)));

         //col2: Owner
         ui->searchResultsTable->setItem(row, 2, new QTableWidgetItem(ownerID));

         //col3: Download Button
         QPushButton *downloadBtn = new QPushButton("Download");
         ui->searchResultsTable->setCellWidget(row, 3, downloadBtn);

         connect(downloadBtn, &QPushButton::clicked, this, [=]() {
             QString hash = ui->searchResultsTable->item(row, 0)->data(Qt::UserRole).toString();
             QString owner = ui->searchResultsTable->item(row, 2)->text();

             //col 4: Progress Bar
             QProgressBar *bar = new QProgressBar(this);
             bar->setRange(0, 100);
             bar->setValue(0);
             ui->searchResultsTable->setCellWidget(row, 4, bar);
             progressBars[hash] = bar;

             requestFileDownload(hash, owner);
         });
     }
 }

 void MainWindow::updateProgressBar(const QString &fileHash, int percent) {
     if (progressBars.contains(fileHash)) {
         progressBars[fileHash]->setValue(percent);
     }
 }

 void MainWindow::notifyTransferFailed(const QString &fileHash) {
     qDebug() << "❌ Transfer failed for file:" << fileHash;

     //it remove and delete progress bar if it exists
     if (progressBars.contains(fileHash)) {
         progressBars[fileHash]->deleteLater();
         progressBars.remove(fileHash);
     }

     QMessageBox::warning(this, "Transfer Failed",
                          "The download failed for file with hash:\n" + fileHash +
                              "\nPlease try again later.");
 }






