#include "playerapp.h"
#include "browserwin.h"
#include "playerwidget.h"
#include "playlistwidget.h"
#include "amarokdcophandler.h"

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
   if ( (s > 0) && pApp->m_pPlayObject && !pApp->m_pPlayObject->isNull() )
   {
      Arts::poTime time;
      time.ms = 0;
      time.seconds = s;
      time.custom = 0;
      time.customUnit = std::string();

      pApp->m_pPlayObject->seek( time );
   }
}

int AmarokDcopHandler::trackCurrentTime()
{
   return pApp->m_pPlayerWidget->m_pSlider->value();
}

void AmarokDcopHandler::addMedia(const KURL &url)
{
   pApp->m_pBrowserWin->m_pPlaylistWidget->insertMedia(url);
}

void AmarokDcopHandler::addMediaList(const KURL::List &urls)
{
   KURL::List::ConstIterator it;
   for ( it = urls.begin(); it != urls.end(); it++ )
      addMedia( (*it) );
}


#include "amarokdcophandler.moc"
