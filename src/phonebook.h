#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#include <QList>
#include <QString>
#include "bbsdata.h"

// The BBS phonebook: a list of dialable entries, loaded from and saved to XML.
//
// This replaces PhoneDirectory from AspeQt-2k26, which was a QDialog mixing the
// widget UI with the file handling. Only the file handling is kept here; the
// list is shown by the QML side. See AUTHORS.txt.
//
// The file format is the one AspeQt-2k26 writes, so phonebooks are shared
// between the two programs:
//
//     <Phonebook>
//         <BBS name="..." ip="..." port="23" protocol="TELNET"
//              login="..." password="..." keyfile="..."/>
//     </Phonebook>
//
// Reading accepts <BBS> at any depth. Upstream disagrees with itself here --
// its dialog searches the whole document while its R: device insists on a
// <Phonebook> nested inside the root, and so reads nothing at all from a file
// its own dialog wrote.
class PhoneBook
{
public:
    bool load(const QString &path);
    bool save(const QString &path) const;

    const QList<BbsEntry> &entries() const { return m_entries; }
    void setEntries(const QList<BbsEntry> &list) { m_entries = list; }
    int  count() const { return m_entries.size(); }
    void clear() { m_entries.clear(); }

    // Dial-by-name (ATDS=name). Case-insensitive; returns an entry whose
    // name is empty when there is no match.
    BbsEntry findByName(const QString &name) const;

private:
    QList<BbsEntry> m_entries;
};

#endif // PHONEBOOK_H
