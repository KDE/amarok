/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef NEPOMUKCOLLECTION_H_
#define NEPOMUKCOLLECTION_H_

#include "Collection.h"

#include <QString>
#include <QStringList>
#include <QHash>

#include <KIcon>

#include <Soprano/Model>

class KUrl;
class NepomukRegistry;

class NepomukCollectionFactory : public Amarok::CollectionFactory
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
    NepomukCollection( Soprano::Model* model, bool isFast );
    virtual ~NepomukCollection();
    
    virtual QueryMaker* queryMaker();
    
    virtual QString uidUrlProtocol() const;
    virtual QString collectionId() const;
    virtual QString prettyName() const;
    virtual KIcon icon() const { return KIcon("nepomuk"); }
    
    virtual bool possiblyContainsTrack( const KUrl &url ) const;
    virtual Meta::TrackPtr trackForUrl( const KUrl &url );
    
    // only for nepomuk collection plugin

    bool isEmpty() const;
    
    QString getNameForValue( const qint64 ) const;
    QString getUrlForValue( const qint64 ) const;
    qint64 valueForUrl( const QString& ) const;
    const QStringList& getAllNamesAndUrls( void ) const;
    NepomukRegistry* registry() const;
   

private:
    
    void initHashMaps();
    
    Soprano::Model *m_model;
    QHash< qint64, QString > m_nameForValue;
    QHash< qint64, QString > m_urlForValue;
    QHash< QString, qint64 > m_valueForUrl;
    QStringList m_allNamesAndUrls;
    bool m_isFast;
    NepomukRegistry *m_registry;
};

#endif /*NEPOMUKCOLLECTION_H_*/
