/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
 * Copyright (c) 2013 Edward Toroshchin <amarok@hades.name>                             *
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

#include <QIcon>

namespace Collections
{


class NepomukCache;

/**
 * This collection class interfaces with KDE Nepomuk semantic data storage.
 *
 * It connects to the underlying Soprano database interface and provides a
 * QueryMaker interface to the objects of type nfo:Audio stored there.
 */
class NepomukCollection : public Collection
{
    Q_OBJECT

public:
    NepomukCollection();
    virtual ~NepomukCollection();

    virtual QueryMaker *queryMaker();
    virtual QString uidUrlProtocol() const;
    virtual QString collectionId() const;
    virtual QString prettyName() const;
    virtual QIcon icon() const;
    virtual bool isWritable() const;

    // TrackProvider methods
    virtual bool possiblyContainsTrack( const QUrl &url ) const;
    virtual Meta::TrackPtr trackForUrl( const QUrl &url );

    NepomukCache *cache() const { return m_cache; }

private:
    NepomukCache *m_cache;
};

} //namespace Collections
#endif // NEPOMUKCOLLECTION_H
