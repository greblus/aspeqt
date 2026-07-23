#include "phonebook.h"
#include "miscutils.h"

#include <QDebug>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

// ContentFile rather than QFile so a phonebook picked through the Android
// document picker (a content:// URI) opens in place. QXmlStream is used
// instead of QDomDocument to avoid pulling in the QtXml module.

bool PhoneBook::load(const QString &path)
{
    m_entries.clear();
    if (path.isEmpty())
        return false;

    ContentFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "!e" << QObject::tr("Cannot open phonebook %1: %2")
                              .arg(path, file.errorString());
        return false;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement)
            continue;
        // Any depth: see the note in phonebook.h.
        if (xml.name() != QLatin1String("BBS"))
            continue;

        const QXmlStreamAttributes a = xml.attributes();
        BbsEntry e;
        e.name       = a.value("name").toString();
        e.ip         = a.value("ip").toString();
        e.port       = a.hasAttribute("port") ? a.value("port").toInt() : 23;
        e.protocol   = a.hasAttribute("protocol") ? a.value("protocol").toString()
                                                  : QStringLiteral("TELNET");
        e.login      = a.value("login").toString();
        e.password   = a.value("password").toString();
        e.privateKey = a.value("keyfile").toString();
        e.favourite  = a.value("favourite").toString() == QLatin1String("true");
        m_entries.append(e);
    }

    if (xml.hasError()) {
        qCritical() << "!e" << QObject::tr("Malformed phonebook %1: %2")
                              .arg(path, xml.errorString());
        m_entries.clear();
        return false;
    }
    return true;
}

bool PhoneBook::save(const QString &path) const
{
    if (path.isEmpty())
        return false;

    ContentFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCritical() << "!e" << QObject::tr("Cannot write phonebook %1: %2")
                              .arg(path, file.errorString());
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(4);
    xml.writeStartDocument();
    xml.writeStartElement("Phonebook");
    for (const BbsEntry &e : m_entries) {
        xml.writeStartElement("BBS");
        xml.writeAttribute("name", e.name);
        xml.writeAttribute("ip", e.ip);
        xml.writeAttribute("port", QString::number(e.port));
        xml.writeAttribute("protocol", e.protocol);
        xml.writeAttribute("login", e.login);
        xml.writeAttribute("password", e.password);
        xml.writeAttribute("keyfile", e.privateKey);
        if (e.favourite)
            xml.writeAttribute("favourite", "true");
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndDocument();

    file.close();
    return file.error() == QFileDevice::NoError;
}

BbsEntry PhoneBook::findByName(const QString &name) const
{
    // Exact match wins, so two entries differing only in case (e.g. a Telnet
    // "Mylocal" and an SSH "mylocal") each dial their own protocol.
    for (const BbsEntry &e : m_entries)
        if (e.name == name)
            return e;
    for (const BbsEntry &e : m_entries)
        if (e.name.compare(name, Qt::CaseInsensitive) == 0)
            return e;
    return BbsEntry();
}
