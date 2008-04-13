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

#include "tracktooltip.h"

#include "amarokconfig.h"
#include "Amarok.h"
#include "amarok_export.h"
#include "App.h"
#include "debug.h"
#include "meta/MetaUtility.h"
#include "moodbar.h"

#include <KCalendarSystem>
#include <KStandardDirs>

#include <QApplication>
#include <QLabel>
#include <QPixmap>
#include <QBoxLayout>

TrackToolTip *TrackToolTip::instance()
{
    static TrackToolTip tip;
    return &tip;
}

TrackToolTip::TrackToolTip()
    : QWidget( 0 )
    , m_haspos( false )
    , m_track( 0 )
{
    setWindowFlags( Qt::ToolTip );
    QVBoxLayout *vbl = new QVBoxLayout;
    QHBoxLayout *hbl = new QHBoxLayout;
    m_imageLabel = new QLabel( this );
    hbl->addWidget( m_imageLabel );
    m_titleLabel = new QLabel( this );
    QFont f;
    f.setBold( true );
    m_titleLabel->setFont( f );
    hbl->addWidget( m_titleLabel );

    vbl->addLayout( hbl );
    m_otherInfoLabel = new QLabel( this );
    vbl->addWidget( m_otherInfoLabel );
    setLayout( vbl );
    clear();

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
    DEBUG_BLOCK
    if( m_track && m_track->artist() )
        m_track->artist()->unsubscribe( this );
    if( m_track && m_track->album() )
        m_track->album()->unsubscribe( this );
    if( m_track )
        m_track->unsubscribe( this );

    if( force || m_track != track )
    {
        DEBUG_LINE_INFO
        m_haspos = false;
        m_tooltip.clear();

        QStringList left, right;
        const QString tableRow = "<tr><td width=70 align=right>%1:</td><td align=left>%2</td></tr>";

        QString filename = "", title = ""; //special case these, put the first one encountered on top
        m_track = track;

//         Playlist *playlist = Playlist::instance();
//         const int n = playlist->numVisibleColumns();
//         for( int i = 0; i < n; ++i )
//         {
//             const int column = playlist->mapToLogicalColumn( i );
//
//             if( column == PlaylistItem::Score )
//             {
                const float score = m_track->score();
                if( score > 0.f )
                {
                    right << QString::number( score, 'f', 2 );  // 2 digits after decimal point
//                     left << playlist->columnText( column );
                    left << i18n( "Score" );
                }
//             }
//             else if( column == PlaylistItem::Rating )
//             {
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
//                     left << playlist->columnText( column );
                    left << i18n( "Rating" );
                }
//             }
//             else if( column == PlaylistItem::Mood )
//             {
//                 if( !AmarokConfig::showMoodbar() )
//                   continue;
//
//                 m_tags.moodbar().load();
//
//                 switch( tags.moodbar_const().state() )
//                   {
//                   case Moodbar::JobQueued:
//                   case Moodbar::JobRunning:
//                     right << tags.prettyText( column );
//                     left  << playlist->columnText( column );
//                     break;
//
//                   case Moodbar::Loaded:
//                     {
//                       // Ok so this is a hack, but it works quite well.
//                       // Save an image in the user's home directory just so
//                       // it can be referenced in an <img> tag.  Store which
//                       // moodbar is saved in m_moodbarURL so we don't have
//                       // to re-save it every second.
//                       left << playlist->columnText( column );
//                       QString filename = KStandardDirs::locateLocal( "data",
//                                                         "amarok/mood_tooltip.png" );
//                       int height = QFontMetrics( QToolTip::font() ).height() - 2;
//
//                       if( m_moodbarURL != tags.url().url() )
//                         {
//                           QPixmap moodbar
//                             = const_cast<MetaBundle&>( tags ).moodbar().draw(
//                                   MOODBAR_WIDTH, height );
//                           moodbar.save( filename, "PNG", 100 );
//                           m_moodbarURL = tags.url().url();
//                         }
//
//                       right << QString( "<img src=\"%1\" height=\"%2\" width=\"%3\">" )
//                           .arg( filename ).arg( height ).arg( MOODBAR_WIDTH );
//                     }
//                     break;
//
//                   default:
//                     // no tag
//                     break;
//                   }
//             }
//             else if( column == PlaylistItem::PlayCount )
//             {
                const int count = m_track->playCount();
                if( count > 0 )
                {
                    right << QString::number( count );
//                     left << playlist->columnText( column );
                    left << i18n( "Play Count" );
                }
//             }
//             else if( column == PlaylistItem::LastPlayed )
//             {
                const uint lastPlayed = m_track->lastPlayed();
                right << Amarok::verboseTimeSince( lastPlayed );
//                 left << playlist->columnText( column );
                left << i18n( "Last Played" );
//             }
//             else if( column == PlaylistItem::Filename && title.isEmpty() )
//                 filename = tags.prettyText( column );
//             else if( column == PlaylistItem::Title && filename.isEmpty() )
//                 title = tags.prettyText( column );
                right << m_track->prettyName();
                left << i18n("Track: ");
//             else if( column != PlaylistItem::Length )
//             {
                const QString length = QString::number( m_track->length() );
                if( !length.isEmpty() )
                {
                    right << length;
                    left << i18n( "Length" );
                }
//             }
//         }

//         if( tags.length() > 0 ) //special case this too, always on the bottom
        if( length > 0 )
        {
            m_haspos = true;
            right << "%1 / " + length;
//             left << playlist->columnText( PlaylistItem::Length );
            left << i18n( "Length" );
        }

        //NOTE it seems to be necessary to <center> each element indivdually
        m_tooltip += "<center><b>Amarok</b></center><table cellpadding='2' cellspacing='2' align='center'><tr>";

        if( m_track->album() )
            m_image = m_track->album()->image();

        m_tooltip += "<td><table cellpadding='0' cellspacing='0'>";

//         if (tags.title().isEmpty() || tags.artist().isEmpty())
//         // no title or no artist, so we add prettyTitle
//             m_tooltip += QString ("<tr><td align=center colspan='2'>%1</td></tr>")
//                       .arg(tags.veryNiceTitle());
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

        debug() << "TOOLTIP: " << m_tooltip;
        debug() << m_title;

        updateWidgets();

        m_track->subscribe( this );
        if( m_track->artist() )
            m_track->artist()->subscribe( this );
        if( m_track->album() )
            m_track->album()->subscribe( this );

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

// QPair<QString, QRect> TrackToolTip::toolTipText( QWidget*, const QPoint& ) const
// {
//     return QPair<QString, QRect>( tooltip(), QRect() );
// }

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

#include "tracktooltip.moc"
