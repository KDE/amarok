// Author: Mattias Fliesberg (C) Copyright 2006
// Copyright: See COPYING file that comes with this distribution
//

#include "atomicstring.h"
#include "xspfplaylist.h"
#include "debug.h"

XSPFPlaylist::XSPFPlaylist( )
{
    QDomElement root = createElement( "playlist" );

    root.setAttribute( "version", 1 );
    root.setAttribute( "xmlns", "http://xspf.org/ns/0/" );

    root.appendChild( createElement( "trackList" ) );

    appendChild( root );
}

XSPFPlaylist::XSPFPlaylist( QTextStream &stream )
{
    loadXSPF( stream );
}

bool
XSPFPlaylist::loadXSPF( QTextStream &stream )
{
    QString errorMsg;
    int errorLine, errorColumn;
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    if (!setContent(stream.read(), &errorMsg, &errorLine, &errorColumn))
    {
        debug() << "[XSPFPlaylist]: Error loading xml file: " "(" << errorMsg << ")"
                << " at line " << errorLine << ", column " << errorColumn << endl;
        return false;
    }

    return true;
}

QString
XSPFPlaylist::title()
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

KURL
XSPFPlaylist::info()
{
    return KURL::fromPathOrURL( documentElement().namedItem( "info" ).firstChild().nodeValue() );
}

KURL
XSPFPlaylist::location()
{
    return KURL::fromPathOrURL( documentElement().namedItem( "location" ).firstChild().nodeValue() );
}

QString
XSPFPlaylist::identifier()
{
    return documentElement().namedItem( "identifier" ).firstChild().nodeValue();
}

KURL
XSPFPlaylist::image()
{
    return KURL::fromPathOrURL( documentElement().namedItem( "image" ).firstChild().nodeValue() );
}

QDateTime
XSPFPlaylist::date()
{
    return QDateTime::fromString( documentElement().namedItem( "date" ).firstChild().nodeValue(), Qt::ISODate );
}

KURL
XSPFPlaylist::license()
{
    return KURL::fromPathOrURL( documentElement().namedItem( "license" ).firstChild().nodeValue() );
}

KURL::List
XSPFPlaylist::attribution()
{
    QDomNode node = documentElement().namedItem( "attribution" );
    KURL::List list;

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

KURL
XSPFPlaylist::link()
{
    return KURL::fromPathOrURL( documentElement().namedItem( "link" ).firstChild().nodeValue() );
}

void
XSPFPlaylist::setTitle( QString title )
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
XSPFPlaylist::setCreator( QString creator )
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
XSPFPlaylist::setAnnotation( QString annotation )
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
XSPFPlaylist::setInfo( KURL info )
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
XSPFPlaylist::setLocation( KURL location )
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
XSPFPlaylist::setIdentifier( QString identifier )
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
XSPFPlaylist::setImage( KURL image )
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
XSPFPlaylist::setDate( QDateTime date )
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
XSPFPlaylist::setLicense( KURL license )
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
XSPFPlaylist::setAttribution( KURL attribution, bool append )
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
XSPFPlaylist::setLink( KURL link )
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


XSPFtrackList
XSPFPlaylist::trackList()
{
    XSPFtrackList list;

    QDomNode trackList = documentElement().namedItem( "trackList" );
    QDomNode subNode = trackList.firstChild();
    QDomNode subSubNode;

    while ( !subNode.isNull() )
    {
        XSPFtrack track;
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

    return list;
}

void
XSPFPlaylist::setTrackList( XSPFtrackList trackList, bool append )
{
    if ( documentElement().namedItem( "trackList" ).isNull() )
        documentElement().appendChild( createElement( "trackList" ) );

    QDomNode node = createElement( "trackList" );

    XSPFtrackList::iterator it;

    for ( it = trackList.begin(); it != trackList.end(); ++it )
    {
        XSPFtrack track = (*it);

        QDomNode subNode = createElement( "track" );

        QDomNode location = createElement( "location" );
        QDomNode identifier = createElement( "identifier" );
        QDomNode title = createElement( "title" );
        QDomNode creator = createElement( "creator" );
        QDomNode annotation = createElement( "annotation" );
        QDomNode info = createElement( "info" );
        QDomNode image = createElement( "image" );
        QDomNode album = createElement( "album" );
        QDomNode trackNum = createElement( "trackNum" );
        QDomNode duration = createElement( "duration" );
        QDomNode link = createElement( "link" );
        // QDomNode meta
        // QDomNode extension

        if ( !track.location.url().isNull() )
            location.appendChild( createTextNode( track.location.url() ) );
        if ( !track.identifier.isNull() )
            identifier.appendChild( createTextNode( track.identifier ) );
        if ( !track.title.isNull() )
            title.appendChild( createTextNode( track.title ) );
        if ( !track.creator.isNull() )
            creator.appendChild( createTextNode( track.creator ) );
        if ( !track.annotation.isNull() )
            annotation.appendChild( createTextNode( track.annotation ) );
        if ( !track.info.url().isNull() )
            info.appendChild( createTextNode( track.info.url() ) );
        if ( !track.image.url().isNull() )
            image.appendChild( createTextNode( track.image.url() ) );
        if ( !track.album.isNull() )
            album.appendChild( createTextNode( track.album ) );
        if ( track.trackNum > 0 )
            trackNum.appendChild( createTextNode( QString::number( track.trackNum ) ) );
        if ( track.duration > 0 )
            duration.appendChild( createTextNode( QString::number( track.duration ) ) );
        if ( !track.link.url().isNull() )
            link.appendChild( createTextNode( track.link.url() ) );


        if ( !location.firstChild().isNull() )
            subNode.appendChild( location );
        if ( !identifier.firstChild().isNull() )
            subNode.appendChild( identifier );
        if ( !title.firstChild().isNull() )
            subNode.appendChild( title );
        if ( !creator.firstChild().isNull() )
            subNode.appendChild( creator );
        if ( !annotation.firstChild().isNull() )
            subNode.appendChild( annotation );
        if ( !info.firstChild().isNull() )
            subNode.appendChild( info );
        if ( !image.firstChild().isNull() )
            subNode.appendChild( image );
        if ( !album.firstChild().isNull() )
            subNode.appendChild( album );
        if ( !trackNum.firstChild().isNull() )
            subNode.appendChild( trackNum );
        if ( !duration.firstChild().isNull() )
            subNode.appendChild( duration );
        if ( !link.firstChild().isNull() )
            subNode.appendChild( link );

        node.appendChild( subNode );
    }

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





