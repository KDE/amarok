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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "XSPFPlaylist.h"

#include "Debug.h"
#include "CollectionManager.h"
#include "MainWindow.h"
#include "meta/proxy/MetaProxy.h"
#include "meta/MetaUtility.h"
#include "meta/capabilities/StreamInfoCapability.h"
#include "meta/stream/Stream.h"
#include "meta/file/File.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "PlaylistManager.h"
#include "PlaylistFileSupport.h"

#include "timecode/TimecodeMeta.h"

#include <kurl.h>
#include <KMessageBox>

#include <QDateTime>
#include <QDomElement>
#include <QFile>
#include <QString>

#include <typeinfo>

namespace Meta
{

XSPFPlaylist::XSPFPlaylist()
    : PlaylistFile()
    , QDomDocument()
    , m_url( Meta::newPlaylistFilePath( "xspf" ) )
{
    m_name = m_url.fileName();
    QDomElement root = createElement( "playlist" );

    root.setAttribute( "version", 1 );
    root.setAttribute( "xmlns", "http://xspf.org/ns/1/" );

    root.appendChild( createElement( "trackList" ) );

    appendChild( root );
}

XSPFPlaylist::XSPFPlaylist( const KUrl &url, bool autoAppend )
    : PlaylistFile( url )
    , QDomDocument()
    , m_url( url )
    , m_autoAppendAfterLoad( autoAppend )
{
    DEBUG_BLOCK
    debug() << "url: " << m_url;
    m_name = m_url.fileName();

    //check if file is local or remote
    if ( m_url.isLocalFile() )
    {
        QFile file( m_url.toLocalFile() );
        if( !file.open( QIODevice::ReadOnly ) ) {
            debug() << "cannot open file";
            return;
        }

        QTextStream stream( &file );
        stream.setAutoDetectUnicode( true );

        loadXSPF( stream );
    }
    else
    {
        The::playlistManager()->downloadPlaylist( m_url, PlaylistFilePtr( this ) );
    }
}

XSPFPlaylist::XSPFPlaylist( Meta::TrackList list )
    : PlaylistFile()
    , QDomDocument()
{
    DEBUG_BLOCK

    QDomElement root = createElement( "playlist" );

    root.setAttribute( "version", 1 );
    root.setAttribute( "xmlns", "http://xspf.org/ns/0/" );

    root.appendChild( createElement( "trackList" ) );

    appendChild( root );
    setTrackList( list );
}

XSPFPlaylist::~XSPFPlaylist()
{}

bool
XSPFPlaylist::save( const KUrl &location, bool relative )
{
    DEBUG_BLOCK
    Q_UNUSED( relative );

    KUrl savePath = location;
    //if the location is a directory append the name of this playlist.
    if( savePath.fileName().isNull() )
        savePath.setFileName( name() );

    QFile file;
    
    if( location.isLocalFile() )
    {
        file.setFileName( savePath.toLocalFile() );
    }
    else
    {
        file.setFileName( savePath.path() );
    }

    if( file.exists() )
        //TODO: prompt for overwrite.
        {
            if( KUrl( ::Playlist::ModelStack::instance()->source()->defaultPlaylistPath() ) != savePath )
            {
                return false;
            }
            warning() << "The file" << location << "exists, overwriting...";
        }
    if( !file.open( QIODevice::WriteOnly ) )
    {
        if( The::mainWindow() ) // MainWindow might already be destroyed at this point (at program shutdown)
            KMessageBox::sorry( The::mainWindow(), i18n( "Cannot write playlist (%1).", file.fileName() ) );
        else
            warning() << QString( "Cannot write playlist (%1)." ).arg( file.fileName() ); 

        return false;
    }

    QTextStream stream ( &file );
    stream.setCodec( "UTF-8" );
    QDomDocument::save( stream, 2 /*indent*/, QDomNode::EncodingFromTextStream );

    return true;
}

bool
XSPFPlaylist::loadXSPF( QTextStream &stream )
{
    DEBUG_BLOCK
    QString errorMsg;
    int errorLine, errorColumn;

    QString rawText = stream.readAll();
    
    if ( !setContent( rawText, &errorMsg, &errorLine, &errorColumn ) )
    {
        debug() << "[XSPFPlaylist]: Error loading xml file: " "(" << errorMsg << ")"
                << " at line " << errorLine << ", column " << errorColumn;
        return false;
    }

    if( m_autoAppendAfterLoad )
        The::playlistController()->insertPlaylist( ::Playlist::ModelStack::instance()->source()->rowCount(), Meta::PlaylistPtr( this ) );

    return true;
}

TrackList
XSPFPlaylist::tracks()
{
    DEBUG_BLOCK
    XSPFTrackList xspfTracks = trackList();
    TrackList tracks;

    foreach( const XSPFTrack &track, xspfTracks )
    {
        TrackPtr trackPtr;
        if( !track.identifier.isEmpty() )
            trackPtr = CollectionManager::instance()->trackForUrl( track.identifier );
        else
            trackPtr = CollectionManager::instance()->trackForUrl( track.location );
        if ( trackPtr )
        {
            if( !trackPtr->isPlayable() )
                trackPtr = CollectionManager::instance()->trackForUrl( track.identifier );
        }

        if ( trackPtr )
        {
            if ( typeid( * trackPtr.data() ) == typeid( MetaStream::Track ) )
            {
                MetaStream::Track * streamTrack = dynamic_cast<MetaStream::Track *> ( trackPtr.data() );
                if ( streamTrack )
                {
                    streamTrack->setTitle( track.title );
                    streamTrack->setAlbum( track.album );
                    streamTrack->setArtist( track.creator );
                }
            }
            else if ( typeid( * trackPtr.data() ) == typeid( Meta::TimecodeTrack ) )
            {
                Meta::TimecodeTrack * timecodeTrack = dynamic_cast<Meta::TimecodeTrack *> ( trackPtr.data() );
                if ( timecodeTrack )
                { 
                    timecodeTrack->beginMetaDataUpdate();
                    timecodeTrack->setTitle( track.title );
                    timecodeTrack->setAlbum( track.album );
                    timecodeTrack->setArtist( track.creator );
                    timecodeTrack->endMetaDataUpdate();
                }
            }
            
            tracks << trackPtr;
        }

        
        // why do we need this? sqlplaylist is not doing this
        // we don't want (probably) unplayable tracks
        // and it causes problems for me (DanielW) as long
        // amarok not respects Track::isPlayable()
        /*else {
        
            MetaProxy::Track *proxyTrack = new MetaProxy::Track( track.location );
            {
                //Fill in values from xspf..
                QVariantMap map;
                map.insert( Meta::Field::TITLE, track.title );
                map.insert( Meta::Field::ALBUM, track.album );
                map.insert( Meta::Field::ARTIST, track.creator );
                map.insert( Meta::Field::LENGTH, track.duration );
                map.insert( Meta::Field::TRACKNUMBER, track.trackNum );
                map.insert( Meta::Field::URL, track.location );
                Meta::Field::updateTrack( proxyTrack, map );
            }
            tracks << Meta::TrackPtr( proxyTrack );
    //         tracks << CollectionManager::instance()->trackForUrl( track.location );
        }*/
        
    }
    return tracks;
}

QString
XSPFPlaylist::title() const
{
    return documentElement().namedItem( "title" ).firstChild().nodeValue();
}

QString
XSPFPlaylist::creator()
{
    return documentElement().namedItem( "creator" ).firstChild().nodeValue();
}

QString
XSPFPlaylist::annotation()
{
    return documentElement().namedItem( "annotation" ).firstChild().nodeValue();
}

KUrl
XSPFPlaylist::info()
{
    return KUrl( documentElement().namedItem( "info" ).firstChild().nodeValue() );
}

KUrl
XSPFPlaylist::location()
{
    return KUrl( documentElement().namedItem( "location" ).firstChild().nodeValue() );
}

QString
XSPFPlaylist::identifier()
{
    return documentElement().namedItem( "identifier" ).firstChild().nodeValue();
}

KUrl
XSPFPlaylist::image()
{
    return KUrl( documentElement().namedItem( "image" ).firstChild().nodeValue() );
}

QDateTime
XSPFPlaylist::date()
{
    return QDateTime::fromString( documentElement().namedItem( "date" ).firstChild().nodeValue(), Qt::ISODate );
}

KUrl
XSPFPlaylist::license()
{
    return KUrl( documentElement().namedItem( "license" ).firstChild().nodeValue() );
}

KUrl::List
XSPFPlaylist::attribution()
{
    QDomNode node = documentElement().namedItem( "attribution" );
    KUrl::List list;

    while ( !node.isNull() )
    {
        if ( !node.namedItem( "location" ).firstChild().nodeValue().isNull() )
            list.append( node.namedItem( "location" ).firstChild().nodeValue() );
        else if ( !node.namedItem( "identifier" ).firstChild().nodeValue().isNull() )
            list.append( node.namedItem( "identifier" ).firstChild().nodeValue() );

        node = node.nextSibling();
    }

    return list;
}

KUrl
XSPFPlaylist::link()
{
    return KUrl( documentElement().namedItem( "link" ).firstChild().nodeValue() );
}

void
XSPFPlaylist::setTitle( const QString &title )
{
    if ( documentElement().namedItem( "title" ).isNull() )
    {
        QDomNode node = createElement( "title" );
        QDomNode subNode = createTextNode( title );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
        documentElement().namedItem( "title" ).replaceChild( createTextNode( title ), documentElement().namedItem( "title" ).firstChild() );
}

void
XSPFPlaylist::setCreator( const QString &creator )
{
    if ( documentElement().namedItem( "creator" ).isNull() )
    {
        QDomNode node = createElement( "creator" );
        QDomNode subNode = createTextNode( creator );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
        documentElement().namedItem( "creator" ).replaceChild( createTextNode( creator ), documentElement().namedItem( "creator" ).firstChild() );
}

void
XSPFPlaylist::setAnnotation( const QString &annotation )
{
    if ( documentElement().namedItem( "annotation" ).isNull() )
    {
        QDomNode node = createElement( "annotation" );
        QDomNode subNode = createTextNode( annotation );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
        documentElement().namedItem( "annotation" ).replaceChild( createTextNode( annotation ), documentElement().namedItem( "annotation" ).firstChild() );
}

void
XSPFPlaylist::setInfo( const KUrl &info )
{
    if ( documentElement().namedItem( "info" ).isNull() )
    {
        QDomNode node = createElement( "info" );
        QDomNode subNode = createTextNode( info.url() );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
        documentElement().namedItem( "info" ).replaceChild( createTextNode( info.url() ), documentElement().namedItem( "info" ).firstChild() );
}

void
XSPFPlaylist::setLocation( const KUrl &location )
{
    if ( documentElement().namedItem( "location" ).isNull() )
    {
        QDomNode node = createElement( "location" );
        QDomNode subNode = createTextNode( location.url() );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
        documentElement().namedItem( "location" ).replaceChild( createTextNode( location.url() ), documentElement().namedItem( "location" ).firstChild() );
}

void
XSPFPlaylist::setIdentifier( const QString &identifier )
{
    if ( documentElement().namedItem( "identifier" ).isNull() )
    {
        QDomNode node = createElement( "identifier" );
        QDomNode subNode = createTextNode( identifier );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
        documentElement().namedItem( "identifier" ).replaceChild( createTextNode( identifier ), documentElement().namedItem( "identifier" ).firstChild() );
}

void
XSPFPlaylist::setImage( const KUrl &image )
{
    if ( documentElement().namedItem( "image" ).isNull() )
    {
        QDomNode node = createElement( "image" );
        QDomNode subNode = createTextNode( image.url() );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
        documentElement().namedItem( "image" ).replaceChild( createTextNode( image.url() ), documentElement().namedItem( "image" ).firstChild() );
}

void
XSPFPlaylist::setDate( const QDateTime &date )
{
    /* date needs timezone info to be compliant with the standard
    (ex. 2005-01-08T17:10:47-05:00 ) */

    if ( documentElement().namedItem( "date" ).isNull() )
    {
        QDomNode node = createElement( "date" );
        QDomNode subNode = createTextNode( date.toString( "yyyy-MM-ddThh:mm:ss" ) );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
        documentElement().namedItem( "date" ).replaceChild( createTextNode( date.toString( "yyyy-MM-ddThh:mm:ss" ) ), documentElement().namedItem( "date" ).firstChild() );
}

void
XSPFPlaylist::setLicense( const KUrl &license )
{
    if ( documentElement().namedItem( "license" ).isNull() )
    {
        QDomNode node = createElement( "license" );
        QDomNode subNode = createTextNode( license.url() );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
        documentElement().namedItem( "license" ).replaceChild( createTextNode( license.url() ), documentElement().namedItem( "license" ).firstChild() );
}

void
XSPFPlaylist::setAttribution( const KUrl &attribution, bool append )
{
    if ( documentElement().namedItem( "attribution" ).isNull() )
        documentElement().insertBefore( createElement( "attribution" ), documentElement().namedItem( "trackList" ) );

    if ( append )
    {
        QDomNode subNode = createElement( "location" );
        QDomNode subSubNode = createTextNode( attribution.url() );
        subNode.appendChild( subSubNode );
        documentElement().namedItem( "attribution" ).insertBefore( subNode, documentElement().namedItem( "attribution" ).firstChild() );
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
}

void
XSPFPlaylist::setLink( const KUrl &link )
{
    if ( documentElement().namedItem( "link" ).isNull() )
    {
        QDomNode node = createElement( "link" );
        QDomNode subNode = createTextNode( link.url() );
        node.appendChild( subNode );
        documentElement().insertBefore( node, documentElement().namedItem( "trackList" ) );
    }
    else
        documentElement().namedItem( "link" ).replaceChild( createTextNode( link.url() ), documentElement().namedItem( "link" ).firstChild() );
}

XSPFTrackList
XSPFPlaylist::trackList()
{
    DEBUG_BLOCK

    XSPFTrackList list;

    QDomNode trackList = documentElement().namedItem( "trackList" );
    QDomNode subNode = trackList.firstChild();
    QDomNode subSubNode;

    while ( !subNode.isNull() )
    {
        XSPFTrack track;
        subSubNode = subNode.firstChild();
        if ( subNode.nodeName() == "track" )
        {
            while ( !subSubNode.isNull() )
            {
                if ( subSubNode.nodeName() == "location" )
                    track.location = subSubNode.firstChild().nodeValue();
                else if ( subSubNode.nodeName() == "title" )
                    track.title = subSubNode.firstChild().nodeValue();
                else if ( subSubNode.nodeName() == "creator" )
                    track.creator = subSubNode.firstChild().nodeValue();
                else if ( subSubNode.nodeName() == "duration" )
                    track.duration = subSubNode.firstChild().nodeValue().toInt();
                else if ( subSubNode.nodeName() == "annotation" )
                    track.annotation = subSubNode.firstChild().nodeValue();
                else if ( subSubNode.nodeName() == "album" )
                    track.album = subSubNode.firstChild().nodeValue();
                else if ( subSubNode.nodeName() == "trackNum" )
                    track.trackNum = (uint)subSubNode.firstChild().nodeValue().toInt();
                else if ( subSubNode.nodeName() == "identifier" )
                    track.identifier = subSubNode.firstChild().nodeValue();
                else if ( subSubNode.nodeName() == "info" )
                    track.info = subSubNode.firstChild().nodeValue();
                else if ( subSubNode.nodeName() == "image" )
                    track.image = subSubNode.firstChild().nodeValue();
                else if ( subSubNode.nodeName() == "link" )
                    track.link = subSubNode.firstChild().nodeValue();

                subSubNode = subSubNode.nextSibling();
            }
        }
        list.append( track );
        subNode = subNode.nextSibling();
    }

    debug() << "returning " << list.size() << "tracks";
    return list;
}


//documentation of attributes from http://www.xspf.org/xspf-v1.html
void
XSPFPlaylist::setTrackList( Meta::TrackList trackList, bool append )
{
    DEBUG_BLOCK

    if ( documentElement().namedItem( "trackList" ).isNull() )
        documentElement().appendChild( createElement( "trackList" ) );

    QDomNode node = createElement( "trackList" );
    XSPFTrackList::iterator it;

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
        //QDomNode extension

        #define APPENDNODE( X, Y ) \
        { \
            X.appendChild( createTextNode( Y ) );    \
            subNode.appendChild( X ); \
        }

        if ( !track->playableUrl().url().isEmpty() )
            APPENDNODE( location, track->playableUrl().url() )
        else
            APPENDNODE( location, track->uidUrl() )

        APPENDNODE( identifier, track->uidUrl() )

        Meta::StreamInfoCapability *streamInfo = track->create<Meta::StreamInfoCapability>();
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
            if ( !track->name().isEmpty() )
                APPENDNODE(title, track->name() )
            if ( track->artist() && !track->artist()->name().isEmpty() )
                APPENDNODE(creator, track->artist()->name() );
        }
        if ( !track->comment().isEmpty() )
            APPENDNODE(annotation, track->comment() );
        if ( track->album() && !track->album()->name().isEmpty() )
            APPENDNODE( album, track->album()->name() );
        if ( track->trackNumber() > 0 )
            APPENDNODE( trackNum, QString::number( track->trackNumber() ) );
        if ( track->length() > 0 )
            APPENDNODE( duration, QString::number( track->length() * 1000 ) );
        node.appendChild( subNode );
    }
    #undef APPENDNODE

    if ( append )
    {
        while ( !node.isNull() )
        {
            documentElement().namedItem( "trackList" ).appendChild( node.firstChild() );
            node = node.nextSibling();
        }
    }
    else
        documentElement().replaceChild( node, documentElement().namedItem( "trackList" ) );
}

bool
XSPFPlaylist::hasCapabilityInterface( Capability::Type type ) const
{
    switch( type )
    {
        case Capability::EditablePlaylist: return true; break;
        default: return false; break;
    }
}

Capability*
XSPFPlaylist::createCapabilityInterface( Capability::Type type )
{
    switch( type )
    {
        case Capability::EditablePlaylist: return static_cast<EditablePlaylistCapability *>(this);
        default: return 0;
    }
}

bool
XSPFPlaylist::isWritable()
{
    return QFile( m_url.path() ).isWritable();
}

void
XSPFPlaylist::setName( const QString &name )
{
    setTitle( name );
    //TODO: notify observers
}

} //namespace Meta
