/****************************************************************************
** Meta object code from reading C++ file 'Tuner.h'
**
** Created: Sun Oct 26 11:22:42 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.4.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Tuner.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Tuner.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.4.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Tuner[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
       7,    6,    6,    6, 0x05,
      28,    6,    6,    6, 0x05,
      49,    6,    6,    6, 0x05,

 // slots: signature, parameters, type, tag, flags
      66,    6,    6,    6, 0x0a,
      88,    6,    6,    6, 0x08,
     111,    6,    6,    6, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Tuner[] = {
    "Tuner\0\0stationName(QString)\0"
    "tracks(QList<Track>)\0error(Ws::Error)\0"
    "fetchFiveMoreTracks()\0onTuneReturn(WsReply*)\0"
    "onGetPlaylistReturn(WsReply*)\0"
};

const QMetaObject Tuner::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Tuner,
      qt_meta_data_Tuner, 0 }
};

const QMetaObject *Tuner::metaObject() const
{
    return &staticMetaObject;
}

void *Tuner::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Tuner))
        return static_cast<void*>(const_cast< Tuner*>(this));
    return QObject::qt_metacast(_clname);
}

int Tuner::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: stationName((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: tracks((*reinterpret_cast< const QList<Track>(*)>(_a[1]))); break;
        case 2: error((*reinterpret_cast< Ws::Error(*)>(_a[1]))); break;
        case 3: fetchFiveMoreTracks(); break;
        case 4: onTuneReturn((*reinterpret_cast< WsReply*(*)>(_a[1]))); break;
        case 5: onGetPlaylistReturn((*reinterpret_cast< WsReply*(*)>(_a[1]))); break;
        }
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void Tuner::stationName(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Tuner::tracks(const QList<Track> & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Tuner::error(Ws::Error _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_END_MOC_NAMESPACE
