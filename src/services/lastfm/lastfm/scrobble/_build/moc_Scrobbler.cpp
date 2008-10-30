/****************************************************************************
** Meta object code from reading C++ file 'Scrobbler.h'
**
** Created: Sun Oct 26 11:17:36 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.4.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Scrobbler.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Scrobbler.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.4.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Scrobbler[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      21,   11,   10,   10, 0x05,
      47,   42,   10,   10, 0x25,

 // slots: signature, parameters, type, tag, flags
      59,   10,   10,   10, 0x0a,
      77,   10,   10,   10, 0x0a,
      90,   10,   10,   10, 0x0a,
      99,   10,   10,   10, 0x08,
     129,   10,   10,   10, 0x08,
     160,   10,   10,   10, 0x08,
     191,   10,   10,   10, 0x08,
     216,   10,   10,   10, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Scrobbler[] = {
    "Scrobbler\0\0code,data\0status(int,QVariant)\0"
    "code\0status(int)\0nowPlaying(Track)\0"
    "cache(Track)\0submit()\0"
    "onHandshakeReturn(QByteArray)\0"
    "onNowPlayingReturn(QByteArray)\0"
    "onSubmissionReturn(QByteArray)\0"
    "onSubmissionStarted(int)\0"
    "onHandshakeHeaderReceived(QHttpResponseHeader)\0"
};

const QMetaObject Scrobbler::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Scrobbler,
      qt_meta_data_Scrobbler, 0 }
};

const QMetaObject *Scrobbler::metaObject() const
{
    return &staticMetaObject;
}

void *Scrobbler::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Scrobbler))
        return static_cast<void*>(const_cast< Scrobbler*>(this));
    return QObject::qt_metacast(_clname);
}

int Scrobbler::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: status((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QVariant(*)>(_a[2]))); break;
        case 1: status((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: nowPlaying((*reinterpret_cast< const Track(*)>(_a[1]))); break;
        case 3: cache((*reinterpret_cast< const Track(*)>(_a[1]))); break;
        case 4: submit(); break;
        case 5: onHandshakeReturn((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 6: onNowPlayingReturn((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 7: onSubmissionReturn((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 8: onSubmissionStarted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: onHandshakeHeaderReceived((*reinterpret_cast< const QHttpResponseHeader(*)>(_a[1]))); break;
        }
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void Scrobbler::status(int _t1, QVariant _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, 1, _a);
}
QT_END_MOC_NAMESPACE
