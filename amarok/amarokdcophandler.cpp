#include "amarokdcophandler.h"
#include "amarokslider.h"
#include "engine/enginebase.h"
#include "playerapp.h"
#include "playerwidget.h"

#include <dcopclient.h>

#include <kdebug.h>

AmarokDcopHandler::AmarokDcopHandler()
   : DCOPObject( "player" )
   , m_nowPlaying( "" )
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

int AmarokDcopHandler::trackTotalTime()
{
   return pApp->trackLength();
}

void AmarokDcopHandler::seek(int s)
{
   if ( (s > 0) && ( pApp->m_pEngine->state() != EngineBase::Empty ) )
   {
      pApp->m_pEngine->seek( s * 1000 );
   }
}

int AmarokDcopHandler::trackCurrentTime()
{
   //FIXME must be a more reliable way to get the currentTime
   return pApp->m_pPlayerWidget->m_pSlider->value();
}

void AmarokDcopHandler::addMedia(const KURL &url)
{
   pApp->insertMedia(url);
}

void AmarokDcopHandler::addMediaList(const KURL::List &urls)
{
   pApp->insertMedia(urls);
}


#include "amarokdcophandler.moc"
