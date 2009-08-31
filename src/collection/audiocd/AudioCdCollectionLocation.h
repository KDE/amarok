/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#ifndef AUDIOCDCOLLECTIONLOCATION_H
#define AUDIOCDCOLLECTIONLOCATION_H

#include "CollectionLocation.h"

#include "AudioCdCollection.h"

/**
A custom CollectionLocation handling the encoding file type and so on for AudioCd collections

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class AudioCdCollectionLocation : public CollectionLocation
{
    Q_OBJECT
public:
    AudioCdCollectionLocation( const AudioCdCollection* parentCollection );
    ~AudioCdCollectionLocation();

    virtual void getKIOCopyableUrls( const Meta::TrackList &tracks );

    virtual void showSourceDialog( const Meta::TrackList &tracks, bool removeSources );

private slots:

    void formatSelected( int format );
    void formatSelectionCancelled();

    void onFormatSelected( int format );
    void onCancel();
    
private:

    const AudioCdCollection * m_collection;

};

#endif
