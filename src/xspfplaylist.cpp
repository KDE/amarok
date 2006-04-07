// Author: Mattias Fliesberg (C) Copyright 2006
// Copyright: See COPYING file that comes with this distribution
//

#include "xspfplaylist.h"
#include "debug.h"

XSPFPlaylist::XSPFPlaylist( )
{
    m_root = documentElement();

    m_root.setAttribute( "version", 1 );
    m_root.setAttribute( "xmlns", "http://xspf.org/ns/0/" );

    m_root.appendChild( createElement( "trackList" ) );
}

XSPFPlaylist::XSPFPlaylist( QTextStream &stream )
{
    loadXSPF( stream );
    m_root = documentElement();
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
    return m_root.namedItem( "title" ).firstChild().nodeValue();
}

QString
XSPFPlaylist::creator()
{
    return m_root.namedItem( "creator" ).firstChild().nodeValue();
}

QString
XSPFPlaylist::annotation()
{
    return m_root.namedItem( "annotation" ).firstChild().nodeValue();
}

KURL
XSPFPlaylist::info()
{
    return KURL::fromPathOrURL( m_root.namedItem( "info" ).firstChild().nodeValue() );
}

KURL
XSPFPlaylist::location()
{
    return KURL::fromPathOrURL( m_root.namedItem( "location" ).firstChild().nodeValue() );
}

QString
XSPFPlaylist::identifier()
{
    return m_root.namedItem( "identifier" ).firstChild().nodeValue();
}

KURL
XSPFPlaylist::image()
{
    return KURL::fromPathOrURL( m_root.namedItem( "image" ).firstChild().nodeValue() );
}

QDateTime
XSPFPlaylist::date()
{
    return QDateTime::fromString( m_root.namedItem( "date" ).firstChild().nodeValue(), Qt::ISODate );
}

KURL
XSPFPlaylist::license()
{
    return KURL::fromPathOrURL( m_root.namedItem( "license" ).firstChild().nodeValue() );
}

KURL::List
XSPFPlaylist::attribution()
{
    QDomNode node = m_root.namedItem( "attribution" );
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
    return KURL::fromPathOrURL( m_root.namedItem( "link" ).firstChild().nodeValue() );
}

void
XSPFPlaylist::title( QString title )
{
    if ( m_root.namedItem( "title" ).isNull() )
    {
        QDomNode node = createElement( "title" );
        QDomNode subNode = createTextNode( title );
        node.appendChild( subNode );
        m_root.insertBefore( node, m_root.namedItem( "trackList" ) );
    }
    else
        m_root.namedItem( "title" ).replaceChild( createTextNode( title ), m_root.namedItem( "title" ).firstChild() );
}

void
XSPFPlaylist::creator( QString creator )
{
    if ( m_root.namedItem( "creator" ).isNull() )
    {
        QDomNode node = createElement( "creator" );
        QDomNode subNode = createTextNode( creator );
        node.appendChild( subNode );
        m_root.insertBefore( node, m_root.namedItem( "trackList" ) );
    }
    else
        m_root.namedItem( "creator" ).replaceChild( createTextNode( creator ), m_root.namedItem( "creator" ).firstChild() );
}

void
XSPFPlaylist::annotation( QString annotation )
{
    if ( m_root.namedItem( "annotation" ).isNull() )
    {
        QDomNode node = createElement( "annotation" );
        QDomNode subNode = createTextNode( annotation );
        node.appendChild( subNode );
        m_root.insertBefore( node, m_root.namedItem( "trackList" ) );
    }
    else
        m_root.namedItem( "annotation" ).replaceChild( createTextNode( annotation ), m_root.namedItem( "annotation" ).firstChild() );
}

void
XSPFPlaylist::info( KURL info )
{
    if ( m_root.namedItem( "info" ).isNull() )
    {
        QDomNode node = createElement( "info" );
        QDomNode subNode = createTextNode( info.url() );
        node.appendChild( subNode );
        m_root.insertBefore( node, m_root.namedItem( "trackList" ) );
    }
    else
        m_root.namedItem( "info" ).replaceChild( createTextNode( info.url() ), m_root.namedItem( "info" ).firstChild() );
}

void
XSPFPlaylist::location( KURL location )
{
    if ( m_root.namedItem( "location" ).isNull() )
    {
        QDomNode node = createElement( "location" );
        QDomNode subNode = createTextNode( location.url() );
        node.appendChild( subNode );
        m_root.insertBefore( node, m_root.namedItem( "trackList" ) );
    }
    else
        m_root.namedItem( "location" ).replaceChild( createTextNode( location.url() ), m_root.namedItem( "location" ).firstChild() );
}

void
XSPFPlaylist::identifier( QString identifier )
{
    if ( m_root.namedItem( "identifier" ).isNull() )
    {
        QDomNode node = createElement( "identifier" );
        QDomNode subNode = createTextNode( identifier );
        node.appendChild( subNode );
        m_root.insertBefore( node, m_root.namedItem( "trackList" ) );
    }
    else
        m_root.namedItem( "identifier" ).replaceChild( createTextNode( identifier ), m_root.namedItem( "identifier" ).firstChild() );
}

void
XSPFPlaylist::image( KURL image )
{
    if ( m_root.namedItem( "image" ).isNull() )
    {
        QDomNode node = createElement( "image" );
        QDomNode subNode = createTextNode( image.url() );
        node.appendChild( subNode );
        m_root.insertBefore( node, m_root.namedItem( "trackList" ) );
    }
    else
        m_root.namedItem( "image" ).replaceChild( createTextNode( image.url() ), m_root.namedItem( "image" ).firstChild() );
}

void
XSPFPlaylist::date( QDateTime date )
{
    if ( m_root.namedItem( "date" ).isNull() )
    {
        QDomNode node = createElement( "date" );
        QDomNode subNode = createTextNode( date.toString( "yyyy-MM-ddThh:mm:ss" ) );
        node.appendChild( subNode );
        m_root.insertBefore( node, m_root.namedItem( "trackList" ) );
    }
    else
        m_root.namedItem( "date" ).replaceChild( createTextNode( date.toString( "yyyy-MM-ddThh:mm:ss" ) ), m_root.namedItem( "date" ).firstChild() );
}

void
XSPFPlaylist::license( KURL license )
{
    if ( m_root.namedItem( "license" ).isNull() )
    {
        QDomNode node = createElement( "license" );
        QDomNode subNode = createTextNode( license.url() );
        node.appendChild( subNode );
        m_root.insertBefore( node, m_root.namedItem( "trackList" ) );
    }
    else
        m_root.namedItem( "license" ).replaceChild( createTextNode( license.url() ), m_root.namedItem( "license" ).firstChild() );
}

void
XSPFPlaylist::attribution( KURL attribution, bool append )
{
    if ( m_root.namedItem( "attribution" ).isNull() )
        m_root.insertBefore( createElement( "attribution" ), m_root.namedItem( "trackList" ) );

    if ( append )
    {
        QDomNode subNode = createElement( "location" );
        QDomNode subSubNode = createTextNode( attribution.url() );
        subNode.appendChild( subSubNode );
        m_root.namedItem( "attribution" ).insertBefore( subNode, m_root.namedItem( "attribution" ).firstChild() );
    }
    else
    {
        QDomNode node = createElement( "attribution" );
        QDomNode subNode = createElement( "location" );
        QDomNode subSubNode = createTextNode( attribution.url() );
        subNode.appendChild( subSubNode );
        node.appendChild( subNode );
        m_root.replaceChild( node, m_root.namedItem( "attribution" ) );
    }
}

void
XSPFPlaylist::link( KURL link )
{
    if ( m_root.namedItem( "link" ).isNull() )
    {
        QDomNode node = createElement( "link" );
        QDomNode subNode = createTextNode( link.url() );
        node.appendChild( subNode );
        m_root.insertBefore( node, m_root.namedItem( "trackList" ) );
    }
    else
        m_root.namedItem( "link" ).replaceChild( createTextNode( link.url() ), m_root.namedItem( "link" ).firstChild() );
}


XSPFtrackList
XSPFPlaylist::trackList()
{
    XSPFtrackList list;

    QDomNode trackList = m_root.namedItem( "trackList" );
    QDomNode subNode = trackList.firstChild();
    QDomNode subSubNode;

    while ( !subNode.isNull() )
    {
        subSubNode = subNode.firstChild();
        if ( subNode.nodeName() == "track" )
        {
            while ( !subSubNode.isNull() )
            {
                XSPFtrack track;

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
                else if ( subSubNode.nodeName() == "tracknum" )
                    track.trackNum = (uint)subSubNode.firstChild().nodeValue().toInt();
                else if ( subSubNode.nodeName() == "identifier" )
                    track.identifier = subSubNode.firstChild().nodeValue();
                else if ( subSubNode.nodeName() == "info" )
                    track.info = subSubNode.firstChild().nodeValue();
                else if ( subSubNode.nodeName() == "image" )
                    track.image = subSubNode.firstChild().nodeValue();
                else if ( subSubNode.nodeName() == "link" )
                    track.link = subSubNode.firstChild().nodeValue();

                list.append(track);

                subSubNode = subSubNode.nextSibling();
            }
        }
        subNode = subNode.nextSibling();
    }

    return list;
}

void
XSPFPlaylist::trackList( XSPFtrackList trackList, bool append )
{

    if ( m_root.namedItem( "trackList" ).isNull() )
        m_root.appendChild( createElement( "trackList" ) );

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

        subNode.appendChild( location.appendChild( createTextNode( track.location.url() ) ) );

        node.appendChild( subNode );
    }

    if ( append )
    {
        while ( !node.isNull() )
        {
            m_root.namedItem( "trackList" ).appendChild( node.firstChild() );
            node = node.nextSibling();
        }
    }
    else
        m_root.replaceChild( node, m_root.namedItem( "trackList" ) );
}





