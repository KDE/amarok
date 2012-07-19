/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef NEPOMUKCOLLECTION_H
#define NEPOMUKCOLLECTION_H

#include "core/collections/Collection.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "NepomukConstructMetaJob.h"

#include <QString>
#include <QStringList>
#include <KIcon>
#include <QSharedPointer>

using namespace Meta;

namespace Collections
{
class NepomukConstructMetaJob;

// see if Meta::Observer also has to be inherited
class NepomukCollection : public Collections::Collection
{
    Q_OBJECT

public:
    /**
      * The entry point of Nepomuk Collection.
      * It gets an instance of NepomukResourceManager and instantiates it.
      * It returns true if Nepomuk enabled.
      * The constructor also constructs the {Meta}Maps which will be used
      * for all queries later on.
      */
    NepomukCollection();
    virtual ~NepomukCollection();

    /**
      * This function returns a generic MemoryQueryMaker.
      * Nepomuk Collection uses a MemoryQueryMaker as its QueryMaker
      * There is no need to construct a separate NepomukQueryMaker.
      */
    virtual Collections::QueryMaker* queryMaker();

    virtual bool isDirInCollection( const QString &path )
    {
        Q_UNUSED( path );
        return false;
    }

    virtual QString uidUrlProtocol() const;

    // unsure if this is really needed.
    virtual QString collectionId() const;

    virtual QString prettyName() const;

    virtual KIcon icon() const;

    virtual bool isWritable() const;

private:
    // nepomuk specific
    /**
      * This function is called to build the Nepomuk Collection by populating the Meta QMaps.
      * This function forms the crux of the Nepomuk Collection.
      * It first executes a query to fetch all resources of type 'audio'
      * It then enumerates them into Meta QMaps of MemoeryCollection.h
      *
      * After each track is extracted, its corresponding properties of artist, genre, composer
      * album ( year is not yet implemented ) is fetched and inserted into the NepomukTrack
      * object.
      */
    bool buildCollection();

private:
    friend class NepomukConstructMetaJob;

private:
    /**
      * This variable return true, if Nepomuk is available and enabled
      * It is also an indicator that NepomukCollection can be used.
      */
    bool m_nepomukCollectionReady;
    QWeakPointer<NepomukConstructMetaJob> m_constructMetaJob;

protected:
    QSharedPointer<Collections::MemoryCollection> m_mc;

};

} //namespace Collections
#endif // NEPOMUKCOLLECTION_H
