/* 
   Copyright (C) 2008 Daniel Winter <dw@danielwinter.de>

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


#ifndef NEPOMUKCOLLECTION_H_
#define NEPOMUKCOLLECTION_H_

#include "Collection.h"

#include <QString>
#include <QHash>

#include <Soprano/Client/DBusClient>

class NepomukCollectionFactory : public CollectionFactory
{
    Q_OBJECT
    public:
        NepomukCollectionFactory() {};
        virtual ~NepomukCollectionFactory() {};

        virtual void init();
};

class NepomukCollection : public Collection
{
public:
    NepomukCollection( Soprano::Client::DBusClient* );
    virtual ~NepomukCollection();
    
    virtual QueryMaker* queryMaker();
    
    virtual QString collectionId() const;
    virtual QString prettyName() const;
    
    virtual void lostDBusConnection();
    
    QString getNameForValue( const qint64 ) const;
    QString getUrlForValue( const qint64 ) const;

private:
    
    void initHashMaps();
    
    Soprano::Client::DBusClient *m_client;
    QHash< qint64, QString > m_nameForValue;
    QHash< qint64, QString > m_urlForValue;
};

#endif /*NEPOMUKCOLLECTION_H_*/
