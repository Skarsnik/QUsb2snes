#ifndef IPSPARSE_H
#define IPSPARSE_H

#include <QByteArray>
#include <QList>


struct IPSReccord {
    quint32     offset; // Starting offset
    QByteArray  data; // the data
    bool        rle; // see parseIPS, but should be useless
    quint16     size; // pretty useless since it can be read on the data
};

QList<IPSReccord>   parseIPSData(QByteArray ipsData);

#endif // IPSPARSE_H
