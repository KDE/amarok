#include "playerapp.h"
#include "amarokdcophandler.h"

#include <dcopclient.h>


AmarokDcopHandler::AmarokDcopHandler()
   : DCOPObject( "player" )
   , m_nowPlaying( "" )
   , m_trackTotalTime( 0 )
{
    // Register with DCOP
    if ( !kapp->dcopClient()->isRegistered() )
    {
        kapp->dcopClient()->registerAs( "amarok" );
        kapp->dcopClient()->setDefaultObject( objId() );
    }
}

void AmarokDcopHandler::play()
{
    pApp->slotPlay();
}


void AmarokDcopHandler::stop()
{
    pApp->slotStop();
}


void AmarokDcopHandler::next()
{
    pApp->slotNext();
}


void AmarokDcopHandler::prev()
{
    pApp->slotPrev();
}


void AmarokDcopHandler::pause()
{
    pApp->slotPause();
}

void AmarokDcopHandler::setNowPlaying( const QString &s )
{
   m_nowPlaying = s;
}

QString AmarokDcopHandler::nowPlaying()
{
    return m_nowPlaying;
}

bool AmarokDcopHandler::isPlaying()
{
    return pApp->isPlaying();
}

void AmarokDcopHandler::setTrackTotalTime( int t )
{
   m_trackTotalTime = t;
}

int AmarokDcopHandler::trackTotalTime()
{
   return m_trackTotalTime;
}

void AmarokDcopHandler::seek(int ms)
{
}

int AmarokDcopHandler::trackCurrentTime()
{
   return 0;
}

void AmarokDcopHandler::addMedia(const KURL &url)
{
}

void AmarokDcopHandler::addMediaList(const KURL::List &urls)
{
}


#include "amarokdcophandler.moc"
