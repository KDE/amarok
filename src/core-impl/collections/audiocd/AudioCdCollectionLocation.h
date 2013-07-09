/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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
 
#ifndef AUDIOCDCOLLECTIONLOCATION_H
#define AUDIOCDCOLLECTIONLOCATION_H

#include "core/collections/CollectionLocation.h"

#include "AudioCdCollection.h"

#include <KTemporaryFile>
#include <QWeakPointer>

namespace Collections {

/**
 * A custom CollectionLocation handling the encoding file type and so on for AudioCd collections
 */
class AudioCdCollectionLocation : public CollectionLocation
{
    Q_OBJECT
public:
    AudioCdCollectionLocation( QWeakPointer<AudioCdCollection> parentCollection );
    ~AudioCdCollectionLocation();

    Collections::Collection* collection() const;
    virtual void getKIOCopyableUrls( const Meta::TrackList & tracks );

private slots:
    void addToMap( Meta::TrackPtr track, const QString &fileName, bool succesful );
private:
    QWeakPointer<AudioCdCollection> m_collection;
    QMap<Meta::TrackPtr, KUrl> m_resultsMap;
    int m_tracks;
};

} //namespace Collections

#endif
