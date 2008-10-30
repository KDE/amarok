/****************************************************************************
** Meta object code from reading C++ file 'WsReply.h'
**
** Created: Sun Oct 26 11:17:32 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.4.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../WsReply.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WsReply.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.4.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_WsReply[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
       9,    8,    8,    8, 0x05,

 // slots: signature, parameters, type, tag, flags
      28,    8,    8,    8, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_WsReply[] = {
    "WsReply\0\0finished(WsReply*)\0onFinished()\0"
};

const QMetaObject WsReply::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_WsReply,
      qt_meta_data_WsReply, 0 }
};

const QMetaObject *WsReply::metaObject() const
{
    return &staticMetaObject;
}

void *WsReply::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_WsReply))
        return static_cast<void*>(const_cast< WsReply*>(this));
    return QObject::qt_metacast(_clname);
}

int WsReply::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: finished((*reinterpret_cast< WsReply*(*)>(_a[1]))); break;
        case 1: onFinished(); break;
        }
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void WsReply::finished(WsReply * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
