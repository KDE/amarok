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




#define DEBUG_PREFIX "PlaylistLoader"

#include "CollectionManager.h"
#include "PlaylistLoader.h"
#include "PlaylistModel.h"
#include "statusbar.h"
#include "TheInstances.h"

#include <KUrl>

#include <QFile>


using namespace Meta;


PlaylistLoader::PlaylistLoader()
    : QObject( 0 )
    , m_format( Unknown )
{

}

bool PlaylistLoader::isPlaylist(const KUrl & path)
{
    DEBUG_BLOCK;
    return getFormat( path ) != Unknown;
}

void PlaylistLoader::load(const QString & path)
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


PlaylistLoader::Format PlaylistLoader::getFormat( const KUrl &path ) 
{
    DEBUG_BLOCK

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


void PlaylistLoader::handleByFormat( QTextStream &stream, Format format)
{
    DEBUG_BLOCK

    switch( format ) {

        case PLS:
            loadPls( stream );
            break;
        case M3U:
            loadM3u( stream );
            break;
        default:
            debug() << "unknown type!";
            break;

    }

}


void PlaylistLoader::downloadPlaylist(const KUrl & path)
{
    DEBUG_BLOCK
    m_downloadJob =  KIO::storedGet( path, false, true );

    connect( m_downloadJob, SIGNAL( result( KJob * ) ),
        this, SLOT( downloadComplete( KJob * ) ) );

    Amarok::StatusBar::instance() ->newProgressOperation( m_downloadJob )
    .setDescription( i18n( "Downloading Playlist" ) );

}

void PlaylistLoader::downloadComplete(KJob * job)
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
PlaylistLoader::loadPls( QTextStream &stream )
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
            currentTrack = CollectionManager::instance()->trackForUrl( KUrl(tmp) );
            tracks.append( currentTrack );
            continue;
        }
        if ((*i).contains(regExp_Title)) {
            // Have a "Title#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if (index > numberOfEntries || index == 0)
                continue;
            tmp = (*i).section('=', 1).trimmed();
            
            if ( currentTrack.data() != 0 )
                currentTrack->setTitle( tmp );
            continue;
        }
        if ((*i).contains(regExp_Length)) {
            // Have a "Length#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if (index > numberOfEntries || index == 0)
                continue;
            tmp = (*i).section('=', 1).trimmed();
            //tracks.append( KUrl(tmp) );
            Q_ASSERT(ok);
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
            Q_ASSERT(ok);
            continue;
        }
        warning() << ".pls playlist: Unrecognized line: \"" << *i << "\"" << endl;
    }

    debug() << QString( "inserting %1 tracks from playlist" ).arg( tracks.count() );
    The::playlistModel()->insertOptioned( tracks, Playlist::Append );

    return true;
}

unsigned int
PlaylistLoader::loadPls_extractIndex( const QString &str ) const
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
PlaylistLoader::loadM3u( QTextStream &stream )
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
                tracks.append( CollectionManager::instance()->trackForUrl( kurl ) );
            }
            else {
                tracks.append( CollectionManager::instance()->trackForUrl( KUrl( line ) ) );
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


#include "PlaylistLoader.moc"





