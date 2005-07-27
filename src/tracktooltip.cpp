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

void TrackToolTip::add( QWidget * widget, const MetaBundle & tags, int pos )
{
    static MetaBundle cachedtags = MetaBundle();
    static QString tipBuf = QString::null;
    static bool hasLength = true; //needed to know whether to .arg() in the current pos
    if( cachedtags != tags || tipBuf.isNull() ) //we don't autoupdate when the columns change, but *blahrg*.
    {
        tipBuf = "";
        QStringList left, right;
        const QString tableRow = "<tr><td width=70 align=right>%1:</td><td align=left>%2</td></tr>";

        QString filename = "", title = ""; //special case these, put the first one encountered on top
        hasLength = false;

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
            hasLength = true;
            right << "%9 / " + tags.prettyLength();
            left << playlist->columnText( PlaylistItem::Length );
        }

        //NOTE it seems to be necessary to <center> each element indivdually
        tipBuf += "<center><b>amaroK</b></center><table cellpadding='2' cellspacing='2' align='center'><tr>";

        QString image = CollectionDB::instance()->albumImage( tags );
        if ( !image.isEmpty() && image.find( QString("nocover") ) == -1 )
        {
            tipBuf +=( QString( "<td><table cellpadding='0' cellspacing='0'><tr><td>"
                               "<img src='%1'>"
                               "</td></tr></table></td>" )
                               .arg( image )
                               );

        }

        tipBuf += "<td><table cellpadding='0' cellspacing='0'>";

        if (tags.title().isEmpty() || tags.artist().isEmpty())
        // no title or no artist, so we add prettyTitle
            tipBuf += QString ("<tr><td align=center colspan='2'>%1</td></tr>")
                      .arg(tags.veryNiceTitle());
        for( uint x = 0; x < left.count(); ++x )
            if ( !right[x].isEmpty() )
                tipBuf += tableRow.arg( left[x] ).arg( right[x] );

        tipBuf += "</table></td>";
        tipBuf += "</tr></table></center>";

        cachedtags = tags;
    }

    QToolTip::add( widget, hasLength ? tipBuf.arg( MetaBundle::prettyLength( pos / 1000 ) ) : tipBuf );
}
