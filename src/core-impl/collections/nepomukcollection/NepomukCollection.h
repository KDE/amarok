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

#include <QString>
#include <QStringList>
#include <KIcon>
#include <QSharedPointer>


using namespace Meta;

namespace Collections
{

// see if Meta::Observer also has to be inherited
class NepomukCollection : public Collections::Collection
{
    Q_OBJECT

public:
    NepomukCollection();
    virtual ~NepomukCollection();

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
    bool buildCollection();
    void setupTrackMap(TrackMap &trackmap);
    void setupArtistMap(ArtistMap &artistmap);
    void setupGenreMap(GenreMap &genremap);
    void setupComposerMap(ComposerMap &composermap);
    void setupAlbumMap(AlbumMap &albummap);

    /** this function is used to update the members of the class
    * whenever the collection is changed (addition, deletion etc)
    */
    void updated();

private:
    bool m_nepomukCollectionReady;

protected:
    QSharedPointer<Collections::MemoryCollection> m_mc;

};

} //namespace Collections
#endif // NEPOMUKCOLLECTION_H
