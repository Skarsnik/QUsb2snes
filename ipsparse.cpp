
#include "ipsparse.h"

#define BYTE3_TO_UINT(bp) \
     (((unsigned int)(bp)[0] << 16) & 0x00FF0000) | \
     (((unsigned int)(bp)[1] << 8) & 0x0000FF00) | \
     ((unsigned int)(bp)[2] & 0x000000FF)

#define BYTE2_TO_UINT(bp) \
    (((unsigned int)(bp)[0] << 8) & 0xFF00) | \
    ((unsigned int) (bp)[1] & 0x00FF)

QList<IPSReccord> parseIPSData(QByteArray ipsData)
{
    QList<IPSReccord> toret;
    //header 'PATCH'
    ipsData = ipsData.mid(5);
    unsigned int pos = 0;
    while (pos < ipsData.size())
    {
        QByteArray boffset = ipsData.mid(pos, 3); // This is the offset
        quint32 offset = BYTE3_TO_UINT(boffset);
        if (offset == 0x454f46) // This mark the end of the IPS patch
            break;
        IPSReccord ipsr;
        ipsr.offset = offset;
        pos += 3;
        QByteArray bsize = ipsData.mid(pos, 2); // then the size
        quint16 size = BYTE2_TO_UINT(bsize);
        pos += 2;
        if (size >= ipsData.size()) // This is an error
            return QList<IPSReccord>();
        if (size == 0) // RLE entry, meaning data are just a repeat of x time the next byte
        {
            ipsr.rle = true;
            bsize = ipsData.mid(pos, 2);
            ipsr.size = BYTE2_TO_UINT(bsize);
            ipsr.data = QByteArray(ipsr.size, ipsData.at(pos + 2));
            pos += 3;
        } else {
            ipsr.size = size;
            ipsr.data = ipsData.mid(pos, size);
            pos += size;
        }
        toret.append(ipsr);
    }
    return toret;
}
