/****************************************************************************
** Meta object code from reading C++ file 'ScrobblerHttp.h'
**
** Created: Sun Oct 26 11:17:36 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.4.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../ScrobblerHttp.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ScrobblerHttp.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.4.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ScrobblerHttp[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      20,   15,   14,   14, 0x05,

 // slots: signature, parameters, type, tag, flags
      37,   14,   14,   14, 0x09,
      51,   14,   47,   14, 0x09,
      73,   64,   14,   14, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScrobblerHttp[] = {
    "ScrobblerHttp\0\0data\0done(QByteArray)\0"
    "request()\0int\0get(QString)\0id,error\0"
    "onRequestFinished(int,bool)\0"
};

const QMetaObject ScrobblerHttp::staticMetaObject = {
    { &QHttp::staticMetaObject, qt_meta_stringdata_ScrobblerHttp,
      qt_meta_data_ScrobblerHttp, 0 }
};

const QMetaObject *ScrobblerHttp::metaObject() const
{
    return &staticMetaObject;
}

void *ScrobblerHttp::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScrobblerHttp))
        return static_cast<void*>(const_cast< ScrobblerHttp*>(this));
    return QHttp::qt_metacast(_clname);
}

int ScrobblerHttp::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QHttp::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: done((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 1: request(); break;
        case 2: { int _r = get((*reinterpret_cast< QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 3: onRequestFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        }
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void ScrobblerHttp::done(const QByteArray & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
