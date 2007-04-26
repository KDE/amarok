/* 
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef DAAPCOLLECTION_H
#define DAAPCOLLECTION_H

#include "collection.h"
#include "memorycollection.h"
#include "reader.h"

#include <QtGlobal>

using namespace Daap;

class DaapCollectionFactory : public CollectionFactory
{
    Q_OBJECT
    public:
        DaapCollectionFactory();
        virtual ~DaapCollectionFactory();

        virtual void init();

    private slots:
        void connectToManualServers();
        QString resolve( const QString &hostname );
};

class DaapCollection : public QObject, public Collection, public MemoryCollection
{
    Q_OBJECT
    public:
        DaapCollection( const QString &host, const QString &ip, quint16 port );
        virtual ~DaapCollection();

        virtual void startFullScan();
        virtual QueryMaker* queryBuilder();

        virtual QString collectionId() const;
        virtual QString prettyName() const;

    private slots:
        void passwordRequired();
        void httpError( const QString &error );

    private:
        QString m_host;
        quint16 m_port;
        QString m_ip;

        Reader *m_reader;

};

#endif
