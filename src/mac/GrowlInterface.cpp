/***************************************************************************
 * copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>          *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#include "GrowlInterface.h"

#include "AmarokConfig.h"
#include "Debug.h"
#include "EngineController.h"
#include "MetaUtility.h" // for secToPrettyTime
 
#include "lastfm/core/mac/CFStringToQString.h"
#include <GrowlApplicationBridge-Carbon.h>
#include <CoreFoundation/CFString.h>


struct Growl_Delegate delegate;

GrowlInterface::GrowlInterface( QString appName ) :
                m_appName( appName )
              ,  EngineObserver( The::engineController() )
{
    CFStringRef app = QStringToCFString( appName );
    InitGrowlDelegate(&delegate);
    delegate.applicationName = CFSTR( "Amarok" );
    Growl_SetDelegate(&delegate);
}
 
void 
GrowlInterface::show( Meta::TrackPtr track )
{
    DEBUG_BLOCK
    QString text;
    if( !track || track->playableUrl().isEmpty() )
        text = i18n( "No track playing" );
    else
    {
        text = track->prettyName();
        if( track->artist() && !track->artist()->prettyName().isEmpty() )
            text = track->artist()->prettyName() + " - " + text;
        if( track->album() && !track->album()->prettyName().isEmpty() )
            text += "\n (" + track->album()->prettyName() + ") ";
        else
            text += "\n";
        if( track->length() > 0 )
            text += Meta::secToPrettyTime( track->length() );
    }
    
    if( text.isEmpty() )
        text =  track->playableUrl().fileName();

    if( text.startsWith( "- " ) ) //When we only have a title tag, _something_ prepends a fucking hyphen. Remove that.
        text = text.mid( 2 );

    if( text.isEmpty() ) //still
        text = i18n("No information available for this track");

    QImage image;
    if( track && track->album() )
        image = track->album()->imageWithBorder( 100, 5 ).toImage();

    CFDataRef imgData = CFDataCreate( kCFAllocatorDefault, image.bits(), image.numBytes() );
    
    show( text, imgData );
    
    
}

void
GrowlInterface::show( QString text, CFDataRef img )
{
    debug() << "is growl enabled:" << AmarokConfig::growlEnabled();
    if( AmarokConfig::growlEnabled() )
        Growl_NotifyWithTitleDescriptionNameIconPriorityStickyClickContext( CFSTR( "Amarok" ), QStringToCFString( text ), CFSTR( "Song Playing"), img, 0, false, 0 );
}

void 
GrowlInterface::engineNewTrackPlaying()
{
    DEBUG_BLOCK
    Meta::TrackPtr track = The::engineController()->currentTrack();

    unsubscribeFrom( m_currentTrack );
    m_currentTrack = track;
    subscribeTo( track );
    metadataChanged( track );
    
    show( m_currentTrack );
}

void
GrowlInterface::engineStateChanged( Phonon::State state, Phonon::State oldState )
{
    Q_UNUSED( oldState )
    DEBUG_BLOCK
    if( state == Phonon::PausedState )
        show( i18n( "Paused" ), 0 );
}


void
GrowlInterface::engineVolumeChanged( int newVolume )
{
  //  volChanged( newVolume );
}

void
GrowlInterface::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK

   // show( m_currentTrack );
} 
