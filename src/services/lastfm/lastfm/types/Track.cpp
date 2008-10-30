/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *    This program is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "Track.h"
#include "User.h"
#include "../core/CoreUrl.h"
#include "../ws/WsRequestBuilder.h"
#include "../ws/WsReply.h"
#include "../common/qt/md5.cpp"
#include <QFileInfo>


Track::Track()
{
    d = new TrackData;
}


Track::Track( const QDomElement& e )
{
    d = new TrackData;

    d->artist = e.namedItem( "artist" ).toElement().text();
    d->album =  e.namedItem( "album" ).toElement().text();
    d->title = e.namedItem( "track" ).toElement().text();
    d->trackNumber = 0;
    d->duration = e.namedItem( "duration" ).toElement().text().toInt();
    d->url = e.namedItem( "url" ).toElement().text();
    d->rating = e.namedItem( "rating" ).toElement().text().toUInt();
	d->extras["trackauth"] = e.namedItem( "auth" ).toElement().text();

    // this is necessary because the default return for toInt() is 0, and that
    // corresponds to Radio not Unknown :( oops.
    QString const source = e.namedItem( "source" ).toElement().text();
    if (source.isEmpty())
        d->source = Unknown;
    else
        d->source = (Source)source.toInt();

    // support 1.1.3 stringed timestamps, and 1.3.0 Unix Timestamps
    QString const t130 = e.namedItem( "timestamp" ).toElement().text();
    QDateTime const t113 = QDateTime::fromString( t130, "yyyy-MM-dd hh:mm:ss" );
    if (t113.isValid())
        d->time = t113;
    else
        d->time = QDateTime::fromTime_t( t130.toUInt() );

#if 0
    //old code, most likely unused
    setPath( e.namedItem( "path" ).toElement().text() );
    setFpId( e.namedItem( "fpId" ).toElement().text() );
    setMbId( e.namedItem( "mbId" ).toElement().text() );
    setPlayerId( e.namedItem( "playerId" ).toElement().text() );
#endif
}


QDomElement
Track::toDomElement( QDomDocument& document ) const
{
    QDomElement item = document.createElement( "item" );

    #define makeElement( tagname, getter ) { \
		QString v = getter; \
		if (!v.isEmpty())\
		{ \
			QDomElement e = document.createElement( tagname ); \
			e.appendChild( document.createTextNode( v ) ); \
			item.appendChild( e ); \
		} \
	}

    makeElement( "artist", d->artist );
    makeElement( "album", d->album );
    makeElement( "track", d->title );
    makeElement( "duration", QString::number( d->duration ) );
    makeElement( "timestamp", QString::number( d->time.toTime_t() ) );
    makeElement( "url", d->url.toString() );
    makeElement( "source", QString::number( d->source ) );
    makeElement( "rating", QString::number(d->rating) );
    makeElement( "fpId", fingerprintId() );
    makeElement( "mbId", mbid() );
	makeElement( "auth", d->extras[	"trackauth"] );

    return item;
}


QString
Track::toString( const QChar& separator ) const
{
    if ( d->artist.isEmpty() )
    {
        if ( d->title.isEmpty() )
            return QFileInfo( d->url.path() ).fileName();
        else
            return d->title;
    }

    if ( d->title.isEmpty() )
        return d->artist;

    return d->artist + ' ' + separator + ' ' + d->title;
}


QString
Track::durationString() const
{
    QTime t = QTime().addSecs( d->duration );
    if (d->duration < 60*60)
        return t.toString( "m:ss" );
    else
        return t.toString( "hh:mm:ss" );
}


WsReply*
Track::share( const User& recipient, const QString& message )
{
    return WsRequestBuilder( "track.share" )
        .add( "recipient", recipient )
        .add( "artist", d->artist )
        .add( "track", d->title )
        .addIfNotEmpty( "message", message )
        .post();
}


WsReply*
MutableTrack::love()
{
    if (d->extras.value("rating").size())
        return 0;
    
    d->extras["rating"] = "L";
    
	return WsRequestBuilder( "track.love" )
		.add( "artist", d->artist )
		.add( "track", d->title )
		.add( "api_key", QString( Ws::ApiKey ) )
		.add( "sk", QString( Ws::SessionKey ) )
		.post();
}


WsReply*
MutableTrack::ban()
{
    d->extras["rating"] = "B";
    
	return WsRequestBuilder( "track.ban" )
		.add( "artist", d->artist )
		.add( "track", d->title )
		.post();
}


void
MutableTrack::unlove()
{
    QString& r = d->extras["rating"];
    if (r == "L") r = "";
}


struct TrackWsRequestBuilder : WsRequestBuilder
{
	TrackWsRequestBuilder( const char* p ) : WsRequestBuilder( p )
	{}
	
	TrackWsRequestBuilder& add( Track const * const t )
	{
		if (t->mbid().isEmpty()) 
		{
			WsRequestBuilder::add( "artist", t->artist() );
			WsRequestBuilder::add( "track", t->title() );
		}
		else
			WsRequestBuilder::add( "mbid", t->mbid() );

		return *this;
	}
};


WsReply*
Track::getTopTags() const
{
	return TrackWsRequestBuilder( "track.getTopTags" ).add( this ).get();
}


WsReply*
Track::getTags() const
{
	return TrackWsRequestBuilder( "track.getTags" ).add( this ).get();
}


WsReply*
Track::addTags( const QStringList& tags ) const
{
    if (tags.isEmpty())
        return 0;
    
    QString comma_separated_tags;
    foreach( QString const tag, tags)
        comma_separated_tags += tag;
    
    return WsRequestBuilder( "track.addTags" )
            .add( "artist", d->artist )
            .add( "track", d->title )
            .add( "tags", comma_separated_tags )
            .post();
}


QUrl
Track::www() const
{
	QString const artist = CoreUrl::encode( d->artist );
	QString const track = CoreUrl::encode( d->title );
	return CoreUrl( "http://www.last.fm/music/" + artist + "/_/" + track ).localised();
}
