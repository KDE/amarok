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
#include "metabundle.h"
#include "collectiondb.h"
#include "playlist.h"
#include "playlistitem.h"
#include <qtooltip.h>
#include <qapplication.h>


TrackToolTip *TrackToolTip::instance()
{
    static TrackToolTip tip;
    return &tip;
}

TrackToolTip::TrackToolTip(): m_haspos( false )
{
    connect( CollectionDB::instance(), SIGNAL( coverChanged( const QString &, const QString & ) ),
             this, SLOT( slotCoverChanged( const QString &, const QString & ) ) );
    connect( Playlist::instance(), SIGNAL( columnsChanged() ), this, SLOT( slotColumnsChanged() ) );
    clear();
}

void TrackToolTip::addToWidget( QWidget *widget )
{
    if( widget && !m_widgets.containsRef( widget ) )
    {
        m_widgets.append( widget );
        QToolTip::add( widget, tooltip() );
    }
}

void TrackToolTip::removeFromWidget( QWidget *widget )
{
    if( widget && m_widgets.containsRef( widget ) )
    {
        QToolTip::remove( widget );
        m_widgets.removeRef( widget );
    }
}

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
        const int n = playlist->visibleColumns();
        for( int i = 0; i < n; ++i )
        {
            const int column = playlist->mapToLogicalColumn( i );

            if( column == PlaylistItem::Score )
            {
                const int score = CollectionDB::instance()->getSongPercentage( tags.url().path() );
                if( score > 0 )
                {
                    right << QString::number( score );
                    left << playlist->columnText( column );
                }
            }
            else if( column == PlaylistItem::Playcount )
            {
                const int count = CollectionDB::instance()->getPlayCount( tags.url().path() );
                if( count > 0 )
                {
                    right << QString::number( count );
                    left << playlist->columnText( column );
                }
            }
            else if( column == PlaylistItem::Filename && title.isEmpty() )
                filename = tags.infoByColumn( column, true );
            else if( column == PlaylistItem::Title && filename.isEmpty() )
                title = tags.infoByColumn( column, true );
            else if( column != PlaylistItem::Length )
            {
                const QString tag = tags.infoByColumn( column, true );
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
        m_tooltip += "<center><b>amaroK</b></center><table cellpadding='2' cellspacing='2' align='center'><tr>";

        m_tooltip += "%1"; //the cover gets substituted in, in tooltip()
        m_cover = CollectionDB::instance()->albumImage( tags );
        if ( m_cover == CollectionDB::instance()->notAvailCover() )
            m_cover = QString::null;

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
    m_cover   = QString::null;
    m_tooltip = i18n( "amaroK - rediscover your music" );
    m_tags    = MetaBundle();
    m_tags.setUrl( KURL() );

    updateWidgets();
}

void TrackToolTip::slotCoverChanged( const QString &artist, const QString &album )
{
    if( artist == m_tags.artist() && album == m_tags.album() )
    {
        m_cover = CollectionDB::instance()->albumImage( m_tags );
        if( m_cover == CollectionDB::instance()->notAvailCover() )
            m_cover = QString::null;

        updateWidgets();
    }
}

void TrackToolTip::slotColumnsChanged()
{
    setTrack( m_tags, true );
}

QString TrackToolTip::tooltip()
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
    for( QWidget *widget = m_widgets.first(); widget; widget = m_widgets.next() )
        QToolTip::add( widget, tooltip() );
}

#include "tracktooltip.moc"
