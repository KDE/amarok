/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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

#include "ASXPlaylist.h"

#include "core/capabilities/StreamInfoCapability.h"
#include "core/support/Debug.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "core-impl/playlists/types/file/xspf/XSPFPlaylist.h"

#include <QUrl>

#include <QFile>
#include <QString>

using namespace Playlists;

ASXPlaylist::ASXPlaylist( const QUrl &url, PlaylistProvider *provider )
    : PlaylistFile( url, provider )
    , QDomDocument()
{
}

void
ASXPlaylist::savePlaylist( QFile &file )
{
    QTextStream stream( &file );
    stream.setCodec( "UTF-8" );
    writeTrackList();
    QDomDocument::save( stream, 2 /*indent*/, QDomNode::EncodingFromTextStream );
}

bool
ASXPlaylist::processContent( QTextStream &stream )
{
    QString errorMsg;
    int errorLine, errorColumn;

    QString data = stream.readAll();

    // ASX looks a lot like xml, but doesn't require tags to be case sensitive,
    // meaning we have to accept things like: <Abstract>...</abstract>
    // We use a dirty way to achieve this: we make all tags lower case
    QRegularExpression tagPattern( QLatin1String("(<[/]?[^>]*[A-Z]+[^>]*>)"), QRegularExpression::CaseInsensitiveOption );
    QRegularExpression urlPattern( QLatin1String("(href\\s*=\\s*\")([^\"]+)\""), QRegularExpression::CaseInsensitiveOption );

    int index = 0;
    while ( ( index = data.indexOf( tagPattern, index ) ) != -1 )
    {
        QRegularExpressionMatch tagMatch = tagPattern.match( data, index );
        QString original = tagMatch.captured( 1 ).toLocal8Bit();
        QString tagReplacement = tagMatch.captured( 1 ).toLower().toLocal8Bit();
        if ( original.indexOf( urlPattern, 0 ) != -1  )
        {
            QRegularExpressionMatch urlMatch = urlPattern.match( original, 0 );
            // Some playlists have unescaped & characters in URLs
            QString url = urlMatch.captured( 2 );
            url.replace( QRegularExpression( QLatin1String("&(?!amp;|quot;|apos;|lt;|gt;)") ), QStringLiteral("&amp;") );

            QString urlReplacement = urlMatch.captured( 1 ) % url % "\"";
            tagReplacement.replace( urlMatch.captured(0).toLocal8Bit().toLower(),
                                    urlReplacement.toLocal8Bit() );
        }
        data.replace( original, tagReplacement );
        index += tagMatch.capturedLength();
    }
    if( !setContent( data, &errorMsg, &errorLine, &errorColumn ) )
    {
        error() << "Error loading xml file: " "(" << errorMsg << ")"
                << " at line " << errorLine << ", column " << errorColumn;
        m_tracksLoaded = false;
    }
    else
        m_tracksLoaded = true;

    return m_tracksLoaded;
}

bool
ASXPlaylist::loadAsx( QTextStream &stream )
{
    if ( !processContent( stream ) )
        return false;

    QDomNode asx = documentElement();
    QDomNode subNode = asx.firstChild();
    QDomNode subSubNode;
    while( !subNode.isNull() )
    {
        XSPFTrack track;
        subSubNode = subNode.firstChild();
        if( subNode.nodeName() == QLatin1String("entry") )
        {
            while( !subSubNode.isNull() )
            {
                if( subSubNode.nodeName() == QLatin1String("ref") )
                {
                    QByteArray path = subSubNode.attributes().namedItem(QStringLiteral("href")).nodeValue().toUtf8();
                    path.replace( '\\', '/' );

                    QUrl url = getAbsolutePath( QUrl::fromEncoded( path ) );
                    track.location = url;
                }
                else if( subSubNode.nodeName() == QLatin1String("title") )
                     track.title = subSubNode.firstChild().nodeValue();
                else if( subSubNode.nodeName() == QLatin1String("author") )
                     track.creator = subSubNode.firstChild().nodeValue();

                subSubNode = subSubNode.nextSibling();
            }
        }
        MetaProxy::Track *proxyTrack = new MetaProxy::Track( track.location );
        proxyTrack->setTitle( track.title );
        proxyTrack->setArtist( track.creator );
        proxyTrack->setLength( track.duration );
        m_tracks << Meta::TrackPtr( proxyTrack );
        subNode = subNode.nextSibling();
    }
    return true;
}

void
ASXPlaylist::writeTrackList( )
{
    Meta::TrackList trackList = tracks();

    if ( documentElement().namedItem( QStringLiteral("asx") ).isNull() )
    {
        QDomElement root = createElement( QStringLiteral("asx") );
        root.setAttribute( QStringLiteral("version"), 3.0 );
        appendChild( root );
    }

    foreach( Meta::TrackPtr track, trackList )
    {
        QDomNode subNode = createElement( QStringLiteral("entry") );

        //URI of resource to be rendered.
        QDomElement location = createElement( QStringLiteral("ref") );

        //Track title
        QDomNode title = createElement( QStringLiteral("title") );

        //Human-readable name of the entity that authored the resource.
        QDomNode creator = createElement( QStringLiteral("author") );

        //Description of a track
        QDomNode abstract = createElement( QStringLiteral("abstract") );

        location.setAttribute( QStringLiteral("href"), trackLocation( track ) );
        subNode.appendChild( location );

        #define APPENDNODE( X, Y ) \
        { \
            X.appendChild( createTextNode( Y ) );    \
            subNode.appendChild( X ); \
        }

        Capabilities::StreamInfoCapability *streamInfo = track->create<Capabilities::StreamInfoCapability>();
        if( streamInfo ) // We have a stream, use it's metadata instead of the tracks.
        {
            if( !streamInfo->streamName().isEmpty() )
                APPENDNODE( title, streamInfo->streamName() );
            if( !streamInfo->streamSource().isEmpty() )
                APPENDNODE( creator, streamInfo->streamSource() );

            delete streamInfo;
        }
        else
        {
            if( !track->name().isEmpty() )
                APPENDNODE( title, track->name() );
            if( track->artist() && !track->artist()->name().isEmpty() )
                APPENDNODE( creator, track->artist()->name() );
        }
        if( !track->comment().isEmpty() )
            APPENDNODE(abstract, track->comment() );
        #undef APPENDNODE
        documentElement().appendChild( subNode );
    }
}
