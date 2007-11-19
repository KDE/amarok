// Author: Max Howell (C) Copyright 2003-4
// Author: Mark Kretschmann (C) Copyright 2004
// Author: Nikolaj Hald Nielsen (C) Copyright 2007
// .ram file support from Kaffeine 0.5, Copyright (C) 2004 by Jürgen Kofler (GPL 2 or later)
// .asx file support added by Michael Seiwert Copyright (C) 2006
// .asx file support from Kaffeine, Copyright (C) 2004-2005 by Jürgen Kofler (GPL 2 or later)
// .smil file support from Kaffeine 0.7
// .pls parser (C) Copyright 2005 by Michael Buesch <mbuesch@freenet.de>
// .xspf file support added by Mattias Fliesberg <mattias.fliesberg@gmail.com> Copyright (C) 2006
// Copyright: See COPYING file that comes with this distribution
//




#define DEBUG_PREFIX "PlaylistHandler"

#include "amarokconfig.h"
#include "app.h"
#include "CollectionManager.h"
#include "MainWindow.h"
#include "meta/EditCapability.h"
#include "meta/proxy/MetaProxy.h"
#include "PlaylistHandler.h"
#include "playlist/PlaylistModel.h"
#include "statusbar.h"
#include "TheInstances.h"
#include "xspfplaylist.h"

#include <KMessageBox>
#include <KUrl>

#include <QFile>
#include <QtXml>


using namespace Meta;


PlaylistHandler::PlaylistHandler()
    : QObject( 0 )
    , m_format( Unknown )
{

}

bool PlaylistHandler::isPlaylist(const KUrl & path)
{
    return getFormat( path ) != Unknown;
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

    Amarok::StatusBar::instance() ->newProgressOperation( m_downloadJob )
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
    DEBUG_BLOCK


    TrackList tracks;
    TrackPtr currentTrack;

    // Counted number of "File#=" lines.
    unsigned int entryCnt = 0;
    // Value of the "NumberOfEntries=#" line.
    unsigned int numberOfEntries = 0;
    // Does the file have a "[playlist]" section? (as it's required by the standard)
    bool havePlaylistSection = false;
    QString tmp;
    QStringList lines;

    const QRegExp regExp_NumberOfEntries("^NumberOfEntries\\s*=\\s*\\d+$");
    const QRegExp regExp_File("^File\\d+\\s*=");
    const QRegExp regExp_Title("^Title\\d+\\s*=");
    const QRegExp regExp_Length("^Length\\d+\\s*=\\s*\\d+$");
    const QRegExp regExp_Version("^Version\\s*=\\s*\\d+$");
    const QString section_playlist("[playlist]");

    /* Preprocess the input data.
     * Read the lines into a buffer; Cleanup the line strings;
     * Count the entries manually and read "NumberOfEntries".
     */
    while (!stream.atEnd()) {
        tmp = stream.readLine();
        tmp = tmp.trimmed();
        if (tmp.isEmpty())
            continue;
        lines.append(tmp);

        if (tmp.contains(regExp_File)) {
            entryCnt++;
            continue;
        }
        if (tmp == section_playlist) {
            havePlaylistSection = true;
            continue;
        }
        if (tmp.contains(regExp_NumberOfEntries)) {
            numberOfEntries = tmp.section('=', -1).trimmed().toUInt();
            continue;
        }
    }
    if (numberOfEntries != entryCnt) {
        warning() << ".pls playlist: Invalid \"NumberOfEntries\" value.  "
                  << "NumberOfEntries=" << numberOfEntries << "  counted="
                  << entryCnt << endl;
        /* Corrupt file. The "NumberOfEntries" value is
         * not correct. Fix it by setting it to the manually
         * counted number and go on parsing.
         */
        numberOfEntries = entryCnt;
    }
    if (!numberOfEntries)
        return true;

    unsigned int index;
    bool ok = false;
    bool inPlaylistSection = false;

    /* Now iterate through all beautified lines in the buffer
     * and parse the playlist data.
     */
    QStringList::const_iterator i = lines.begin(), end = lines.end();
    for ( ; i != end; ++i) {
        if (!inPlaylistSection && havePlaylistSection) {
            /* The playlist begins with the "[playlist]" tag.
             * Skip everything before this.
             */
            if ((*i) == section_playlist)
                inPlaylistSection = true;
            continue;
        }
        if ((*i).contains(regExp_File)) {
            // Have a "File#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if (index > numberOfEntries || index == 0)
                continue;
            tmp = (*i).section('=', 1).trimmed();
            currentTrack = Meta::TrackPtr( new MetaProxy::Track( KUrl( tmp ) ) );
            tracks.append( currentTrack );
            continue;
        }
        if ((*i).contains(regExp_Title)) {
            // Have a "Title#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if (index > numberOfEntries || index == 0)
                continue;
            tmp = (*i).section('=', 1).trimmed();

            if ( currentTrack.data() != 0 && currentTrack->is<Meta::EditCapability>() )
            {
                Meta::EditCapability *ec = currentTrack->as<Meta::EditCapability>();
                if( ec )
                    ec->setTitle( tmp );
                delete ec;
            }
            continue;
        }
        if ((*i).contains(regExp_Length)) {
            // Have a "Length#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if (index > numberOfEntries || index == 0)
                continue;
            tmp = (*i).section('=', 1).trimmed();
            //tracks.append( KUrl(tmp) );
//             Q_ASSERT(ok);
            continue;
        }
        if ((*i).contains(regExp_NumberOfEntries)) {
            // Have the "NumberOfEntries=#" line.
            continue;
        }
        if ((*i).contains(regExp_Version)) {
            // Have the "Version=#" line.
            tmp = (*i).section('=', 1).trimmed();
            // We only support Version=2
            if (tmp.toUInt(&ok) != 2)
                warning() << ".pls playlist: Unsupported version." << endl;
//             Q_ASSERT(ok);
            continue;
        }
        warning() << ".pls playlist: Unrecognized line: \"" << *i << "\"" << endl;
    }

    debug() << QString( "inserting %1 tracks from playlist" ).arg( tracks.count() );
    The::playlistModel()->insertOptioned( tracks, Playlist::Append );

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

    KUrl::List urls;
    QStringList titles;
    QList<int> lengths;
    foreach( TrackPtr track, tracks )
    {
        urls << track->url();
        titles << track->name();
        lengths << track->length();
    }

    QTextStream stream( &file );
    stream << "[Playlist]\n";
    stream << "NumberOfEntries=" << tracks.count() << endl;
    for( int i = 1, n = urls.count(); i < n; ++i )
    {
        stream << "File" << i << "=";
        stream << urls[i].path();
        stream << "\nTitle" << i << "=";
        stream << titles[i];
        stream << "\nLength" << i << "=";
        stream << lengths[i];
        stream << "\n";
    }

    stream << "Version=2\n";
    file.close();
    return true;
}

unsigned int
PlaylistHandler::loadPls_extractIndex( const QString &str ) const
{
    /* Extract the index number out of a .pls line.
     * Example:
     *   loadPls_extractIndex("File2=foobar") == 2
     */
    bool ok = false;
    unsigned int ret;
    QString tmp(str.section('=', 0, 0));
    tmp.remove(QRegExp("^\\D*"));
    ret = tmp.trimmed().toUInt(&ok);
    Q_ASSERT(ok);
    return ret;
}

bool
PlaylistHandler::loadM3u( QTextStream &stream )
{
    DEBUG_BLOCK

    TrackList tracks;

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

    QTextStream stream( &file );
    stream << "#EXTM3U\n";

    KUrl::List urls;
    QStringList titles;
    QList<int> lengths;
    foreach( TrackPtr track, tracks )
    {
        urls << track->url();
        titles << track->name();
        lengths << track->length();
    }

    //Port 2.0 is this still necessary?
//     foreach( KUrl url, urls)
//     {
//         if( url.isLocalFile() && QFileInfo( url.path() ).isDir() )
//             urls += recurse( url );
//         else
//             urls += url;
//     }

    for( int i = 0, n = urls.count(); i < n; ++i )
    {
        const KUrl &url = urls[i];

        if( !titles.isEmpty() && !lengths.isEmpty() )
        {
            stream << "#EXTINF:";
            stream << QString::number( lengths[i] );
            stream << ',';
            stream << titles[i];
            stream << '\n';
        }
        if (url.protocol() == "file" ) {
            if ( relative ) {
                const QFileInfo fi(file);
                stream << KUrl::relativePath(fi.path(), url.path());
            } else
                stream << url.path();
        } else {
            stream << url.url();
        }
        stream << "\n";
    }

    file.close(); // Flushes the file, before we read it
//     PlaylistBrowser::instance()->addPlaylist( path, 0, true ); //Port 2.0: re add when we have a playlistbrowser

    return true;
}

bool
PlaylistHandler::loadRealAudioRam( QTextStream &stream )
{
    DEBUG_BLOCK

    TrackList tracks;
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

    TrackList tracks;

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
    TrackList tracks;
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
            TrackPtr trackPtr = Meta::TrackPtr( new MetaProxy::Track( KUrl( url ) ) );
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
    DEBUG_BLOCK
    XSPFPlaylist* doc = new XSPFPlaylist( stream );

    XSPFtrackList trackList = doc->trackList();

    TrackList tracks;
    foreach( const XSPFtrack &track, trackList )
    {
        KUrl location = track.location;
        QString artist = track.creator;
        debug() << "TRACK CREATOR IS: " << track.creator;
        QString title  = track.title;
        QString album  = track.album;


        if( location.isEmpty() || ( location.isLocalFile() && !QFile::exists( location.path() ) ) )
        {
            TrackPtr trackPtr = Meta::TrackPtr( new MetaProxy::Track( KUrl( location ) ) );
            tracks.append( trackPtr );
        }
        else
        {
            debug() << location << ' ' << artist << ' ' << title << ' ' << album;

            TrackPtr trackPtr = Meta::TrackPtr( new MetaProxy::Track( KUrl( location ) ) );
            Meta::EditCapability *ec = trackPtr->as<Meta::EditCapability>();
            if( ec )
            {
                debug() << "Have EditCapability";
                ec->setTitle( title );
                ec->setArtist( artist );
                ec->setAlbum( album );
                ec->setComment( track.annotation );
            }
            delete ec;
            tracks.append( trackPtr );

        }
    }

    delete doc;

    The::playlistModel()->insertOptioned( tracks, Playlist::Append );
    return true;
}

bool
PlaylistHandler::saveXSPF( Meta::TrackList tracks, const QString &location )
{
    if( tracks.isEmpty() )
        return false;
    XSPFPlaylist playlist;

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

#include <kdirlister.h>
#include <QEventLoop>
//this function (C) Copyright 2003-4 Max Howell, (C) Copyright 2004 Mark Kretschmann
KUrl::List
PlaylistHandler::recurse( const KUrl &url )
{
    typedef QMap<QString, KUrl> FileMap;

    KDirLister lister( false );
    lister.setAutoUpdate( false );
    lister.setAutoErrorHandlingEnabled( false, 0 );
    lister.openUrl( url );

    while( !lister.isFinished() )
        kapp->processEvents( QEventLoop::ExcludeUserInput );

    KFileItemList items = lister.items();
    KUrl::List urls;
    FileMap files;
    foreach( const KFileItem& it, items ) {
        if( it.isFile() ) { files[it.name()] = it.url(); continue; }
        if( it.isDir() ) urls += recurse( it.url() );
    }

    oldForeachType( FileMap, files )
        // users often have playlist files that reflect directories
        // higher up, or stuff in this directory. Don't add them as
        // it produces double entries
            if( !isPlaylist( (*it).fileName() ) )
            urls += *it;

    return urls;
}

#include "PlaylistHandler.moc"

