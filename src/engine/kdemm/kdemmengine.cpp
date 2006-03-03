/***************************************************************************
         kdemmengine.cpp  -  KDE Multimedia (KDEMM) interface
                         -------------------
begin                : 2004-10-01
copyright            : (C) 2004 by Roland Gigler
email                : rolandg@web.de
what                 : interface to the KDE Multimedia interface (KDEMM)
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "enginebase.h"
#include "engineobserver.h"

#include <assert.h>
#include <math.h>            //setVolume(), timerEvent()
#include <string>
#include <vector>

#include <qdir.h>
#include <qdom.h>
#include <qfile.h>
#include <qlayout.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <kdemm/simpleplayer.h>

#include "kdemmengine.h"

#define KDEMM_TIMER 250

AMAROK_EXPORT_PLUGIN( KDEMMEngine )

using namespace KDE::Multimedia;

KDEMMEngine::KDEMMEngine( )
        : Engine::Base()
//        , m_scopeId( 0 )
//        , m_xfadeFadeout( false )
//        , m_xfadeValue( 0.0 )
//        , m_xfadeCurrent( "invalue2" )
        , m_state( Engine::Empty )
	, m_pPlayingTimer( new QTimer( this ) )
{
    kdDebug() << k_funcinfo << endl;
    m_player = new SimplePlayer();
}


KDEMMEngine::~KDEMMEngine()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;
    m_pPlayingTimer->stop();
    delete m_player;
    kdDebug() << "END " << k_funcinfo << endl;
}


bool KDEMMEngine::init()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;
   /*
    m_scopeSize = 1 << scopeSize;
    m_restoreEffects = restoreEffects;
    m_mixerHW = -1;   //initialize
    */

    connect ( m_pPlayingTimer, SIGNAL( timeout() ), this, SLOT( playingTimeout() ) );

    kdDebug() << "END " << k_funcinfo << endl;
    return true;
}


////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
////////////////////////////////////////////////////////////////////////////////

bool KDEMMEngine::canDecode( const KURL &url ) const
{
    static QStringList list;

    kdDebug() << "BEGIN " << k_funcinfo << endl;
    kdDebug() << "  Param: url: " << url << endl;
    //kdDebug() << "  url.protocol()   >" << url.protocol() <<"<"<< endl;

    if (url.protocol() == "http" ) return false;

    // TODO determine list of supported MimeTypes/Extensions from KDEMM
    list += QString("audio/x-mp3");

    KFileItem fileItem( KFileItem::Unknown, KFileItem::Unknown, url, false ); //false = determineMimeType straight away
    KMimeType::Ptr mimetype = fileItem.determineMimeType();
    kdDebug() << "mimetype: " << mimetype->name().latin1() << endl;

    return list.contains( mimetype->name().latin1() );
}  // canDecode


bool KDEMMEngine::load( const KURL& url, bool stream )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    m_isStream = stream;
    kdDebug() << "  m_url: " << m_url << endl;
    kdDebug() << "  Param: stream: " << stream << endl;
    kdDebug() << "  Param: url " << url << endl;

    if ( !url.isLocalFile() ) {         // for now
       return false;
    }

    if ( m_url == url ) {
       return true;
    } else {
       stop();
   }
    m_url = url;
    // the KDEMM  SimplePlayer dows the loading in the play method

    m_state = Engine::Idle;

    kdDebug() << "END " << k_funcinfo << endl;
    return true;
}   // load


bool KDEMMEngine::play( unsigned int offset)
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;
    kdDebug() << "  param: offset " << offset << endl;

    m_player->play(m_url);
    m_pPlayingTimer->start(KDEMM_TIMER, false);
    m_state = Engine::Playing;
    emit stateChanged( Engine::Playing );

    kdDebug() << "END " << k_funcinfo << endl;
    return true;
}   // play


/*
 * return current position in milli seconds
*/
uint KDEMMEngine::position() const
{
    uint pos=0;
    //kdDebug() << "BEGIN " << k_funcinfo << endl;

    //kdDebug() << "  totalTime: " << m_player->totalTime() << endl;
    //kdDebug() << "  currentTime: " << m_player->currentTime() << endl;

    pos = m_player->currentTime();

    //kdDebug() << "END " << k_funcinfo << endl;
    return (pos);
}   // position


//////////////////////////////////////////////////////////////////////

void KDEMMEngine::stop()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    //switch xfade channels
/*    m_xfadeCurrent = ( m_xfadeCurrent == "invalue1" ) ? "invalue2" : "invalue1";

    if ( m_xfadeValue == 0.0 )
        m_xfadeValue = 1.0;
*/

    m_player->stop();
    m_pPlayingTimer->stop();

    m_state = Engine::Idle;     // Empty
    emit stateChanged( m_state );
    kdDebug() << "END " << k_funcinfo << endl;
}


void KDEMMEngine::pause()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    // KDEMM: pause() cannot do un-pause, do it manually
    if (m_state == Engine::Paused){
        m_player->play(m_url);
        m_pPlayingTimer->start(KDEMM_TIMER, false);
        m_state = Engine::Playing;
    } else {
        m_player->pause();
        m_pPlayingTimer->stop();
        m_state = Engine::Paused;
    }

    emit stateChanged( m_state );
    kdDebug() << "END " << k_funcinfo << endl;
}   // pause


void KDEMMEngine::seek( unsigned int ms )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;
    kdDebug() << "  param: ms " << ms << endl;
    m_player->seek(ms);
    kdDebug() << "END " << k_funcinfo << endl;
}   // seek


void KDEMMEngine::setVolumeSW( unsigned int percent )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;
    kdDebug() << "  Param: percent " << percent << endl;


    float vol = percent*0.01;
    kdDebug() << " setting vol to " << vol << endl;
    m_player->setVolume(vol);

    kdDebug() << "END " << k_funcinfo << endl;
}   // setVolumeSW

////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
////////////////////////////////////////////////////////////////////////////////

void KDEMMEngine::playingTimeout() //SLOT
{
    //kdDebug() << "BEGIN " << k_funcinfo << endl;
    if( !m_player->isPlaying() ) {
        m_pPlayingTimer->stop();
        m_state = Engine::Idle;
        emit trackEnded();
    }
    //kdDebug() << "END " << k_funcinfo << endl;
}   // playingTimeout

#include "kdemmengine.moc"

