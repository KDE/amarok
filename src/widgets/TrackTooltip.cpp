/******************************************************************************
 * Copyright (C) 2004 Christian Muehlhaeuser <chris@chris.de>                 *
 * Copyright (C) 2005 Gábor Lehel <illissius@gmail.com>                       *
 * Copyright (C) 2008 Mark Kretschmann <kretschmann@kde.org>                  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#define DEBUG_PREFIX "TrackTooltip"

#include "TrackTooltip.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "Systray.h"
#include "amarok_export.h"
#include "amarokconfig.h"
#include "meta/MetaUtility.h"

#include <KStandardDirs>

#include <QApplication>
#include <QCursor>
#include <QDesktopWidget>
#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QTextDocument>
#include <QTimer>
#include <QToolTip>


TrackToolTip* TrackToolTip::s_instance = 0;

TrackToolTip *TrackToolTip::instance()
{
    return s_instance ? s_instance : new TrackToolTip( The::mainWindow() );
}

TrackToolTip::TrackToolTip( QWidget* parent )
    : QWidget( parent )
    , EngineObserver( The::engineController() )
    , m_track( 0 )
    , m_haspos( false )
    , m_timer( new QTimer( this ) )
{
    DEBUG_BLOCK

    s_instance = this;

    setWindowFlags( Qt::ToolTip );
    setWindowOpacity( 0.9 );

    QGridLayout *l = new QGridLayout;

    m_titleLabel = new QLabel( this );
    QFont f;
    m_titleLabel->setFont( f );
    l->addWidget( m_titleLabel, 0, 0, 1, 2 );

    m_imageLabel = new QLabel( this );
    l->addWidget( m_imageLabel, 1, 0 );

    m_otherInfoLabel = new QLabel( this );
    l->addWidget( m_otherInfoLabel, 1, 1 );

    setLayout( l );
    clear();

    m_titleLabel->installEventFilter( this );
    m_imageLabel-> installEventFilter( this );
    m_otherInfoLabel-> installEventFilter( this );

    // Timer for checking the mouse position
    connect( m_timer, SIGNAL( timeout() ), SLOT( slotTimer() ) );
}

TrackToolTip::~TrackToolTip()
{
    DEBUG_BLOCK
}

void TrackToolTip::show( const QPoint & bottomRight )
{
    DEBUG_BLOCK

    if( !isHidden() )
        return; 

    QRect rect = QApplication::desktop()->screenGeometry( bottomRight );

    const int x = bottomRight.x() - sizeHint().width();
    const int y = bottomRight.y() - sizeHint().height();

    if( rect.contains( x, y ) )
        //widget is on the top-left of the icon
        move( x, y );
    
    else
    {
        //so we don't have to keep calling sizeHint()
        const int w = sizeHint().width();
        const int h = sizeHint().height();
        
        //widget is on the bottom-right of the icon
        if( rect.contains( bottomRight.x() + w, bottomRight.y() + h ) )
            move( bottomRight.x(), bottomRight.y() );

        //widget is on the bottom-left of the icon
        else if( rect.contains( bottomRight.x() - w, bottomRight.y() + h ) )
            move( bottomRight.x() - w, bottomRight.y() );
        
        //widget is on the top-right of the icon
        else if( rect.contains( bottomRight.x() + w, bottomRight.y() - h ) )
            move( bottomRight.x(), bottomRight.y() - h );
        
        //this shouldn't happen, because it's covered in the first case...
        else
            move( x, y );
    }

    // Start monitoring the mouse position
    m_timer->start( 200 );

    QWidget::show();
    QTimer::singleShot( 15000, this, SLOT( hide() ) );
}

void TrackToolTip::setTrack()
{
    DEBUG_BLOCK

    clear();

    if( m_track )
    {
        m_tooltip.clear();
        m_haspos = false;

        QStringList left, right;
        const QString tableRow = "<tr><td width=70 align=right>%1: </td><td align=left>%2</td></tr>";

        QString filename = "", title = ""; //special case these, put the first one encountered on top

        right << QString("<i>%1%</i>").arg( The::engineController()->volume() );
        left << "<i>Volume</i>";

        const float score = m_track->score();
        if( score > 0.f )
        {
            right << QString::number( score, 'f', 2 );  // 2 digits after decimal point
            left << i18n( "Score" );
        }

        const int rating = m_track->rating();
        if( rating > 0 )
        {
            QString s;
            for( int i = 0; i < rating / 2; ++i )
                s += QString( "<img src=\"%1\" height=\"%2\" width=\"%3\">" )
                        .arg( KStandardDirs::locate( "data", "amarok/images/star.png" ) )
                        .arg( QFontMetrics( QToolTip::font() ).height() )
                        .arg( QFontMetrics( QToolTip::font() ).height() );
            if( rating % 2 )
                s += QString( "<img src=\"%1\" height=\"%2\" width=\"%3\">" )
                        .arg( KStandardDirs::locate( "data", "amarok/images/smallstar.png" ) )
                        .arg( QFontMetrics( QToolTip::font() ).height() )
                        .arg( QFontMetrics( QToolTip::font() ).height() );
            right << s;
            left << i18n( "Rating" );
        }

        const int count = m_track->playCount();
        if( count > 0 )
        {
            right << QString::number( count );
            left << i18n( "Play Count" );
        }

        const uint lastPlayed = m_track->lastPlayed();
        right << Amarok::verboseTimeSince( lastPlayed );
        left << i18n( "Last Played" );

        if( m_track->length() > 0 )
        {
            m_haspos = true;
            right << "%1 / " + Meta::secToPrettyTime( m_track->length() );
            left << i18n( "Length" );
        }

        // NOTE: It seems to be necessary to <center> each element indivdually
        m_tooltip += "<table cellpadding='2' cellspacing='2' align='center'><tr>";

        if( m_track->album() )
            m_image = m_track->album()->imageWithBorder( 100, 5 );

        m_tooltip += "<td><table cellpadding='0' cellspacing='0'>";

        for( int x = 0; x < left.count(); ++x )
            if ( !right[x].isEmpty() )
                m_tooltip += tableRow.arg( left[x] ).arg( right[x] );

        const QFontMetrics fontMetrics( font() );
        const int elideWidth = 200;
       
        m_title = "<center><b>" + fontMetrics.elidedText( Qt::escape(m_track->prettyName()), Qt::ElideRight, elideWidth ) + "</b>";
        if( m_track->artist() ) {
            const QString artist = fontMetrics.elidedText( Qt::escape(m_track->artist()->prettyName()), Qt::ElideRight, elideWidth );
            if( !artist.isEmpty() )
                m_title += i18n( " by <b>%1</b>", artist );
        }
        if( m_track->album() ) {
            const QString album = fontMetrics.elidedText( Qt::escape(m_track->album()->prettyName()), Qt::ElideRight, elideWidth );
            if( !album.isEmpty() )
                m_title += i18n( " on <b>%1</b></center>", album );
        }

        m_tooltip += "</table></td>";
        m_tooltip += "</tr></table></center>";

        updateWidgets();
    }
}

void TrackToolTip::clear()
{
    DEBUG_BLOCK

    m_trackPosition = 0;
    m_tooltip = i18n( "Amarok - No track playing." );
    m_title.clear();
    m_image = QPixmap();

    updateWidgets();
}

QString TrackToolTip::tooltip() const
{
    return ( m_track && m_haspos ) ? m_tooltip.arg( Meta::msToPrettyTime( m_trackPosition ) ) : m_tooltip;
}

void TrackToolTip::updateWidgets()
{
    m_imageLabel->setPixmap( m_image );
    m_titleLabel->setVisible( !m_title.isEmpty() );
    m_titleLabel->setText( m_title );
    m_otherInfoLabel->setText( tooltip() );
}

void TrackToolTip::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK

    setTrack();
    updateWidgets();
}

void TrackToolTip::metadataChanged( Meta::AlbumPtr album )
{
    Q_UNUSED( album )
    DEBUG_BLOCK

    setTrack();
}

void TrackToolTip::engineNewTrackPlaying()
{
    DEBUG_BLOCK

    if ( m_track )
    {
        unsubscribeFrom( m_track );
        if ( m_track->album() )
            unsubscribeFrom( m_track->album() );
    }

    m_track =  The::engineController()->currentTrack();

    if ( m_track )
    {
        subscribeTo( m_track );
        if ( m_track->album() )
            subscribeTo( m_track->album() );
    }

    setTrack();
}

void TrackToolTip::enginePlaybackEnded( int finalPosition, int trackLength, const QString &reason )
{
    Q_UNUSED( finalPosition )
    Q_UNUSED( trackLength )
    Q_UNUSED( reason )

    DEBUG_BLOCK

    if ( m_track )
    {
        unsubscribeFrom( m_track );
        if ( m_track->album() )
            unsubscribeFrom( m_track->album() );
        m_track = 0;
    }

    clear();
}

void TrackToolTip::engineTrackPositionChanged( long position, bool userSeek )
{
        Q_UNUSED( userSeek )

        if( !isHidden() && m_trackPosition != position ) {
            m_trackPosition = position;
            updateWidgets();
        }
}

void TrackToolTip::engineVolumeChanged( int percent )
{
    Q_UNUSED( percent )

    setTrack();
}

bool TrackToolTip::eventFilter( QObject* obj, QEvent* event )
{
    if( event->type() == QEvent::MouseButtonPress ) {
        mousePressEvent( dynamic_cast<QMouseEvent*>( event ) );
        return true;
    }

    return QObject::eventFilter( obj, event );
}

void TrackToolTip::mousePressEvent( QMouseEvent* )
{
    DEBUG_BLOCK

    hide();
}

void TrackToolTip::hide()  // SLOT
{
    QWidget::hide();
}

void TrackToolTip::slotTimer()  // SLOT
{
    if( !Amarok::TrayIcon::instance()->geometry().contains( QCursor::pos() ) ) {
        m_timer->stop();
        QTimer::singleShot( 500, this, SLOT( hide() ) );
    }        
}

#include "TrackTooltip.moc"
