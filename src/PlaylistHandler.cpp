// Author: Max Howell (C) Copyright 2003-4
// Author: Mark Kretschmann (C) Copyright 2004
// Author: Nikolaj Hald Nielsen (C) Copyright 2007
// Author: Casey Link (C) Copyright 2007
// .ram file support from Kaffeine 0.5, Copyright (C) 2004 by Jürgen Kofler (GPL 2 or later)
// .asx file support added by Michael Seiwert Copyright (C) 2006
// .asx file support from Kaffeine, Copyright (C) 2004-2005 by Jürgen Kofler (GPL 2 or later)
// .smil file support from Kaffeine 0.7
// .pls parser (C) Copyright 2005 by Michael Buesch <mbuesch@freenet.de>
// .xspf file support added by Mattias Fliesberg <mattias.fliesberg@gmail.com> Copyright (C) 2006
//
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/



#define DEBUG_PREFIX "PlaylistHandler"

#include "amarokconfig.h"
#include "amarok.h"
#include "app.h"
#include "CollectionManager.h"
#include "MainWindow.h"
#include "meta/EditCapability.h"
#include "meta/proxy/MetaProxy.h"
#include "PlaylistHandler.h"
#include "PlaylistManager.h"
#include "playlist/PlaylistModel.h"
#include "ContextStatusBar.h"
#include "TheInstances.h"
#include "meta/XSPFPlaylist.h"
#include "meta/M3UPlaylist.h"
#include "meta/PLSPlaylist.h"

#include <kdirlister.h>
#include <KMessageBox>
#include <KUrl>

#include <QEventLoop>
#include <QFile>
#include <QtXml>

PlaylistHandler::PlaylistHandler()
    : QObject( 0 )
    , m_format( Unknown )
{

}

void PlaylistHandler::load(const QString & path)
{
    DEBUG_BLOCK
    debug() << "file: " << path;

    //check if file is local or remote
    KUrl url( path );
    m_path = url.path();
    m_format = getFormat( url );

    if ( url.isLocalFile() ) {

        QFile file( url.path() );
        if( !file.open( QIODevice::ReadOnly ) ) {
            debug() << "cannot open file";
            return;
        }

        m_contents = QString( file.readAll() );
        file.close();

        QTextStream stream;
        stream.setString( &m_contents );
        handleByFormat( stream, m_format);

    } else {
        downloadPlaylist( url );
    }

}

bool PlaylistHandler::save( Meta::TrackList tracks,
                            const QString &location )
{
    KUrl url( location );
    Format playlistFormat = getFormat( url );
    switch (playlistFormat) {
        case M3U:
            return saveM3u( tracks, location );
            break;
        case PLS:
            return savePls( tracks, location );
            break;
        case XSPF:
            return saveXSPF( tracks, location );
            break;
        default:
            debug() << "Currently unhandled type!";
            return false;
            break;
    }
    return false;
}


PlaylistHandler::Format PlaylistHandler::getFormat( const KUrl &path )
{

    const QString ext = Amarok::extension( path.fileName() );

    if( ext == "m3u" ) return M3U;
    if( ext == "pls" ) return PLS;
    if( ext == "ram" ) return RAM;
    if( ext == "smil") return SMIL;
    if( ext == "asx" || ext == "wax" ) return ASX;
    if( ext == "xml" ) return XML;
    if( ext == "xspf" ) return XSPF;

    return Unknown;
}


void PlaylistHandler::handleByFormat( QTextStream &stream, Format format)
{
    DEBUG_BLOCK

    switch( format ) {

        case PLS:
            loadPls( stream );
            break;
        case M3U:
            loadM3u( stream );
            break;
        case RAM:
            loadRealAudioRam( stream );
            break;
       case ASX:
            loadASX( stream );
            break;
        case SMIL:
            loadSMIL( stream );
            break;
        case XSPF:
            loadXSPF( stream );
        break;

        default:
            debug() << "unknown type!";
            break;

    }

}


void PlaylistHandler::downloadPlaylist(const KUrl & path)
{
    DEBUG_BLOCK
    m_downloadJob =  KIO::storedGet( path );

    connect( m_downloadJob, SIGNAL( result( KJob * ) ),
        this, SLOT( downloadComplete( KJob * ) ) );

    Amarok::ContextStatusBar::instance() ->newProgressOperation( m_downloadJob )
    .setDescription( i18n( "Downloading Playlist" ) );

}

void PlaylistHandler::downloadComplete(KJob * job)
{
    DEBUG_BLOCK

    if ( !m_downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }

        m_contents =  m_downloadJob->data();
        QTextStream stream;
        stream.setString( &m_contents );


    handleByFormat( stream, m_format );

    m_downloadJob->deleteLater();

}


bool
PlaylistHandler::loadPls( QTextStream &stream )
{

    Meta::PLSPlaylist playlist = Meta::PLSPlaylist( stream );
    The::playlistModel()->insertOptioned( playlist.tracks(), Playlist::Append );

    return true;
}

bool
PlaylistHandler::savePls( Meta::TrackList tracks, const QString &location )
{
    QFile file( location );

    if( !file.open( QIODevice::WriteOnly ) )
    {
        KMessageBox::sorry( MainWindow::self(), i18n( "Cannot write playlist (%1).").arg(location) );
        return false;
    }

    Meta::PLSPlaylist playlist( tracks );
    playlist.save( file, false );

    file.close();
    return true;
}

bool
PlaylistHandler::loadM3u( QTextStream &stream )
{
    DEBUG_BLOCK

    Meta::TrackList tracks;

    const QString directory = m_path.left( m_path.lastIndexOf( '/' ) + 1 );

    for( QString line; !stream.atEnd(); )
    {
        line = stream.readLine();

        if( line.startsWith( "#EXTINF" ) ) {
            const QString extinf = line.section( ':', 1 );
            const int length = extinf.section( ',', 0, 0 ).toInt();
            //b.setTitle( extinf.section( ',', 1 ) );
            //b.setLength( length <= 0 ? /*MetaBundle::Undetermined HACK*/ -2 : length );
        }

        else if( !line.startsWith( "#" ) && !line.isEmpty() )
        {
            // KUrl::isRelativeUrl() expects absolute URLs to start with a protocol, so prepend it if missing
            QString url = line;
            if( url.startsWith( "/" ) )
                url.prepend( "file://" );

            if( KUrl::isRelativeUrl( url ) ) {
                KUrl kurl( KUrl( directory + line ) );
                kurl.cleanPath();
                debug() << "found track: " << kurl.path();
                tracks.append( Meta::TrackPtr( new MetaProxy::Track( kurl ) ) );
            }
            else {
                tracks.append( Meta::TrackPtr( new MetaProxy::Track( KUrl( line ) ) ) );
                debug() << "found track: " << line;
            }

            // Ensure that we always have a title: use the URL as fallback
            //if( b.title().isEmpty() )
            //    b.setTitle( url );

        }
    }

    debug() << QString( "inserting %1 tracks from playlist" ).arg( tracks.count() );
    The::playlistModel()->insertOptioned( tracks, Playlist::Append );


    return true;
}

bool
PlaylistHandler::saveM3u( Meta::TrackList tracks, const QString &location )
{
    const bool relative = AmarokConfig::relativePlaylist();
    if( location.isEmpty() )
        return false;

    QFile file( location );

    if( !file.open( QIODevice::WriteOnly ) )
    {
        KMessageBox::sorry( MainWindow::self(), i18n( "Cannot write playlist (%1).").arg(location) );
        return false;
    }


    Meta::M3UPlaylist playlist( tracks );

    playlist.save( file, relative );

    file.close(); // Flushes the file, before we read it
//     The::userPlaylistProvider->addPlaylist( path, 0, true ); //Port 2.0: re add when we have a playlistbrowser

    return true;
}

bool
PlaylistHandler::loadRealAudioRam( QTextStream &stream )
{
    DEBUG_BLOCK

    Meta::TrackList tracks;
    QString url;
    //while loop adapted from Kaffeine 0.5
    while (!stream.atEnd())
    {
        url = stream.readLine();
        if (url[0] == '#') continue; /* ignore comments */
        if (url == "--stop--") break; /* stop line */
        if ((url.left(7) == "rtsp://") || (url.left(6) == "pnm://") || (url.left(7) == "http://"))
        {
            tracks.append( Meta::TrackPtr( new MetaProxy::Track( KUrl( url ) ) ) );
        }
    }

    The::playlistModel()->insertOptioned( tracks, Playlist::Append );
    return true;
}

bool
PlaylistHandler::loadSMIL( QTextStream &stream )
{
     // adapted from Kaffeine 0.7
    QDomDocument doc;
    if( !doc.setContent( stream.readAll() ) )
    {
        debug() << "Could now read smil playlist";
        return false;
    }

    QDomElement root = doc.documentElement();
    stream.setAutoDetectUnicode( true );
    stream.setCodec( QTextCodec::codecForName( "UTF-8" ) );

    if( root.nodeName().toLower() != "smil" )
        return false;

    KUrl kurl;
    QString url;
    QDomNodeList nodeList;
    QDomNode node;
    QDomElement element;

    Meta::TrackList tracks;

    //audio sources...
    nodeList = doc.elementsByTagName( "audio" );
    for( uint i = 0; i < nodeList.count(); i++ )
    {
            node = nodeList.item(i);
            url.clear();
            if( (node.nodeName().toLower() == "audio") && (node.isElement()) )
            {
                    element = node.toElement();
                    if( element.hasAttribute("src") )
                            url = element.attribute("src");

                    else if( element.hasAttribute("Src") )
                            url = element.attribute("Src");

                    else if( element.hasAttribute("SRC") )
                            url = element.attribute("SRC");
            }
            if( !url.isNull() )
            {
                tracks.append( Meta::TrackPtr( new MetaProxy::Track( KUrl( url ) ) ) );
            }
    }

    The::playlistModel()->insertOptioned( tracks, Playlist::Append );
    return true;
}


bool
PlaylistHandler::loadASX( QTextStream &stream )
{
    //adapted from Kaffeine 0.7
    Meta::TrackList tracks;
    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;
    stream.setCodec( "UTF8" );

    QString content = stream.readAll();

    //ASX looks a lot like xml, but doesn't require tags to be case sensitive,
    //meaning we have to accept things like: <Abstract>...</abstract>
    //We use a dirty way to achieve this: we make all tags lower case
    QRegExp ex("(<[/]?[^>]*[A-Z]+[^>]*>)");
    ex.setCaseSensitivity( Qt::CaseSensitive );
    while ( (ex.indexIn(content)) != -1 )
        content.replace(ex.cap( 1 ), ex.cap( 1 ).toLower());


    if (!doc.setContent(content, &errorMsg, &errorLine, &errorColumn))
    {
        debug() << "Error loading xml file: " "(" << errorMsg << ")"
                << " at line " << errorLine << ", column " << errorColumn << endl;
        return false;
    }

    QDomElement root = doc.documentElement();

    QString url;
    QString title;
    QString author;
    QTime length;
    QString duration;

    if (root.nodeName().toLower() != "asx") return false;

    QDomNode node = root.firstChild();
    QDomNode subNode;
    QDomElement element;

    while (!node.isNull())
    {
    url.clear();
    title.clear();
    author.clear();
    length = QTime();
    if (node.nodeName().toLower() == "entry")
    {
        subNode = node.firstChild();
        while (!subNode.isNull())
        {
            if ((subNode.nodeName().toLower() == "ref") && (subNode.isElement()) && (url.isNull()))
            {
                element = subNode.toElement();
                if (element.hasAttribute("href"))
                url = element.attribute("href");
                if (element.hasAttribute("HREF"))
                url = element.attribute("HREF");
                if (element.hasAttribute("Href"))
                url = element.attribute("Href");
                if (element.hasAttribute("HRef"))
                url = element.attribute("HRef");
            }
                if ((subNode.nodeName().toLower() == "duration") && (subNode.isElement()))
                {
                duration.clear();
                element = subNode.toElement();
                if (element.hasAttribute("value"))
                    duration = element.attribute("value");
                if (element.hasAttribute("Value"))
                    duration = element.attribute("Value");
                if (element.hasAttribute("VALUE"))
                    duration = element.attribute("VALUE");

                if (!duration.isNull())
                    length = stringToTime(duration);
                }

                if ((subNode.nodeName().toLower() == "title") && (subNode.isElement()))
                {
                    title = subNode.toElement().text();
                }
                if ((subNode.nodeName().toLower() == "author") && (subNode.isElement()))
                {
                    author = subNode.toElement().text();
                }
                subNode = subNode.nextSibling();
        }
        if (!url.isEmpty())
        {
            Meta::TrackPtr trackPtr = Meta::TrackPtr( new MetaProxy::Track( KUrl( url ) ) );
            Meta::EditCapability *ec = trackPtr->as<Meta::EditCapability>();
            if( ec )
                ec->setTitle( title );
            delete ec;
            tracks.append( trackPtr );
        }
        }
        node = node.nextSibling();
    }

    The::playlistModel()->insertOptioned( tracks, Playlist::Append );
    return true;
}

QTime PlaylistHandler::stringToTime(const QString& timeString)
{
   int sec = 0;
   bool ok = false;
   QStringList tokens = timeString.split( ':' );

   sec += tokens[0].toInt(&ok)*3600; //hours
   sec += tokens[1].toInt(&ok)*60; //minutes
   sec += tokens[2].toInt(&ok); //secs

   if (ok)
      return QTime().addSecs(sec);
         else
            return QTime();
}

bool
PlaylistHandler::loadXSPF( QTextStream &stream )
{
//     DEBUG_BLOCK
    Meta::XSPFPlaylistPtr playlist = Meta::XSPFPlaylistPtr( new Meta::XSPFPlaylist( stream ) );

    Meta::TrackList tracks = playlist->tracks();

    The::playlistModel()->insertOptioned( tracks, Playlist::Append );
    return true;
}

bool
PlaylistHandler::saveXSPF( Meta::TrackList tracks, const QString &location )
{
    if( tracks.isEmpty() )
        return false;
    Meta::XSPFPlaylist playlist;

    playlist.setCreator( "Amarok" );
    playlist.setTitle( tracks[0]->artist()->name() );

    playlist.setTrackList( tracks );

    QFile file( location );
    if( !file.open( QIODevice::WriteOnly ) )
    {
        KMessageBox::sorry( MainWindow::self(), i18n( "Cannot write playlist (%1).").arg(location) );
        return false;
    }

    QTextStream stream ( &file );

    playlist.save( stream, 2 );

    file.close();

    return true;
}

KUrl::List
recurse( const KUrl &url )
{
    return Amarok::recursiveUrlExpand( url );
}

namespace Amarok
{

    //this function (C) Copyright 2003-4 Max Howell, (C) Copyright 2004 Mark Kretschmann
    KUrl::List
    recursiveUrlExpand ( const KUrl &url )
    {
        typedef QMap<QString, KUrl> FileMap;

        KDirLister lister ( false );
        lister.setAutoUpdate ( false );
        lister.setAutoErrorHandlingEnabled ( false, 0 );
        lister.openUrl ( url );

        while ( !lister.isFinished() )
            kapp->processEvents ( QEventLoop::ExcludeUserInput );

        KFileItemList items = lister.items();
        KUrl::List urls;
        FileMap files;
        foreach ( const KFileItem& it, items )
        {
            if ( it.isFile() ) { files[it.name() ] = it.url(); continue; }
            if ( it.isDir() ) urls += recurse ( it.url() );
        }

        oldForeachType ( FileMap, files )
        // users often have playlist files that reflect directories
        // higher up, or stuff in this directory. Don't add them as
        // it produces double entries
        if ( !PlaylistManager::isPlaylist( ( *it ).fileName() ) )
            urls += *it;
        return urls;
    }

        KUrl::List
    recursiveUrlExpand ( const KUrl::List &list )
    {
        KUrl::List urls;
        oldForeachType ( KUrl::List, list )
        {
            urls += recursiveUrlExpand ( *it );
        }

        return urls;
    }
}
#include "PlaylistHandler.moc"

