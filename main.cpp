#include "mainwindow.h"
#include <QApplication>
#include <QProcess>
#include <QStringList>
#include <QDebug>
#include "mainwindow.h"
#include <QApplication>
#include <QStringList>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;

    QStringList args = app.arguments();
    if (argc > 1 && QString(argv[1]) == "-noforward") {
        window.setNoForwardMode(true);
        qDebug() << "running in Rendezvous Server Mode (-noforward)";
    }
    if (args.contains("-sharedir")) {
        int idx = args.indexOf("-sharedir");
        QString path = args.value(idx + 1);
        window.setupFileWatcher(path);
    }

    window.show();
    return app.exec();
}




