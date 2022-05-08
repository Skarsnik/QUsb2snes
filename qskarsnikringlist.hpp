#ifndef QSKARSNIKRINGLIST_H
#define QSKARSNIKRINGLIST_H

#include <QVector>

template<class T> class QSkarsnikRingList
{
public:
    QSkarsnikRingList(unsigned int initSize);

    void            append(const T &value);
    const T&        at(int i) const;
    size_t          size() const;

private:
    QVector<T>* _data;
    int         _size;
    int         _firstIndex;
    int         _lastIndex;
    bool        _cycled;
    int         vectorIndex(int index) const;
};

#include "qskarsnikringlist.hpp"

template<class T>
QSkarsnikRingList<T>::QSkarsnikRingList(unsigned int initSize)
{
    _data = new QVector<T>();
    _data->reserve(initSize);
    _size = initSize;
    _cycled = false;
    _firstIndex = 0;
    _lastIndex = -1;
}

#include <QDebug>

template<class T>
void QSkarsnikRingList<T>::append(const T &value)
{
    auto end = _lastIndex;
    //qInfo() << "Last index  :" << _lastIndex;
    //qInfo() << "first index :" << _firstIndex;
    _lastIndex = (_lastIndex + 1) % _size;
    if (_data->size() < _size)
        _data->append(value);
    else
        (*_data)[_lastIndex] = value;
    if (_lastIndex < end && _cycled == false)
        _cycled = true;
    if (_cycled)
        _firstIndex = vectorIndex(1);
}

template<class T> int QSkarsnikRingList<T>::vectorIndex(int index) const {
    return (_firstIndex + index) % _size;
}

template<class T>
const T &QSkarsnikRingList<T>::at(int i) const
{
    return _data->at(vectorIndex(i));
}

template<class T>
size_t QSkarsnikRingList<T>::size() const
{
    return _data->size();
}

#endif // QSKARSNIKRINGLIST_H
