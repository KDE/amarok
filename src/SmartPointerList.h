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

#ifndef AMAROK_SMARTPOINTERLIST_H
#define AMAROK_SMARTPOINTERLIST_H 

#include <QList>    // baseclass
#include <QObject>  // baseclass
#include <QPointer>

#include "amarok_export.h"

template<typename T>
class SmartPointerListRemover;


/**
    A QList for storing pointers to QObjects, that automatically removes the pointers when objects are deleted.
    @author Mark Kretschmann <kretschmann@kde.org> 
*/

template<typename T>
class AMAROK_EXPORT SmartPointerList : private QList<T>
{
    public:
        SmartPointerList();
        SmartPointerList( const SmartPointerList<T>& ); //only use for temp vars
        ~SmartPointerList();
        void append( const T& pointer );

        using QList<T>::count;
        using QList<T>::size;
        using QList<T>::const_iterator;
        using QList<T>::begin;
        using QList<T>::end;
        using QList<T>::at;
        using QList<T>::value;
        
        using QList<T>::clear; //no reason to bother disconnecting
    private:
        QPointer<SmartPointerListRemover<T> > m_remover;
        bool m_ownsRemover; //under the theory that copy constructors are used in
                            //'foreach' constructs and are quite temporary
};

template<typename T>
class SmartPointerListRemover : public QObject
{
    public: //Q_OBJECT
        static const QMetaObject staticMetaObject;
        virtual const QMetaObject *metaObject() const;
        virtual void *qt_metacast(const char *);
        virtual int qt_metacall(QMetaObject::Call, int, void **);

    public:
        SmartPointerListRemover( QList<T>* list );
        ~SmartPointerListRemover();
    public Q_SLOTS:
        void removePointer( QObject* pointer ); 
    private:
        QList<T>* m_list;
};

#include "SmartPointerList.cpp"

#endif
