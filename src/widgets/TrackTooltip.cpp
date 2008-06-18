/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  tracktooltip.cpp
  begin:     Tue 10 Feb 2004
  copyright: (C) 2004 by Christian Muehlhaeuser
  email:     chris@chris.de

  copyright: (C) 2005 by Gábor Lehel
  email:     illissius@gmail.com
*/

#include "TrackTooltip.h"

#include "amarokconfig.h"
#include "Amarok.h"
#include "amarok_export.h"
#include "App.h"
#include "Debug.h"
#include "meta/MetaUtility.h"
#include "moodbar.h"
#include "EngineController.h"
#include "TheInstances.h"

#include <KCalendarSystem>
#include <KStandardDirs>

#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QPixmap>
#include <QTimer>
#include <QToolTip>


K_GLOBAL_STATIC( TrackToolTip, s_trackToolTip );

TrackToolTip *TrackToolTip::instance()
{
    return s_trackToolTip;
}

TrackToolTip::TrackToolTip()
    : QWidget( 0 )
    , m_track( 0 )
    , m_haspos( false )
{
    DEBUG_BLOCK

    qAddPostRoutine( s_trackToolTip.destroy );  // Ensure that the dtor gets called when QCoreApplication destructs

    setWindowFlags( Qt::ToolTip );
//     setWindowOpacity( 0.6 ); // This doesn't work that well, the background should be transparent without the foreground, probably.
    QGridLayout *l = new QGridLayout;
    m_titleLabel = new QLabel( this );
    QFont f;
    f.setBold( true );
    m_titleLabel->setFont( f );
    l->addWidget( m_titleLabel, 0, 0, 1, 2 );
    m_imageLabel = new QLabel( this );
    l->addWidget( m_imageLabel, 1, 0 );

    m_otherInfoLabel = new QLabel( this );
    l->addWidget( m_otherInfoLabel, 1, 1 );
    setLayout( l );
    clear();

}

TrackToolTip::~TrackToolTip()
{
    DEBUG_BLOCK
}

void TrackToolTip::show( const QPoint & bottomRight )
{
    const int x = bottomRight.x() - width();
    const int y = bottomRight.y() - height();

    move( x, y );
    QWidget::show();
    QTimer::singleShot( 5000, this, SLOT( hide() ) ); // HACK: The system tray doesn't get mouse leave messages properly..
}

#define MOODBAR_WIDTH 150

void TrackToolTip::setTrack( const Meta::TrackPtr track, bool force )
{
    if( m_track.isNull() )
        return;

    if( m_track->artist() )
        unsubscribeTo( m_track->artist() );
    if( m_track->album() )
        unsubscribeTo( m_track->album() );
    unsubscribeTo( m_track );

    if( force || m_track != track )
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

        m_title = m_track->prettyName();
        if( m_track->artist() )
            m_title += i18n( " by " ) + m_track->artist()->prettyName();
        if( m_track->album() )
            m_title += i18n( " on " ) + m_track->album()->prettyName();

        m_tooltip += "</table></td>";
        m_tooltip += "</tr></table></center>";

        updateWidgets();

        subscribeTo( m_track );
        if( m_track->artist() )
            subscribeTo( m_track->artist() );
        if( m_track->album() )
            subscribeTo( m_track->album() );
    }
}

void TrackToolTip::setPos( int pos )
{
    if( m_pos != pos )
    {
        m_pos = pos;
        updateWidgets();
    }
}

void TrackToolTip::clear()
{
    m_pos     = 0;
    m_tooltip = i18n( "Amarok - rediscover your music" );
    m_track = Meta::TrackPtr();
    m_title.clear();
    m_image = QPixmap();

    updateWidgets();
}

QString TrackToolTip::tooltip() const
{
    QString tip = m_tooltip;
    if( m_track )
    {
        if( m_haspos )
            tip = tip.arg( Meta::msToPrettyTime( m_pos ) );
    }
    return tip;
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
    setTrack( The::engineController()->currentTrack(), true );
}

void TrackToolTip::metadataChanged( Meta::Album * /*album*/ )
{
    setTrack( The::engineController()->currentTrack(), true );
}

void TrackToolTip::metadataChanged( Meta::Artist * /*artist*/ )
{
    setTrack( The::engineController()->currentTrack(), true );
}

#include "TrackTooltip.moc"
