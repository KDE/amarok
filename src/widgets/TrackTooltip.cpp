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

#include "TrackTooltip.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "EngineController.h"
#include "Systray.h"
#include "amarok_export.h"
#include "amarokconfig.h"
#include "meta/MetaUtility.h"

#include <KCalendarSystem>
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
#include <QTimer>
#include <QToolTip>
#include <QRect>
#include <QPoint>
#include <QDesktopWidget>


K_GLOBAL_STATIC( TrackToolTip, s_trackToolTip )

TrackToolTip *TrackToolTip::instance()
{
    return s_trackToolTip;
}

TrackToolTip::TrackToolTip()
    : QWidget( 0 )
    , m_track( 0 )
    , m_haspos( false )
    , m_timer( new QTimer( this ) )
{
    DEBUG_BLOCK

    qAddPostRoutine( s_trackToolTip.destroy );  // Ensure that the dtor gets called when QCoreApplication destructs

    setWindowFlags( Qt::ToolTip );
    setWindowOpacity( 0.8 );

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

    qRemovePostRoutine( s_trackToolTip.destroy );
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

void TrackToolTip::setTrack( const Meta::TrackPtr track )
{
    DEBUG_BLOCK

    if( m_track && m_track->artist() )
        unsubscribeFrom( m_track->artist() );
    if( m_track && m_track->album() )
        unsubscribeFrom( m_track->album() );
    unsubscribeFrom( m_track );

    if( track )
    {
        m_haspos = false;
        m_tooltip.clear();

        QStringList left, right;
        const QString tableRow = "<tr><td width=70 align=right>%1: </td><td align=left>%2</td></tr>";

        QString filename = "", title = ""; //special case these, put the first one encountered on top
        m_track = track;

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

        right << m_track->prettyName();
        left << i18n("Track");

        const QString length = Meta::secToPrettyTime( m_track->length() );
        if( !length.isEmpty() )
        {
            right << length;
            left << i18n( "Length" );
        }

        if( length > 0 )
        {
            m_haspos = true;
            right << "%1 / " + length;
            left << i18n( "Length" );
        }

        //NOTE it seems to be necessary to <center> each element indivdually
        m_tooltip += "<center><b>Amarok</b></center><table cellpadding='2' cellspacing='2' align='center'><tr>";

        if( m_track->album() )
            m_image = m_track->album()->image( 100 );

        m_tooltip += "<td><table cellpadding='0' cellspacing='0'>";

        for( int x = 0; x < left.count(); ++x )
            if ( !right[x].isEmpty() )
                m_tooltip += tableRow.arg( left[x] ).arg( right[x] );

        const QFontMetrics fontMetrics( font() );
        const int elideWidth = 200;
       
        m_title = "<b>" + fontMetrics.elidedText( m_track->prettyName(), Qt::ElideRight, elideWidth ) + "</b>";
        if( m_track->artist() ) {
            const QString artist = fontMetrics.elidedText( m_track->artist()->prettyName(), Qt::ElideRight, elideWidth );
            m_title += i18n( " by <b>%1</b>", artist );
        }
        if( m_track->album() ) {
            const QString album = fontMetrics.elidedText( m_track->album()->prettyName(), Qt::ElideRight, elideWidth );
            m_title += i18n( " on <b>%1</b>", album );
        }

        m_tooltip += "</table></td>";
        m_tooltip += "</tr></table></center>";

        updateWidgets();

        subscribeTo( m_track );
        if( m_track->artist() )
            subscribeTo( m_track->artist() );
        if( m_track->album() )
            subscribeTo( m_track->album() );
    }
    else
    {
        debug() << "track = null. Aborting.";
    }
}

void TrackToolTip::setTrackPosition( int pos )
{
    if( !isHidden() && m_trackPosition != pos ) {
        m_trackPosition = pos;
        updateWidgets();
    }
}

void TrackToolTip::clear()
{
    m_trackPosition = 0;
    m_tooltip = i18n( "Amarok - No track playing." );
    m_track = Meta::TrackPtr();
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
    if( !m_image.isNull() )
        m_imageLabel->setPixmap( m_image );

    m_titleLabel->setText( m_title );
    m_otherInfoLabel->setText( tooltip() );
}

void TrackToolTip::metadataChanged( Meta::Track * /*track*/ )
{
    setTrack( The::engineController()->currentTrack() );
}

void TrackToolTip::metadataChanged( Meta::Album * /*album*/ )
{
    setTrack( The::engineController()->currentTrack() );
}

void TrackToolTip::metadataChanged( Meta::Artist * /*artist*/ )
{
    setTrack( The::engineController()->currentTrack() );
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

