/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "LastFmItem.h"

#include "amarokconfig.h"
#include "debug.h"
#include "collectiondb.h" // all deprecated, i know
#include "../contextview.h"
#include "enginecontroller.h"
#include "meta/meta.h"
#include "querybuilder.h"
#include "StarManager.h"

#include <QTextDocument> // Qt::escape etc
#include <QBuffer>

#include <kcodecs.h> // for data: URLs
#include <klocale.h>


using namespace Context;

LastFmItem::LastFmItem()
    : ContextItem()
    , ContextObserver( ContextView::instance() )
    , m_relatedArtistsBox( 0 )
    , m_suggestedSongsBox( 0 )
    , m_relHTMLSource( QString() )
    , m_sugHTMLSource( QString() )
    , m_sugBoxVisible( false )
    , m_relBoxVisible( false )
    , m_enabled( false )
{
}

void LastFmItem::message( const QString& message )
{
    if( message == QString( "boxRemoved" ) || message == QString( "boxesRemoved" ) )
    {
        m_relBoxVisible = false;
        m_sugBoxVisible = false;
    } else if( message == QString( "showCurrentTrack" ) )
    {
        if( m_enabled )
        {    
            showRelatedArtists();
            showSuggestedSongs();
        }
    }
}


void LastFmItem::showRelatedArtists()
{
    DEBUG_BLOCK
    // for now using old CollectionDB, i don't know if similarArtists has been ported to the new collection framework yet.
    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    if( !track ) return;

    CloudBox *relatedArtists = new CloudBox();
    relatedArtists->setTitle( i18n("Related Artists to %1", track->artist()->prettyName() ) );
    QStringList relations = CollectionDB::instance()->similarArtists( track->artist()->name(), 10 );
    foreach( QString r, relations )
        relatedArtists->addText( r );
    
    if( !m_relBoxVisible )
    {
        ContextView::instance()->addContextBox( relatedArtists, m_order, false );
        m_relBoxVisible = true;
    }
    
}

void LastFmItem::showSuggestedSongs()
{
    DEBUG_BLOCK
    // again, using old QueryBuilder until a) i figure out how to use the new one and b) i know it supports all of this.
    
    // for now using old CollectionDB, i don't know if similarArtists has been ported to the new collection framework yet.
        Meta::TrackPtr track = EngineController::instance()->currentTrack();
        if( !track )
            return;
        QStringList relArtists = CollectionDB::instance()->similarArtists( track->artist()->name(), 10 );
    
    QString token;
    
    QueryBuilder qb;
    QStringList values;
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valRating );
    qb.addMatches( QueryBuilder::tabArtist, relArtists );
    qb.sortByFavorite();
    qb.setLimit( 0, 10 );
    values = qb.run();
    
    // <Suggested Songs>
    if ( !values.isEmpty() )
    {
        m_sugHTMLSource.append( "<html><body>"
                             "<div id='suggested_box' class='box'>\n"
                             "<div id='suggested_box-header' class='box-header' style='cursor: pointer;'>\n"
                             "<span id='suggested_box-header-title' class='box-header-title'>\n"
                             + i18n( "Suggested Songs" ) +
                             "</span>\n"
                             "</div>\n"
                             "<table class='box-body' id='T_SS' width='100%' border='0' cellspacing='0' cellpadding='0'>\n" );
        
        for ( int i = 0; i < values.count(); i += 5 )
            m_sugHTMLSource.append(
                                 "<tr class='" + QString( (i % 8) ? "box-row-alt" : "box-row" ) + "'>\n"
                                 "<td class='song'>\n"
                                 "<a href=\"file:" + escapeHTMLAttr ( values[i] ) + "\">\n"
                                 "<span class='album-song-title'>\n"+ Qt::escape( values[i + 2] ) + "</span>\n"
                                 "<span class='song-separator'>\n"
                                 + i18n("&#xa0;&#8211; ") +
                                 "</span><span class='album-song-title'>\n" + Qt::escape( values[i + 1] ) + "</span>\n"
                                 "</a>\n"
                                 "</td>\n"
                                 "<td>\n" + statsHTML( static_cast<int>( values[i + 3].toFloat() ), values[i + 4].toInt() ) + "</td>\n"
                                 "<td width='1'></td>\n"
                                 "</tr>\n" );
        
        m_sugHTMLSource.append(
                             "</table>\n"
                                "</div> </html></body>\n" );
    
        m_suggestedSongsBox = new GenericInfoBox();
        m_suggestedSongsBox->setTitle( "Suggested Songs" );
        m_suggestedSongsBox->setContents( m_sugHTMLSource );
        
        if( !m_sugBoxVisible )
        {
            ContextView::instance()->addContextBox( m_suggestedSongsBox, m_order, false );
            m_sugBoxVisible = true;
        }
    }
        
    
}


QString LastFmItem::statsHTML(  int score, int rating, bool statsbox )
{
    if( !AmarokConfig::useScores() && !AmarokConfig::useRatings() )
        return "";
    
    if ( rating < 0 )
        rating = 0;
    if ( rating > 10 )
        rating = 10;
    
    QString table = QString( "<table %1 align='right' border='0' cellspacing='0' cellpadding='0' width='100%'>%2</table>\n" )
        .arg( statsbox ? "class='statsBox'" : "" );
    QString contents;
    
    if( AmarokConfig::useScores() )
        contents += QString( "<tr title='%1'>\n" ).arg( i18n( "Score: %1" ).arg( score ) ) +
        "<td class='sbtext' width='100%' align='right'>\n" + QString::number( score ) + "</td>\n"
        "<td align='left' width='1'>\n"
        "<div class='sbouter'>\n"
        "<div class='sbinner' style='width: "
        + QString::number( score / 2 ) + "px;'></div>\n"
        "</div>\n"
        "</td>\n"
        "</tr>\n";
    
    if( AmarokConfig::useRatings() )
    {
        contents += QString( "<tr title='%1'>\n" ).arg( i18n( "Rating: %1", /*MetaBundle::ratingDescription( rating )*/ QString::number( rating ) ) ) +
            "<td class='ratingBox' align='right' colspan='2'>\n";
        if( rating )
        {
            bool half = rating%2;
            contents += "<nobr>\n";
            
            QBuffer fullStarBuf;
            fullStarBuf.open( QIODevice::WriteOnly );
            StarManager::instance()->getStarImage( half ? rating/2 + 1 : rating/2 ).save( &fullStarBuf, "PNG" );
            fullStarBuf.close();
            QByteArray fullStar = KCodecs::base64Encode( fullStarBuf.buffer(), true );
            
            const QString img = "<img src='%1' height='13px' class='ratingStar'></img>\n";
            for( int i = 0, n = rating / 2; i < n; ++i )
                contents += img.arg( QString( "data:image/png;base64," ).append( fullStar ) );
            if( rating % 2 )
            {
                QBuffer halfStarBuf;
                halfStarBuf.open( QIODevice::WriteOnly );
                StarManager::instance()->getHalfStarImage( half ? rating/2 + 1 : rating/2 ).save( &halfStarBuf, "PNG" );
                halfStarBuf.close();
                QByteArray halfStar = KCodecs::base64Encode( halfStarBuf.buffer(), true );
                contents += img.arg( QString( "data:image/png;base64," ).append( halfStar ) );
            }
            contents += "</nobr>\n";
        }
        else
            contents += i18n( "Not rated" );
        contents += "</td>\n"
            "</tr>\n";
    }
    
    return table.arg( contents );
}

QString LastFmItem::escapeHTMLAttr( const QString &s )
{
    return QString(s).replace( "%", "%25" ).replace( "'", "%27" ).replace( "\"", "%22" ).replace( "#", "%23" ).replace( "?", "%3F" );
}

#include "LastFmItem.moc"
