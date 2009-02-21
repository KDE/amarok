/***************************************************************************
 *   Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>             *
 *             (c) 2009 Ian Monroe <ian@monroe.nu>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

//!!! this file is included into SmartPointerList.h !!!

#include "Debug.h"

template<typename T>
SmartPointerList<T>::SmartPointerList()
    : QList<T>()
    , m_remover( new SmartPointerListRemover<T>( this ) )
    , m_ownsRemover( true )
{
}

template<typename T>
SmartPointerList<T>::SmartPointerList( const SmartPointerList<T>& that )
    : QList<T>( that )
    , m_remover( that.m_remover )
    , m_ownsRemover( false )
{
}

template<typename T> 
SmartPointerList<T>::~SmartPointerList()
{
    if( m_remover && m_ownsRemover )
    {
        m_remover->deleteLater();
    }
}

template<typename T> 
void
SmartPointerList<T>::append( const T& pointer )
{
    DEBUG_BLOCK

    debug() << "Adding Pointer: " << pointer;
    debug() << "Current size of list: " << size();
    if( m_remover )
        QObject::connect( pointer, SIGNAL( destroyed( QObject* ) ), m_remover, SLOT(removePointer(QObject*)) );
    else
        warning() << "The remover has been deleted.";
    QList<T>::append( pointer );
}

template<typename T>
SmartPointerListRemover<T>::SmartPointerListRemover( QList<T>* list )
    : QObject( 0 )
    , m_list( list )
{ 
}

template<typename T>
SmartPointerListRemover<T>::~SmartPointerListRemover()
{
}

template<typename T>
void
SmartPointerListRemover<T>::removePointer( QObject* pointer ) // SLOT
{
    DEBUG_BLOCK
    m_list->removeAll( qobject_cast<T>( pointer ) );
}

//moc doesn't like templates
//the following is just a copy-and-paste out of a Qt 4.5 moc, 
//adapted to templates

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SmartPointerListRemover[] = {

 // content:
       2,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   12, // methods  
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors

 // slots: signature, parameters, type, tag, flags
      33,   25,   24,   24, 0x0a,                 

       0        // eod
};                    

static const char qt_meta_stringdata_SmartPointerListRemover[] = {
    "SmartPointerListRemover\0\0pointer\0"
    "removePointer(QObject*)\0"
};
template<typename T>
const QMetaObject SmartPointerListRemover<T>::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_SmartPointerListRemover,
      qt_meta_data_SmartPointerListRemover, 0 }
};
template<typename T>
const QMetaObject *SmartPointerListRemover<T>::metaObject() const
{
    return &staticMetaObject;
}
template<typename T>
void *SmartPointerListRemover<T>::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SmartPointerListRemover))
        return static_cast<void*>(const_cast< SmartPointerListRemover*>(this));
    return QObject::qt_metacast(_clname);
}
template<typename T>
int SmartPointerListRemover<T>::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: removePointer((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        default: ;
        }
        _id -= 1;
    }
    return _id;
}
QT_END_MOC_NAMESPACE

