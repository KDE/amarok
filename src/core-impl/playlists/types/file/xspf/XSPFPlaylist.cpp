/****************************************************************************************
 * Copyright (c) 2006 Mattias Fliesberg <mattias.fliesberg@gmail.com>                   *
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "XSPFPlaylist"

#include "XSPFPlaylist.h"

#include "core/capabilities/StreamInfoCapability.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/meta/stream/Stream.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"

using namespace Playlists;

XSPFPlaylist::XSPFPlaylist( const QUrl &url, Playlists::PlaylistProvider *provider, OnLoadAction onLoad )
    : PlaylistFile( url, provider )
    , QDomDocument()
    , m_autoAppendAfterLoad( onLoad == AppendToPlaylist )
{
}

XSPFPlaylist::~XSPFPlaylist()
{
}

void
XSPFPlaylist::savePlaylist(QFile &file)
{
    // if trackList item exists than no need to setup new file
    if ( documentElement().namedItem( "trackList" ).isNull() )
    {
        QDomElement root = createElement( "playlist" );

        root.setAttribute( "version", 1 );
        root.setAttribute( "xmlns", "http://xspf.org/ns/0/" );

        root.appendChild( createElement( "trackList" ) );

        appendChild( root );
    }

    setTrackList( tracks(), false );

    QTextStream stream( &file );
    stream.setCodec( "UTF-8" );
    QDomDocument::save( stream, 2 /*indent*/, QDomNode::EncodingFromTextStream );
}

bool
XSPFPlaylist::processContent( QByteArray &content )
{
    QString errorMsg;
    int errorLine, errorColumn;

    if( !setContent( content, &errorMsg, &errorLine, &errorColumn ) )
    {
        error() << "Error loading xml file: " "(" << errorMsg << ")"
                << " at line " << errorLine << ", column " << errorColumn;
        m_tracksLoaded = false;
    }
    else
        m_tracksLoaded = true;
    return m_tracksLoaded;
}

void
XSPFPlaylist::load()
{
    XSPFTrackList xspfTracks = trackList();

    foreach( const XSPFTrack &track, xspfTracks )
    {
       MetaProxy::TrackPtr proxyTrack( new MetaProxy::Track( track.location ) );
       //Fill in values from xspf..
       proxyTrack->setTitle( track.title );
       proxyTrack->setAlbum( track.album );
       proxyTrack->setArtist( track.creator );
       proxyTrack->setLength( track.duration );
       proxyTrack->setTrackNumber( track.trackNum );
       Meta::TrackPtr metaTrack( proxyTrack.data() );
       addProxyTrack( metaTrack );
     }

    //FIXME: this needs to be moved to whatever is creating the XSPFPlaylist
    if( m_autoAppendAfterLoad )
        The::playlistController()->insertPlaylist(
                    ::Playlist::ModelStack::instance()->bottom()->rowCount(),
                    Playlists::PlaylistPtr( this )
                );
}

bool
XSPFPlaylist::loadXSPF( QTextStream &stream )
{
    QByteArray content = stream.readAll().toUtf8();
    if ( !processContent( content ) )
        return false;
    load();

    return true;
}

bool
XSPFPlaylist::loadXSPF( QByteArray &content )
{
    if ( !processContent( content ) )
        return false;
    load();

    return true;
}

QString
XSPFPlaylist::name() const
{
    if ( m_tracksLoaded )
        return title();
    else
        return m_url.fileName();
}

QString
XSPFPlaylist::title() const
{
    return documentElement().namedItem( "title" ).firstChild().nodeValue();
}

QString
XSPFPlaylist::creator() const
{
    return documentElement().namedItem( "creator" ).firstChild().nodeValue();
}

QString
XSPFPlaylist::annotation() const
{
    return documentElement().namedItem( "annotation" ).firstChild().nodeValue();
}

QUrl
XSPFPlaylist::info() const
{
    return QUrl( documentElement().namedItem( "info" ).firstChild().nodeValue() );
}

QUrl
XSPFPlaylist::location() const
{
    return QUrl( documentElement().namedItem( "location" ).firstChild().nodeValue() );
}

QString
XSPFPlaylist::identifier() const
{
    return documentElement().namedItem( "identifier" ).firstChild().nodeValue();
}

QUrl
XSPFPlaylist::image() const
{
    return QUrl( documentElement().namedItem( "image" ).firstChild().nodeValue() );
}

QDateTime
XSPFPlaylist::date() const
{
    return QDateTime::fromString( documentElement().namedItem( "date" ).firstChild().nodeValue(), Qt::ISODate );
}

QUrl
XSPFPlaylist::license() const
{
    return QUrl( documentElement().namedItem( "license" ).firstChild().nodeValue() );
}

QList<QUrl>
XSPFPlaylist::attribution() const
{
    const QDomNodeList nodes = documentElement().namedItem( "attribution" ).childNodes();
    QList<QUrl> list;

    for( int i = 0, count = nodes.length(); i < count; ++i  )
    {
        const QDomNode &node = nodes.at( i );
        if( !node.firstChild().nodeValue().isNull() )
            list.append( node.firstChild().nodeValue() );
    }
    return list;
}

QUrl
XSPFPlaylist::link() const
{
    return QUrl( documentElement().namedItem( "link" ).firstChild().nodeValue() );
}

void
XSPFPlaylist::setTitle( const QString &title )
{
    QDomNode titleNode = documentElement().namedItem( "title" );
    if( titleNode.isNull() || !titleNode.hasChildNodes() )
    {
        QDomNode node = createElement( "title" );
        QDomNode subNode = createTextNode( title );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
    {
        documentElement().namedItem( "title" ).replaceChild( createTextNode( title ),
                                    documentElement().namedItem( "title" ).firstChild()
                                );
    }
    notifyObserversMetadataChanged();
    //write changes to file directly if we know where.
    if( !m_url.isEmpty() )
        PlaylistFile::save( false );
}

void
XSPFPlaylist::setCreator( const QString &creator )
{
    if( documentElement().namedItem( "creator" ).isNull() )
    {
        QDomNode node = createElement( "creator" );
        QDomNode subNode = createTextNode( creator );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
    {
        documentElement().namedItem( "creator" ).replaceChild( createTextNode( creator ),
                                            documentElement().namedItem( "creator" ).firstChild() );
    }

    //write changes to file directly if we know where
    if( !m_url.isEmpty() )
        PlaylistFile::save( false );
}

void
XSPFPlaylist::setAnnotation( const QString &annotation )
{
    if( documentElement().namedItem( "annotation" ).isNull() )
    {
        QDomNode node = createElement( "annotation" );
        QDomNode subNode = createTextNode( annotation );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
    {
        documentElement().namedItem( "annotation" ).replaceChild( createTextNode( annotation ),
                                        documentElement().namedItem( "annotation" ).firstChild() );
    }

    //write changes to file directly if we know where.
    if( !m_url.isEmpty() )
        PlaylistFile::save( false );
}

void
XSPFPlaylist::setInfo( const QUrl &info )
{

    if( documentElement().namedItem( "info" ).isNull() )
    {
        QDomNode node = createElement( "info" );
        QDomNode subNode = createTextNode( info.url() );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
    {
        documentElement().namedItem( "info" ).replaceChild( createTextNode( info.url() ),
                                            documentElement().namedItem( "info" ).firstChild() );
    }

    //write changes to file directly if we know where.
    if( !m_url.isEmpty() )
        PlaylistFile::save( false );
}

void
XSPFPlaylist::setLocation( const QUrl &location )
{
    if( documentElement().namedItem( "location" ).isNull() )
    {
        QDomNode node = createElement( "location" );
        QDomNode subNode = createTextNode( location.url() );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
    {
        documentElement().namedItem( "location" ).replaceChild( createTextNode( location.url() ),
                                        documentElement().namedItem( "location" ).firstChild() );
    }

    //write changes to file directly if we know where.
    if( !m_url.isEmpty() )
        PlaylistFile::save( false );
}

void
XSPFPlaylist::setIdentifier( const QString &identifier )
{
    if( documentElement().namedItem( "identifier" ).isNull() )
    {
        QDomNode node = createElement( "identifier" );
        QDomNode subNode = createTextNode( identifier );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
    {
        documentElement().namedItem( "identifier" ).replaceChild( createTextNode( identifier ),
                                        documentElement().namedItem( "identifier" ).firstChild() );
    }

    //write changes to file directly if we know where.
    if( !m_url.isEmpty() )
        PlaylistFile::save( false );
}

void
XSPFPlaylist::setImage( const QUrl &image )
{
    if( documentElement().namedItem( "image" ).isNull() )
    {
        QDomNode node = createElement( "image" );
        QDomNode subNode = createTextNode( image.url() );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
    {
        documentElement().namedItem( "image" ).replaceChild( createTextNode( image.url() ),
                                            documentElement().namedItem( "image" ).firstChild() );
    }

    //write changes to file directly if we know where.
    if( !m_url.isEmpty() )
        PlaylistFile::save( false );
}

void
XSPFPlaylist::setDate( const QDateTime &date )
{
    /* date needs timezone info to be compliant with the standard
    (ex. 2005-01-08T17:10:47-05:00 ) */

    if( documentElement().namedItem( "date" ).isNull() )
    {
        QDomNode node = createElement( "date" );
        QDomNode subNode = createTextNode( date.toString( "yyyy-MM-ddThh:mm:ss" ) );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
    {
        documentElement().namedItem( "date" )
                .replaceChild( createTextNode( date.toString( "yyyy-MM-ddThh:mm:ss" ) ),
                               documentElement().namedItem( "date" ).firstChild() );
    }

    //write changes to file directly if we know where.
    if( !m_url.isEmpty() )
        PlaylistFile::save( false );
}

void
XSPFPlaylist::setLicense( const QUrl &license )
{
    if( documentElement().namedItem( "license" ).isNull() )
    {
        QDomNode node = createElement( "license" );
        QDomNode subNode = createTextNode( license.url() );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
    {
        documentElement().namedItem( "license" ).replaceChild( createTextNode( license.url() ),
                                        documentElement().namedItem( "license" ).firstChild() );
    }

    //write changes to file directly if we know where.
    if( !m_url.isEmpty() )
        PlaylistFile::save( false );
}

void
XSPFPlaylist::setAttribution( const QUrl &attribution, bool append )
{
    if( !attribution.isValid() )
        return;

    if( documentElement().namedItem( "attribution" ).isNull() )
    {
        documentElement().insertBefore( createElement( "attribution" ),
                                        documentElement().namedItem( "trackList" ) );
    }

    if( append )
    {
        QDomNode subNode = createElement( "location" );
        QDomNode subSubNode = createTextNode( attribution.url() );
        subNode.appendChild( subSubNode );

        QDomNode first = documentElement().namedItem( "attribution" ).firstChild();
        documentElement().namedItem( "attribution" ).insertBefore( subNode, first );
    }
    else
    {
        QDomNode node = createElement( "attribution" );
        QDomNode subNode = createElement( "location" );
        QDomNode subSubNode = createTextNode( attribution.url() );
        subNode.appendChild( subSubNode );
        node.appendChild( subNode );
        documentElement().replaceChild( node, documentElement().namedItem( "attribution" ) );
    }

    //write changes to file directly if we know where.
    if( !m_url.isEmpty() )
        PlaylistFile::save( false );
}

void
XSPFPlaylist::setLink( const QUrl &link )
{
    if( documentElement().namedItem( "link" ).isNull() )
    {
        QDomNode node = createElement( "link" );
        QDomNode subNode = createTextNode( link.url() );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
    {
        documentElement().namedItem( "link" ).replaceChild( createTextNode( link.url() ),
                                            documentElement().namedItem( "link" ).firstChild() );
    }

    //write changes to file directly if we know where.
    if( !m_url.isEmpty() )
        PlaylistFile::save( false );
}

XSPFTrackList
XSPFPlaylist::trackList()
{
    XSPFTrackList list;

    QDomNode trackList = documentElement().namedItem( "trackList" );
    QDomNode subNode = trackList.firstChild();
    QDomNode subSubNode;

    while( !subNode.isNull() )
    {
        XSPFTrack track;
        subSubNode = subNode.firstChild();
        if( subNode.nodeName() == "track" )
        {
            while( !subSubNode.isNull() )
            {
                if( subSubNode.nodeName() == "location" )
                {
                    QByteArray path = subSubNode.firstChild().nodeValue().toAscii();
                    path.replace( '\\', '/' );

                    QUrl url = getAbsolutePath( QUrl::fromEncoded( path ) );
                    track.location = url;
                }
                else if( subSubNode.nodeName() == "title" )
                    track.title = subSubNode.firstChild().nodeValue();
                else if( subSubNode.nodeName() == "creator" )
                    track.creator = subSubNode.firstChild().nodeValue();
                else if( subSubNode.nodeName() == "duration" )
                    track.duration = subSubNode.firstChild().nodeValue().toInt();
                else if( subSubNode.nodeName() == "annotation" )
                    track.annotation = subSubNode.firstChild().nodeValue();
                else if( subSubNode.nodeName() == "album" )
                    track.album = subSubNode.firstChild().nodeValue();
                else if( subSubNode.nodeName() == "trackNum" )
                    track.trackNum = (uint)subSubNode.firstChild().nodeValue().toInt();
                else if( subSubNode.nodeName() == "identifier" )
                    track.identifier = subSubNode.firstChild().nodeValue();
                else if( subSubNode.nodeName() == "info" )
                    track.info = subSubNode.firstChild().nodeValue();
                else if( subSubNode.nodeName() == "image" )
                    track.image = subSubNode.firstChild().nodeValue();
                else if( subSubNode.nodeName() == "link" )
                    track.link = subSubNode.firstChild().nodeValue();

                subSubNode = subSubNode.nextSibling();
            }
        }
        list.append( track );
        subNode = subNode.nextSibling();
    }

    return list;
}

void
XSPFPlaylist::setTrackList( Meta::TrackList trackList, bool append )
{
    //documentation of attributes from http://www.xspf.org/xspf-v1.html

    if( documentElement().namedItem( "trackList" ).isNull() )
        documentElement().appendChild( createElement( "trackList" ) );

    QDomNode node = createElement( "trackList" );

    Meta::TrackPtr track;
    foreach( track, trackList ) // krazy:exclude=foreach
    {
        QDomNode subNode = createElement( "track" );

        //URI of resource to be rendered.
        QDomNode location = createElement( "location" );

        //Human-readable name of the track that authored the resource
        QDomNode title = createElement( "title" );

        //Human-readable name of the entity that authored the resource.
        QDomNode creator = createElement( "creator" );

        //A human-readable comment on the track.
        QDomNode annotation = createElement( "annotation" );

        //Human-readable name of the collection from which the resource comes
        QDomNode album = createElement( "album" );

        //Integer > 0 giving the ordinal position of the media in the album.
        QDomNode trackNum = createElement( "trackNum" );

        //The time to render a resource, in milliseconds. It MUST be a nonNegativeInteger.
        QDomNode duration = createElement( "duration" );

        //location-independent name, such as a MusicBrainz identifier. MUST be a legal URI.
        QDomNode identifier = createElement( "identifier" );

        //info - URI of a place where this resource can be bought or more info can be found.
        //QDomNode info = createElement( "info" );

        //image - URI of an image to display for the duration of the track.
        //QDomNode image = createElement( "image" );

        //link - element allows XSPF to be extended without the use of XML namespaces.
        //QDomNode link = createElement( "link" );

        //QDomNode meta
        //amarok specific queue info, see the XSPF specification's meta element
        QDomElement queue = createElement( "meta" );
        queue.setAttribute( "rel", "http://amarok.kde.org/queue" );

        //QDomNode extension

        #define APPENDNODE( X, Y ) \
        { \
            X.appendChild( createTextNode( Y ) );    \
            subNode.appendChild( X ); \
        }

        APPENDNODE( location, trackLocation( track ) )
        APPENDNODE( identifier, track->uidUrl() )

        Capabilities::StreamInfoCapability *streamInfo = track->create<Capabilities::StreamInfoCapability>();
        if( streamInfo ) // We have a stream, use it's metadata instead of the tracks.
        {
            if( !streamInfo->streamName().isEmpty() )
                APPENDNODE( title, streamInfo->streamName() )
            if( !streamInfo->streamSource().isEmpty() )
                APPENDNODE( creator, streamInfo->streamSource() )

            delete streamInfo;
        }
        else
        {
            if( !track->name().isEmpty() )
                APPENDNODE(title, track->name() )
            if( track->artist() && !track->artist()->name().isEmpty() )
                APPENDNODE(creator, track->artist()->name() );
        }
        if( !track->comment().isEmpty() )
            APPENDNODE(annotation, track->comment() );
        if( track->album() && !track->album()->name().isEmpty() )
            APPENDNODE( album, track->album()->name() );
        if( track->trackNumber() > 0 )
            APPENDNODE( trackNum, QString::number( track->trackNumber() ) );
        if( track->length() > 0 )
            APPENDNODE( duration, QString::number( track->length() ) );

        node.appendChild( subNode );
    }
    #undef APPENDNODE

    if( append )
    {
        while( !node.isNull() )
        {
            documentElement().namedItem( "trackList" ).appendChild( node.firstChild() );
            node = node.nextSibling();
        }
    }
    else
        documentElement().replaceChild( node, documentElement().namedItem( "trackList" ) );
}

void
XSPFPlaylist::setQueue( const QList<int> &queue )
{
    QDomElement q = createElement( "queue" );

    foreach( int row, queue )
    {
        QDomElement qTrack = createElement( "track" );
        qTrack.appendChild( createTextNode( QString::number( row ) ) );
        q.appendChild( qTrack );
    }

    QDomElement extension = createElement( "extension" );
    extension.setAttribute( "application", "http://amarok.kde.org" );
    extension.appendChild( q );

    QDomNode root = firstChild();
    root.appendChild( extension );
}

QList<int>
XSPFPlaylist::queue()
{
    QList<int> tracks;

    QDomElement extension = documentElement().firstChildElement( "extension" );
    if( extension.isNull() )
        return tracks;

    if( extension.attribute( "application" ) != "http://amarok.kde.org" )
        return tracks;

    QDomElement queue = extension.firstChildElement( "queue" );
    if( queue.isNull() )
        return tracks;

    for( QDomElement trackElem = queue.firstChildElement( "track" );
         !trackElem.isNull();
         trackElem = trackElem.nextSiblingElement( "track" ) )
    {
        tracks << trackElem.text().toInt();
    }

    return tracks;
}

void
XSPFPlaylist::setName( const QString &name )
{
    PlaylistFile::setName( name );
    setTitle( name );
}
