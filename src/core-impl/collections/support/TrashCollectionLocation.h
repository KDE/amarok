/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef TRASHCOLLECTIONLOCATION_H
#define TRASHCOLLECTIONLOCATION_H

#include "core/collections/CollectionLocation.h"
#include <KIO/Job>

class KJob;

namespace Collections {

/**
  * Utility class that allows moving tracks to the KIO trash using standard
  * CollectionLocation API. It is not intended to be a collection, but more
  * as a black hole destination.
  */
class TrashCollectionLocation : public CollectionLocation
{
    Q_OBJECT

public:
    TrashCollectionLocation();
    ~TrashCollectionLocation() override;

    QString prettyLocation() const override;
    bool isWritable() const override;

protected:
    void copyUrlsToCollection( const QMap<Meta::TrackPtr, QUrl> &sources,
                               const Transcoding::Configuration &configuration ) override;
    void showDestinationDialog( const Meta::TrackList &tracks, bool removeSources,
                                const Transcoding::Configuration &configuration ) override;

private Q_SLOTS:
    void slotTrashJobFinished( KJob *job );

private:
    bool m_trashConfirmed;
    QHash<KJob*, Meta::TrackList> m_trashJobs;
};

} //namespace Collections

#endif // TRASHCOLLECTIONLOCATION_H
