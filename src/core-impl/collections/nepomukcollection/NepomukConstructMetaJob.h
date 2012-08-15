/****************************************************************************************
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

#ifndef NEPOMUKCONSTRUCTMETAJOB_H
#define NEPOMUKCONSTRUCTMETAJOB_H

#include "NepomukCollection.h"

#include "core-impl/collections/support/MemoryCollection.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"

#include <ThreadWeaver/Job>

namespace Collections
{

class NepomukCollection;

class NepomukConstructMetaJob : public ThreadWeaver::Job
{
    Q_OBJECT

public:
    explicit NepomukConstructMetaJob( NepomukCollection* coll );
    void run();

public slots:
    /**
     * Aborts the job as soon as it is safely possible
     */
    void abort();

signals:
    // signals for progress operation:
    void incrementProgress();
    void endProgressOperation( QObject *obj );
    // not used, defined to keep QObject::conect warning quiet
    void totalSteps( int steps );

private:
    QSharedPointer<Collections::MemoryCollection> m_mc;
    bool m_aborted;
    NepomukCollection* m_coll;

    /** These hash maps are used to store each of the {meta}Ptr so that duplicate {meta}
     * object are not created. In then end, each artist, album, genre, composer and track
     * will have only one corresponding Nepomuk{meta} Object.
     *
     * For the composition of the maps, we could have used
     * <Q*, Nepomuk{meta}Ptr
     * but that will result in a cyclic dependency hell.
     *
     * Genre and year are properties in Nepomuk and not classes and hence do not possess
     * associated QUrls. Hence their names are used as keys in the maps of genre & year
     */

    QHash<QUrl, Meta::TrackPtr> m_trackHash;
    QHash<QUrl, Meta::ArtistPtr> m_artistHash;
    QHash<QString, Meta::GenrePtr> m_genreHash;
    QHash<QUrl, Meta::ComposerPtr> m_composerHash;
    QHash<QUrl, Meta::AlbumPtr> m_albumHash;
    QHash<QUrl, Meta::LabelPtr> m_labelHash;
    QHash<QString, Meta::YearPtr> m_yearHash;
};

}

#endif // NEPOMUKCONSTRUCTMETAJOB_H
