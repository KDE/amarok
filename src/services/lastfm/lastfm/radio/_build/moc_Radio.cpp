/****************************************************************************
** Meta object code from reading C++ file 'Radio.h'
**
** Created: Sun Oct 26 11:22:44 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.4.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Radio.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Radio.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.4.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Radio[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
       7,    6,    6,    6, 0x05,
      30,    6,    6,    6, 0x05,
      50,    6,    6,    6, 0x05,
      70,    6,    6,    6, 0x05,
      85,    6,    6,    6, 0x05,
      95,    6,    6,    6, 0x05,

 // slots: signature, parameters, type, tag, flags
     112,    6,    6,    6, 0x0a,
     131,    6,    6,    6, 0x0a,
     138,    6,    6,    6, 0x0a,
     145,    6,    6,    6, 0x08,
     169,  167,    6,    6, 0x08,
     219,    6,    6,    6, 0x08,
     269,    6,    6,    6, 0x08,
     293,    6,    6,    6, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Radio[] = {
    "Radio\0\0tuningIn(RadioStation)\0"
    "trackSpooled(Track)\0trackStarted(Track)\0"
    "buffering(int)\0stopped()\0error(Ws::Error)\0"
    "play(RadioStation)\0skip()\0stop()\0"
    "enqueue(QList<Track>)\0,\0"
    "onPhononStateChanged(Phonon::State,Phonon::State)\0"
    "onPhononCurrentSourceChanged(Phonon::MediaSource)\0"
    "onTunerError(Ws::Error)\0"
    "setStationNameIfCurrentlyBlank(QString)\0"
};

const QMetaObject Radio::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Radio,
      qt_meta_data_Radio, 0 }
};

const QMetaObject *Radio::metaObject() const
{
    return &staticMetaObject;
}

void *Radio::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Radio))
        return static_cast<void*>(const_cast< Radio*>(this));
    return QObject::qt_metacast(_clname);
}

int Radio::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: tuningIn((*reinterpret_cast< const RadioStation(*)>(_a[1]))); break;
        case 1: trackSpooled((*reinterpret_cast< const Track(*)>(_a[1]))); break;
        case 2: trackStarted((*reinterpret_cast< const Track(*)>(_a[1]))); break;
        case 3: buffering((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: stopped(); break;
        case 5: error((*reinterpret_cast< Ws::Error(*)>(_a[1]))); break;
        case 6: play((*reinterpret_cast< const RadioStation(*)>(_a[1]))); break;
        case 7: skip(); break;
        case 8: stop(); break;
        case 9: enqueue((*reinterpret_cast< const QList<Track>(*)>(_a[1]))); break;
        case 10: onPhononStateChanged((*reinterpret_cast< Phonon::State(*)>(_a[1])),(*reinterpret_cast< Phonon::State(*)>(_a[2]))); break;
        case 11: onPhononCurrentSourceChanged((*reinterpret_cast< const Phonon::MediaSource(*)>(_a[1]))); break;
        case 12: onTunerError((*reinterpret_cast< Ws::Error(*)>(_a[1]))); break;
        case 13: setStationNameIfCurrentlyBlank((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        }
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void Radio::tuningIn(const RadioStation & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Radio::trackSpooled(const Track & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Radio::trackStarted(const Track & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Radio::buffering(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Radio::stopped()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}

// SIGNAL 5
void Radio::error(Ws::Error _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_END_MOC_NAMESPACE
