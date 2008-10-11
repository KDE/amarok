/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "statusbar_ng/StatusBar.h"

#include "Debug.h"
#include "EngineController.h"
#include "meta/MetaUtility.h"
#include "meta/SourceInfoCapability.h"

#include "KJobProgressBar.h"

#include <QTextDocument>


StatusBarNG* StatusBarNG::s_instance = 0;

namespace The
{

StatusBarNG* statusBarNG()
{
    return StatusBarNG::instance();
}
}

StatusBarNG::StatusBarNG( QWidget * parent )
        : KStatusBar( parent )
        , EngineObserver( The::engineController() )
        , m_progressBar( new CompoundProgressBar( this ) )
        , m_busy( false )
        , m_shortMessageTimer( new QTimer( this ) )
{

    s_instance = this;

    addWidget( m_progressBar, 1 );
    m_progressBar->hide();

    connect( m_progressBar, SIGNAL( allDone() ), this, SLOT( hideProgress() ) );

    //setMaximumSize( The::mainWindow()->width(), m_progressBar->height() );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setContentsMargins( 0, 0, 0, 0 );
    setSizeGripEnabled( false );

    m_shortMessageTimer->setSingleShot( true );
    connect( m_shortMessageTimer, SIGNAL( timeout() ), this, SLOT( nextShortMessage() ) );



    m_nowPlayingWidget = new KHBox( 0 );
    m_nowPlayingWidget->setSpacing( 4 );

    m_nowPlayingLabel = new KSqueezedTextLabel( m_nowPlayingWidget );
    m_nowPlayingLabel->setTextElideMode( Qt::ElideRight );
    m_nowPlayingLabel->setAlignment( Qt::AlignRight );

    m_nowPlayingEmblem = new QLabel( m_nowPlayingWidget );
    m_nowPlayingEmblem->setFixedSize( 16, 16 );

    addPermanentWidget( m_nowPlayingWidget );

}


StatusBarNG::~StatusBarNG()
{
}

ProgressBarNG * StatusBarNG::newProgressOperation( QObject * owner, const QString & description )
{
    //clear any short message currently being displayed and stop timer if running...
    clearMessage();
    m_shortMessageTimer->stop();

    //also hide the now playing stuff:
    m_nowPlayingWidget->hide();
    ProgressBarNG * newBar = new ProgressBarNG( 0 );
    newBar->setDescription( description );
    m_progressBar->addProgressBar( newBar, owner );
    m_progressBar->show();
    m_busy = true;

    return newBar;

}

ProgressBarNG * StatusBarNG::newProgressOperation( KJob * job, const QString & description )
{

    //clear any short message currently being displayed and stop timer if running...
    clearMessage();
    m_shortMessageTimer->stop();

    //also hide the now playing stuff:
    m_nowPlayingWidget->hide();
    KJobProgressBar * newBar = new KJobProgressBar( 0, job );
    newBar->setDescription( description );
    m_progressBar->addProgressBar( newBar, job );
    m_progressBar->show();
    m_busy = true;

    return newBar;
}

void StatusBarNG::shortMessage( const QString & text )
{
    if ( !m_busy )
    {
        //not busy, so show right away
        showMessage( text );
        m_shortMessageTimer->start( SHORT_MESSAGE_DURATION );
    }
    else
    {
        m_shortMessageQue.append( text );
    }
}

void StatusBarNG::hideProgress()
{
    DEBUG_BLOCK
    m_progressBar->hide();
    m_busy = false;

    nextShortMessage();


}

void StatusBarNG::nextShortMessage()
{
    if ( m_shortMessageQue.count() > 0 )
    {
        m_busy = true;
        showMessage( m_shortMessageQue.takeFirst() );
        m_shortMessageTimer->start( SHORT_MESSAGE_DURATION );

    }
    else
    {
        clearMessage();
        m_busy = false;
        m_nowPlayingWidget->show();
    }
}



void StatusBarNG::metadataChanged( Meta::Track * track )
{
    if ( m_currentTrack )
        updateInfo( m_currentTrack );
    else
        engineNewTrackPlaying();
}

void StatusBarNG::engineStateChanged( Phonon::State state, Phonon::State oldState )
{
    DEBUG_BLOCK
    switch ( state )
    {
    case Phonon::StoppedState:
        m_nowPlayingLabel->setText( QString() );
        m_nowPlayingEmblem->hide();
        break;

    case Phonon::LoadingState:
        debug() << "LoadingState: clear text";
        if ( m_currentTrack )
            updateInfo( m_currentTrack );
        else
            m_nowPlayingLabel->setText( QString() );
        m_nowPlayingEmblem->hide();
        break;

    case Phonon::PausedState:
        m_nowPlayingLabel->setText( i18n( "Amarok is paused" ) ); // display TEMPORARY message
        m_nowPlayingEmblem->hide();
        break;

    case Phonon::PlayingState:
        debug() << "PlayingState: clear text";
        if ( m_currentTrack )
            updateInfo( m_currentTrack );
        //else
        //resetMainText(); // if we were paused, this is necessary
        break;

    case Phonon::ErrorState:
    case Phonon::BufferingState:
        break;
    }
}

void StatusBarNG::engineNewTrackPlaying()
{
    if ( m_currentTrack )
        unsubscribeFrom( m_currentTrack );

    m_currentTrack = The::engineController()->currentTrack();

    if ( !m_currentTrack )
    {
        m_currentTrack = Meta::TrackPtr();
        return;
    }
    subscribeTo( m_currentTrack );
    updateInfo( m_currentTrack );
}


void StatusBarNG::updateInfo( Meta::TrackPtr track )
{
    QString title       = Qt::escape( track->name() );
    QString prettyTitle = Qt::escape( track->prettyName() );
    QString artist      = track->artist() ? Qt::escape( track->artist()->name() ) : QString();
    QString album       = track->album() ? Qt::escape( track->album()->name() ) : QString();
    QString length      = Qt::escape( Meta::secToPrettyTime( track->length() ) );

    // ugly because of translation requirements
    if ( !title.isEmpty() && !artist.isEmpty() && !album.isEmpty() )
        title = i18nc( "track by artist on album", "<b>%1</b> by <b>%2</b> on <b>%3</b>", title, artist, album );

    else if ( !title.isEmpty() && !artist.isEmpty() )
        title = i18nc( "track by artist", "<b>%1</b> by <b>%2</b>", title, artist );

    else if ( !album.isEmpty() )
        // we try for pretty title as it may come out better
        title = i18nc( "track on album", "<b>%1</b> on <b>%2</b>", prettyTitle, album );
    else
        title = "<b>" + prettyTitle + "</b>";

    if ( title.isEmpty() )
        title = i18n( "Unknown track" );


    // check if we have any source info:

    Meta::SourceInfoCapability *sic = track->as<Meta::SourceInfoCapability>();
    if ( sic )
    {
        //is the source defined
        QString source = sic->sourceName();
        if ( !source.isEmpty() )
        {
            title += ' ' + i18n( "from" ) + " <b>" + source + "</b>";
            m_nowPlayingEmblem->setPixmap( sic->emblem() );
            m_nowPlayingEmblem->show();
        }
        else
            m_nowPlayingEmblem->hide();
        delete sic;

    }
    else
        m_nowPlayingEmblem->hide();

    // don't show '-' or '?'
    if ( length.length() > 1 )
    {
        title += " (";
        title += length;
        title += ')';
    }

    m_nowPlayingLabel->setText( i18n( "Playing: %1", title ) );
}

void StatusBarNG::longMessage( const QString & text, MessageType type )
{
}

void StatusBarNG::longMessageThreadSafe( const QString & text, MessageType type )
{
}





#include "StatusBar.moc"






