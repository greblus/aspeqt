#ifndef BBSDATA_H
#define BBSDATA_H

#include <QString>

struct BbsEntry {
    QString name;
    QString ip;
    int port = 23;
    QString protocol = QStringLiteral("TELNET");
    QString login;
    QString password;
    QString privateKey;
    QString font;
    QString keyMap;
    // Our own extension to the AspeQt-2k26 file format. That program ignores
    // attributes it does not know, so phonebooks stay interchangeable.
    bool favourite = false;


    // Helper to get "ip:port"
    QString address() const { return ip + ":" + QString::number(port); }
};

#endif // BBSDATA_H
