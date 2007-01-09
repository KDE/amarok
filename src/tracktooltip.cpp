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

#include "amarok.h"
#include "app.h"
#include "amarokconfig.h"
#include "debug.h"
#include "metabundle.h"
#include "moodbar.h"
#include "collectiondb.h"
#include "playlist.h"
#include "playlistitem.h"
#include "podcastbundle.h"
#include <qapplication.h>
#include <kstandarddirs.h>


#include "tracktooltip.h"


TrackToolTip *TrackToolTip::instance()
{
    static TrackToolTip tip;
    return &tip;
}

TrackToolTip::TrackToolTip(): m_haspos( false )
{
    connect( CollectionDB::instance(), SIGNAL( coverChanged( const QString &, const QString & ) ),
             this, SLOT( slotCoverChanged( const QString &, const QString & ) ) );
    connect( CollectionDB::instance(), SIGNAL( imageFetched( const QString & ) ),
             this, SLOT( slotImageChanged( const QString & ) ) );
    connect( Playlist::instance(), SIGNAL( columnsChanged() ), this, SLOT( slotUpdate() ) );
    connect( CollectionDB::instance(), SIGNAL( scoreChanged( const QString&, float ) ),
             this, SLOT( slotUpdate( const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( ratingChanged( const QString&, int ) ),
             this, SLOT( slotUpdate( const QString& ) ) );
    // Only connect this once -- m_tags exists for the lifetime of this instance
    connect( &m_tags.moodbar(), SIGNAL( jobEvent( int ) ),
             SLOT( slotMoodbarEvent() ) );
    // This is so the moodbar can be re-rendered when AlterMood is changed
    connect( App::instance(), SIGNAL( moodbarPrefs( bool, bool, int, bool ) ),
             SLOT( slotMoodbarEvent() ) );
    clear();
}

void TrackToolTip::addToWidget( QWidget *widget )
{
    if( widget && !m_widgets.containsRef( widget ) )
    {
        m_widgets.append( widget );
        Amarok::ToolTip::add( this, widget );
    }
}

void TrackToolTip::removeFromWidget( QWidget *widget )
{
    if( widget && m_widgets.containsRef( widget ) )
    {
        Amarok::ToolTip::remove( widget );
        m_widgets.removeRef( widget );
    }
}


#define MOODBAR_WIDTH 150

void TrackToolTip::setTrack( const MetaBundle &tags, bool force )
{
    if( force || m_tags != tags || m_tags.url() != tags.url() )
    {
        m_haspos = false;
        m_tooltip = QString::null;

        QStringList left, right;
        const QString tableRow = "<tr><td width=70 align=right>%1:</td><td align=left>%2</td></tr>";

        QString filename = "", title = ""; //special case these, put the first one encountered on top

        Playlist *playlist = Playlist::instance();
        const int n = playlist->numVisibleColumns();
        for( int i = 0; i < n; ++i )
        {
            const int column = playlist->mapToLogicalColumn( i );

            if( column == PlaylistItem::Score )
            {
                const float score = CollectionDB::instance()->getSongPercentage( tags.url().path() );
                if( score > 0.f )
                {
                    right << QString::number( score, 'f', 2 );  // 2 digits after decimal point
                    left << playlist->columnText( column );
                }
            }
            else if( column == PlaylistItem::Rating )
            {
                const int rating = CollectionDB::instance()->getSongRating( tags.url().path() );
                if( rating > 0 )
                {
                    QString s;
                    for( int i = 0; i < rating / 2; ++i )
                        s += QString( "<img src=\"%1\" height=\"%2\" width=\"%3\">" )
                             .arg( locate( "data", "amarok/images/star.png" ) )
                             .arg( QFontMetrics( QToolTip::font() ).height() )
                             .arg( QFontMetrics( QToolTip::font() ).height() );
                    if( rating % 2 )
                        s += QString( "<img src=\"%1\" height=\"%2\" width=\"%3\">" )
                             .arg( locate( "data", "amarok/images/smallstar.png" ) )
                             .arg( QFontMetrics( QToolTip::font() ).height() )
                             .arg( QFontMetrics( QToolTip::font() ).height() );
                    right << s;
                    left << playlist->columnText( column );
                }
            }
            else if( column == PlaylistItem::Mood )
            {
                if( !AmarokConfig::showMoodbar() )
                  continue;

                m_tags.moodbar().load();

                switch( tags.moodbar_const().state() )
                  {
                  case Moodbar::JobQueued:
                  case Moodbar::JobRunning:
                    right << tags.prettyText( column );
                    left  << playlist->columnText( column );
                    break;

                  case Moodbar::Loaded:
                    {
                      // Ok so this is a hack, but it works quite well.
                      // Save an image in the user's home directory just so
                      // it can be referenced in an <img> tag.  Store which
                      // moodbar is saved in m_moodbarURL so we don't have
                      // to re-save it every second.
                      left << playlist->columnText( column );
                      QString filename = ::locateLocal( "data",
                                                        "amarok/mood_tooltip.png" );
                      int height = QFontMetrics( QToolTip::font() ).height() - 2;

                      if( m_moodbarURL != tags.url().url() )
                        {
                          QPixmap moodbar
                            = const_cast<MetaBundle&>( tags ).moodbar().draw(
                                  MOODBAR_WIDTH, height );
                          moodbar.save( filename, "PNG", 100 );
                          m_moodbarURL = tags.url().url();
                        }

                      right << QString( "<img src=\"%1\" height=\"%2\" width=\"%3\">" )
                          .arg( filename ).arg( height ).arg( MOODBAR_WIDTH );
                    }
                    break;

                  default:
                    // no tag
                    break;
                  }
            }
            else if( column == PlaylistItem::PlayCount )
            {
                const int count = CollectionDB::instance()->getPlayCount( tags.url().path() );
                if( count > 0 )
                {
                    right << QString::number( count );
                    left << playlist->columnText( column );
                }
            }
            else if( column == PlaylistItem::LastPlayed )
            {
                const uint lastPlayed = CollectionDB::instance()->getLastPlay( tags.url().path() ).toTime_t();
                right << Amarok::verboseTimeSince( lastPlayed );
                left << playlist->columnText( column );
            }
            else if( column == PlaylistItem::Filename && title.isEmpty() )
                filename = tags.prettyText( column );
            else if( column == PlaylistItem::Title && filename.isEmpty() )
                title = tags.prettyText( column );
            else if( column != PlaylistItem::Length )
            {
                const QString tag = tags.prettyText( column );
                if( !tag.isEmpty() )
                {
                    right << tag;
                    left << playlist->columnText( column );
                }
            }
        }

        if( !filename.isEmpty() )
        {
            right.prepend( filename );
            left.prepend( playlist->columnText( PlaylistItem::Filename ) );
        }
        else if( !title.isEmpty() )
        {
            right.prepend( title );
            left.prepend( playlist->columnText( PlaylistItem::Title ) );
        }

        if( tags.length() > 0 ) //special case this too, always on the bottom
        {
            m_haspos = true;
            right << "%9 / " + tags.prettyLength();
            left << playlist->columnText( PlaylistItem::Length );
        }

        //NOTE it seems to be necessary to <center> each element indivdually
        m_tooltip += "<center><b>Amarok</b></center><table cellpadding='2' cellspacing='2' align='center'><tr>";

        m_tooltip += "%1"; //the cover gets substituted in, in tooltip()
        m_cover = CollectionDB::instance()->podcastImage( tags, true );
        if( m_cover.isEmpty() || m_cover.contains( "nocover" ) != -1 )
        {
            m_cover = CollectionDB::instance()->albumImage( tags, true, 150 );
            if ( m_cover == CollectionDB::instance()->notAvailCover() )
                m_cover = QString::null;
        }

        m_tooltip += "<td><table cellpadding='0' cellspacing='0'>";

        if (tags.title().isEmpty() || tags.artist().isEmpty())
        // no title or no artist, so we add prettyTitle
            m_tooltip += QString ("<tr><td align=center colspan='2'>%1</td></tr>")
                      .arg(tags.veryNiceTitle());
        for( uint x = 0; x < left.count(); ++x )
            if ( !right[x].isEmpty() )
                m_tooltip += tableRow.arg( left[x] ).arg( right[x] );

        m_tooltip += "</table></td>";
        m_tooltip += "</tr></table></center>";

        m_tags = tags;
        updateWidgets();
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
    m_cover = QString::null;
    m_tooltip = i18n( "Amarok - rediscover your music" );
    m_tags    = MetaBundle();
    m_tags.setUrl( KURL() );

    updateWidgets();
}

QPair<QString, QRect> TrackToolTip::toolTipText( QWidget*, const QPoint& ) const
{
    return QPair<QString, QRect>( tooltip(), QRect() );
}

void TrackToolTip::slotCoverChanged( const QString &artist, const QString &album )
{
    if( artist == m_tags.artist() && album == m_tags.album() )
    {
        m_cover = CollectionDB::instance()->albumImage( m_tags, true, 150 );
        if( m_cover == CollectionDB::instance()->notAvailCover() )
            m_cover = QString::null;

        updateWidgets();
    }
}

void TrackToolTip::slotImageChanged( const QString &remoteURL )
{
    PodcastEpisodeBundle peb;
    if( CollectionDB::instance()->getPodcastEpisodeBundle( m_tags.url().url(), &peb ) )
    {
        PodcastChannelBundle pcb;
        if( CollectionDB::instance()->getPodcastChannelBundle( peb.parent().url(), &pcb ) )
        {
            if( pcb.imageURL().url() == remoteURL )
            {
                m_cover = CollectionDB::instance()->podcastImage( remoteURL );
                if( m_cover == CollectionDB::instance()->notAvailCover() )
                    m_cover = QString::null;

                updateWidgets();
            }

        }
    }
}

void TrackToolTip::slotUpdate( const QString &url )
{
    if( url.isNull() || url == m_tags.url().path() )
        setTrack( m_tags, true );
}

void
TrackToolTip::slotMoodbarEvent( void )
{
  // Clear this so the moodbar gets redrawn
  m_moodbarURL = QString::null;
  // Reset the moodbar in case AlterMood has changed
  m_tags.moodbar().reset();

  setTrack( m_tags, true );
}


QString TrackToolTip::tooltip() const
{
    QString tip = m_tooltip;;
    if( !m_tags.isEmpty() )
    {
        if( !m_cover.isEmpty() )
            tip = tip.arg( QString( "<td><table cellpadding='0' cellspacing='0'><tr><td>"
                                        "<img src='%1'>"
                                    "</td></tr></table></td>" ).arg( m_cover ) );
        else
            tip = tip.arg("");
        if( m_haspos )
            tip = tip.arg( MetaBundle::prettyLength( m_pos / 1000, true ) );
    }
    return tip;
}

void TrackToolTip::updateWidgets()
{
    Amarok::ToolTip::updateTip();
}

#include "tracktooltip.moc"
