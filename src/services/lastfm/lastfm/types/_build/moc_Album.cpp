/****************************************************************************
** Meta object code from reading C++ file 'Album.h'
**
** Created: Sun Oct 26 11:17:32 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.4.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Album.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Album.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.4.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_AlbumImageFetcher[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      19,   18,   18,   18, 0x05,

 // slots: signature, parameters, type, tag, flags
      40,   18,   18,   18, 0x08,
      68,   18,   18,   18, 0x08,
      92,   18,   18,   18, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_AlbumImageFetcher[] = {
    "AlbumImageFetcher\0\0finished(QByteArray)\0"
    "onGetInfoFinished(WsReply*)\0"
    "onImageDataDownloaded()\0fail()\0"
};

const QMetaObject AlbumImageFetcher::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_AlbumImageFetcher,
      qt_meta_data_AlbumImageFetcher, 0 }
};

const QMetaObject *AlbumImageFetcher::metaObject() const
{
    return &staticMetaObject;
}

void *AlbumImageFetcher::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_AlbumImageFetcher))
        return static_cast<void*>(const_cast< AlbumImageFetcher*>(this));
    return QObject::qt_metacast(_clname);
}

int AlbumImageFetcher::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: finished((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 1: onGetInfoFinished((*reinterpret_cast< WsReply*(*)>(_a[1]))); break;
        case 2: onImageDataDownloaded(); break;
        case 3: fail(); break;
        }
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void AlbumImageFetcher::finished(const QByteArray & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
