/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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
 
#ifndef AMAROK_GROWL_INTERFACE_H
#define AMAROK_GROWL_INTERFACE_H

 
 
#include <QString> 
#include <QImage>
 
#include "core/support/Debug.h"
#include "core/meta/Meta.h"
#include "EngineController.h" 
#include "core/engine/EngineObserver.h"
    
 // NOTE if not on mac, this whole file is useless, so not even going to try
class GrowlInterface : public Engine::EngineObserver , public Meta::Observer
{
    public:
        GrowlInterface( QString appName );
     
        void show( Meta::TrackPtr );
     
    protected:
        // Reimplemented from EngineObserver
        virtual void engineVolumeChanged( int );
        virtual void engineNewTrackPlaying();
        virtual void engineStateChanged( Phonon::State state, Phonon::State oldState );

        // Reimplemented from Meta::Observer
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::TrackPtr track ); 
    private:
        QString m_appName;
        Meta::TrackPtr m_currentTrack;
     
};

#endif
 
