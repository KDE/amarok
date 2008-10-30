/****************************************************************************
** Meta object code from reading C++ file 'WsNetEvent.h'
**
** Created: Sun Oct 26 11:17:32 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.4.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../WsNetEvent.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WsNetEvent.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.4.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_WsNetEventAdapter[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      34,   19,   18,   18, 0x05,
      56,   19,   18,   18, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_WsNetEventAdapter[] = {
    "WsNetEventAdapter\0\0connectionName\0"
    "connectionUp(QString)\0connectionDown(QString)\0"
};

const QMetaObject WsNetEventAdapter::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_WsNetEventAdapter,
      qt_meta_data_WsNetEventAdapter, 0 }
};

const QMetaObject *WsNetEventAdapter::metaObject() const
{
    return &staticMetaObject;
}

void *WsNetEventAdapter::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_WsNetEventAdapter))
        return static_cast<void*>(const_cast< WsNetEventAdapter*>(this));
    return QObject::qt_metacast(_clname);
}

int WsNetEventAdapter::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: connectionUp((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: connectionDown((*reinterpret_cast< QString(*)>(_a[1]))); break;
        }
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void WsNetEventAdapter::connectionUp(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void WsNetEventAdapter::connectionDown(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
static const uint qt_meta_data_WsNetEvent[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      27,   12,   11,   11, 0x05,
      49,   12,   11,   11, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_WsNetEvent[] = {
    "WsNetEvent\0\0connectionName\0"
    "connectionUp(QString)\0connectionDown(QString)\0"
};

const QMetaObject WsNetEvent::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_WsNetEvent,
      qt_meta_data_WsNetEvent, 0 }
};

const QMetaObject *WsNetEvent::metaObject() const
{
    return &staticMetaObject;
}

void *WsNetEvent::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_WsNetEvent))
        return static_cast<void*>(const_cast< WsNetEvent*>(this));
    return QObject::qt_metacast(_clname);
}

int WsNetEvent::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: connectionUp((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: connectionDown((*reinterpret_cast< QString(*)>(_a[1]))); break;
        }
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void WsNetEvent::connectionUp(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void WsNetEvent::connectionDown(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
