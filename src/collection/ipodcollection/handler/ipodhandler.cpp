/******************************************************************************************
 * copyright            : (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>          *
 ******************************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "IpodHandler"

#include "ipodhandler.h"

#include "IpodCollection.h"
#include "IpodMeta.h"
#include "Debug.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>

using namespace Ipod;
using namespace Meta;

IpodHandler::IpodHandler( IpodCollection *mc, const QString& mountPoint, QObject *parent )
    : QObject( parent )
    , m_memColl( mc )
    , m_mountPoint( mountPoint )
{
DEBUG_BLOCK

//    initializeIpod();

 GError *err = 0;
 m_itdb = itdb_parse( QFile::encodeName( m_mountPoint ),  &err );
 if (  err )
 {
     g_error_free( err );
     if (  m_itdb )
     {
         itdb_free(  m_itdb );
         m_itdb = 0;
     }

 }


}

IpodHandler::~IpodHandler()
{
        if (  m_itdb )
            itdb_free( m_itdb );

//        m_files.clear();
}


bool
IpodHandler::initializeIpod()
{
    DEBUG_BLOCK
    QDir dir( mountPoint() );
    if( !dir.exists() )
    {
        debug() << "Media device: Mount point does not exist!";
        return false;
    }

    debug() << "initializing iPod mounted at " << mountPoint();

    // initialize iPod
    m_itdb = itdb_new();
    if( m_itdb == 0 )
        return false;

    // in order to get directories right
    detectModel();

    itdb_set_mountpoint(m_itdb, QFile::encodeName(mountPoint()));

    Itdb_Playlist *mpl = itdb_playlist_new("iPod", false);
    itdb_playlist_set_mpl(mpl);
    Itdb_Playlist *podcasts = itdb_playlist_new("Podcasts", false);
    itdb_playlist_set_podcasts(podcasts);
    itdb_playlist_add(m_itdb, podcasts, -1);
    itdb_playlist_add(m_itdb, mpl, 0);

    debug() << "Init 1";

    QString realPath;
    if(!pathExists( itunesDir(), &realPath) )
    {
        dir.setPath(realPath);
        dir.mkdir(dir.absolutePath());
    }
    if(!dir.exists())
        return false;

    debug() << "Init 2";

    if(!pathExists( itunesDir( "Music" ), &realPath) )
    {
        dir.setPath(realPath);
        dir.mkdir(dir.absolutePath());
    }
    if(!dir.exists())
        return false;

    debug() << "Init 3";

    if(!pathExists( itunesDir( "iTunes" ), &realPath) )
    {
        dir.setPath(realPath);
        dir.mkdir(dir.absolutePath());
    }
    if(!dir.exists())
        return false;

    debug() << "Init 4";

    if( !writeITunesDB( false ) )
        return false;

    debug() << "Init 5";

//    The::statusBar()->longMessage(
//            i18n("Media Device: Initialized iPod mounted at %1", mountPoint() ),
//            KDE::StatusBar::Information );

    return true;
}

void
IpodHandler::detectModel()
{
    // set some sane default values
    m_isShuffle = false;
    m_supportsArtwork = true;
    m_supportsVideo = false;
    m_isIPhone = false;
    m_needsFirewireGuid = false;
    m_rockboxFirmware = false;

    // needs recent libgpod-0.3.3 from cvs
    bool guess = false;
    if( m_itdb && m_itdb->device )
    {
        const Itdb_IpodInfo *ipodInfo = itdb_device_get_ipod_info( m_itdb->device );
        const gchar *modelString = 0;
        m_supportsArtwork = itdb_device_supports_artwork( m_itdb->device );

        if( ipodInfo )
        {
            modelString = itdb_info_get_ipod_model_name_string ( ipodInfo->ipod_model );

            switch( ipodInfo->ipod_model )
            {
            case ITDB_IPOD_MODEL_SHUFFLE:
#ifdef HAVE_LIBGPOD_060
            case ITDB_IPOD_MODEL_SHUFFLE_SILVER:
            case ITDB_IPOD_MODEL_SHUFFLE_PINK:
            case ITDB_IPOD_MODEL_SHUFFLE_BLUE:
            case ITDB_IPOD_MODEL_SHUFFLE_GREEN:
            case ITDB_IPOD_MODEL_SHUFFLE_ORANGE:
            case ITDB_IPOD_MODEL_SHUFFLE_PURPLE:
#endif
                m_isShuffle = true;
                break;
#ifdef HAVE_LIBGPOD_060
            case ITDB_IPOD_MODEL_IPHONE_1:
            case ITDB_IPOD_MODEL_TOUCH_BLACK:
                m_isIPhone = true;
                debug() << "detected iPhone/iPod Touch" << endl;
                break;
            case ITDB_IPOD_MODEL_CLASSIC_SILVER:
            case ITDB_IPOD_MODEL_CLASSIC_BLACK:
#endif
            case ITDB_IPOD_MODEL_VIDEO_WHITE:
            case ITDB_IPOD_MODEL_VIDEO_BLACK:
            case ITDB_IPOD_MODEL_VIDEO_U2:
                m_supportsVideo = true;
                debug() << "detected video-capable iPod";
                break;
            case ITDB_IPOD_MODEL_MOBILE_1:
                m_isMobile = true;
                debug() << "detected iTunes phone" << endl;
                break;
            case ITDB_IPOD_MODEL_INVALID:
            case ITDB_IPOD_MODEL_UNKNOWN:
                modelString = 0;
                guess = true;
                break;
            default:
                break;
            }

#ifdef HAVE_LIBGPOD_060
            switch( ipodInfo->ipod_generation )
            {
               case ITDB_IPOD_GENERATION_CLASSIC_1:
               case ITDB_IPOD_GENERATION_NANO_3:
               case ITDB_IPOD_GENERATION_TOUCH_1:
                  m_needsFirewireGuid = true;
                  m_supportsVideo = true;
                  break;
               case ITDB_IPOD_GENERATION_VIDEO_1:
               case ITDB_IPOD_GENERATION_VIDEO_2:
                  m_supportsVideo = true;
                  break;
               case ITDB_IPOD_GENERATION_SHUFFLE_1:
               case ITDB_IPOD_GENERATION_SHUFFLE_2:
               case ITDB_IPOD_GENERATION_SHUFFLE_3:
                  m_isShuffle = true;
                  break;
               default:
                  break;
            }
#endif
        }
        if( modelString )
            m_name = QString( "iPod %1" ).arg( QString::fromUtf8( modelString ) );

        if( m_needsFirewireGuid )
        {
            gchar *fwid = itdb_device_get_sysinfo( m_itdb->device, "FirewireGuid" );
            if( !fwid )
            {
//                The::statusBar()->longMessage(
//                                            i18n("Your iPod's Firewire GUID is required for correctly updating its music database, but it is not known. See http://amarok.kde.org/wiki/Media_Device:IPod for more information." ) );
            }
            else
               g_free( fwid );
        }
    }
    else
    {
        debug() << "iPod type detection failed, no video support";
//        The::statusBar()->longMessage(
//                i18n("iPod type detection failed: no support for iPod Shuffle, for artwork or video") );
        guess = true;
    }

    if( guess )
    {
        if( pathExists( ":iTunes:iTunes_Control" ) )
        {
            debug() << "iTunes/iTunes_Control found - assuming itunes phone" << endl;
            m_isMobile = true;
        }
        else if( pathExists( ":iTunes_Control" ) )
        {
            debug() << "iTunes_Control found - assuming iPhone/iPod Touch" << endl;
            m_isIPhone = true;
        }
    }

    if( m_isIPhone )
    {
        m_supportsVideo = true;
        m_supportsArtwork = true;
    }

    if( pathExists( ":.rockbox" ) )
    {
        debug() << "RockBox firmware detected" << endl;
        m_rockboxFirmware = true;
    }
}

bool
IpodHandler::pathExists( const QString &ipodPath, QString *realPath )
{
    QDir curDir( mountPoint() );
    QString curPath = mountPoint();
    QStringList components = QStringList::split( ":", ipodPath );

    bool found = false;
    QStringList::iterator it = components.begin();
    for( ; it != components.end(); ++it )
    {
        found = false;
        for(uint i=0; i<curDir.count(); i++)
        {
            if( curDir[i].toLower() == (*it).toLower())
            {
                curPath += '/' + curDir[i];
                curDir.cd( curPath );
                found = true;
                break;
            }
        }
        if(!found)
            break;
    }

    for( ; it != components.end(); ++it )
        curPath += '/' + *it;

    //debug() << ipodPath << ( found ? "" : " not" ) << " found, actually " << curPath;

    if( realPath )
        *realPath = curPath;

    return found;
}

bool
IpodHandler::writeITunesDB( bool threaded )
{
    return false; // TODO: implement this function correctly
    if(!m_itdb)
        return false;

    if(m_dbChanged)
    {
        bool ok = false;
//         if( !threaded || MediaBrowser::instance()->isQuitting() )
        if( !threaded )
        {
            if( !m_itdb )
            {
                return false;
            }

            ok = true;
            GError *error = 0;
            if ( !itdb_write (m_itdb, &error) )
            {   /* an error occurred */
                if(error)
                {
                    if (error->message)
                        debug() << "itdb_write error: " << error->message;
                    else
                        debug() << "itdb_write error: " << "error->message == 0!";
                    g_error_free (error);
                }
                error = 0;
                ok = false;
            }

            if( m_isShuffle )
            {
                /* write shuffle data */
                if (!itdb_shuffle_write (m_itdb, &error))
                {   /* an error occurred */
                    if(error)
                    {
                        if (error->message)
                            debug() << "itdb_shuffle_write error: " << error->message;
                        else
                            debug() << "itdb_shuffle_write error: " << "error->message == 0!";
                        g_error_free (error);
                    }
                    error = 0;
                    ok = false;
                }
            }
        }
        else
        {
//            ThreadManager::instance()->queueJob( new IpodWriteDBJob( this, m_itdb, m_isShuffle, &ok ) );
//            while( ThreadManager::instance()->isJobPending( "IpodWriteDBJob" ) )
//            {
//                kapp->processEvents();
//                usleep( 10000 );
//            }
        }

        if( ok )
        {
            m_dbChanged = false;
        }
        else
        {
//            The::statusBar()->longMessage(
//                    i18n("Media device: failed to write iPod database"),
//                    KDE::StatusBar::Error );
        }

        return ok;
    }
    return true;
}

QString
IpodHandler::itunesDir(const QString &p) const
{
    QString base( ":iPod_Control" );
    if( m_isMobile )
        base = ":iTunes:iTunes_Control";

    if( !p.startsWith( ':' ) )
        base += ':';
    return base + p;
}

#ifdef JUST_TESTING
bool
IpodHandler::openDevice( bool silent )
{
    m_isShuffle = false;
    m_isMobile = false;
    m_supportsArtwork = false;
    m_supportsVideo = false;
    m_needsFirewireGuid = false;
    m_rockboxFirmware = false;
    m_dbChanged = false;
//    m_files.clear();

    if( m_itdb )
    {
//        The::statusBar()->longMessage(
//                i18n("Media Device: iPod at %1 already opened", mountPoint() ),
//                KDE::StatusBar::Sorry );
        return false;
    }

    // try to find a mounted ipod
    bool ipodFound = false;
    bool canInitialize = false;
//    KMountPoint::List currentmountpoints = KMountPoint::currentMountPoints();
    for( KMountPoint::List::Iterator mountiter = currentmountpoints.begin();
        mountiter != currentmountpoints.end();
        ++mountiter )
    {
        canInitialize = false;
        QString devicenode = (*mountiter)->mountedFrom();
        QString mountpoint = (*mountiter)->mountPoint();

        if( mountpoint.startsWith( "/proc" ) ||
            mountpoint.startsWith( "/sys" )  ||
            mountpoint.startsWith( "/dev" )  ||
            mountpoint.startsWith( "/boot" ) )
            continue;

        if( !mountPoint().isEmpty() )
        {
            if( mountpoint != mountPoint() )
                continue;
            canInitialize = true;
        }

        else if( !deviceNode().isEmpty() )
        {
            if( devicenode != deviceNode() )
                continue;
            canInitialize = true;
        }

        GError *err = 0;
        m_itdb = itdb_parse(QFile::encodeName(mountpoint), &err);
        if( err )
        {
            g_error_free(err);
            if( m_itdb )
            {
                itdb_free( m_itdb );
                m_itdb = 0;
            }

            if( !canInitialize )
                continue;
        }

        if( mountPoint().isEmpty() )
            m_medium.setMountPoint( mountpoint );
        ipodFound = true;
        break;
    }

    if( !ipodFound && !canInitialize )
    {
        if( !silent )
        {
            The::statusBar()->longMessage(
                    i18n("Media Device: No mounted iPod found" ),
                    KDE::StatusBar::Sorry );
        }
        return false;
    }

    if( !m_itdb && canInitialize )
    {
        QString msg = i18n( "Media Device: could not find iTunesDB on device mounted at %1. "
                "Should I try to initialize your iPod?", mountPoint() );

        if( !silent
                && KMessageBox::warningContinueCancel( m_parent, msg, i18n( "Initialize iPod?" ),
                    KGuiItem(i18n("&Initialize"), "new") ) == KMessageBox::Continue )
        {
            if( !initializeIpod() )
            {
                if( m_itdb )
                {
                    itdb_free( m_itdb );
                    m_itdb = 0;
                }

                The::statusBar()->longMessage(
                        i18n("Media Device: Failed to initialize iPod mounted at %1", mountPoint() ),
                        KDE::StatusBar::Sorry );

                return false;
            }
        }
        else
           return false;
    }

    detectModel();

    if( !createLockFile( silent ) )
    {
        if( m_itdb )
        {
            itdb_free( m_itdb );
            m_itdb = 0;
        }
        return false;
    }

    for( int i=0; i < itdb_musicdirs_number(m_itdb); i++)
    {
        QString real;
        QString ipod;
        ipod.sprintf( itunesDir( "Music:f%02d" ).toLatin1(), i );
        if(!pathExists( ipod, &real ) )
        {
            QDir dir( real );
            dir.mkdir( real );
            dir.setPath( real );
            if( !dir.exists() )
            {
                debug() << "failed to create hash dir " << real;
                The::statusBar()->longMessage(
                        i18n( "Media device: Failed to create directory %1", real ),
                        KDE::StatusBar::Error );
                return false;
            }
        }
    }

    if( !silent )
        kapp->processEvents( 100 );

    initView();
    GList *cur = m_itdb->playlists;
    for( ; cur; cur = cur->next )
    {
        Itdb_Playlist *playlist = (Itdb_Playlist *)cur->data;
        addPlaylistToView( playlist );
    }

    if( !silent )
        kapp->processEvents( 100 );

    for( cur = m_itdb->tracks; cur; cur = cur->next )
    {
        Itdb_Track *track = (Itdb_Track *)cur->data;
        addTrackToView( track, 0 /*parent*/, false /*checkintegrity*/, true /*batchmode*/ );
    }

    if( !silent )
        kapp->processEvents( 100 );

    updateRootItems();
    m_customAction->setEnabled( true );

    m_dbChanged = true; // write at least once for synchronising new stats

    return true;
}
#endif

void
IpodHandler::parseTracks()
{
    DEBUG_BLOCK

    TrackMap trackMap;
    ArtistMap artistMap;
    AlbumMap albumMap;
    GenreMap genreMap;
    ComposerMap composerMap;
    YearMap yearMap;

    GList *cur;

    int debugtest = 0;
/* iterate through tracklist and add to appropriate map */
    for ( cur = m_itdb->tracks; cur; cur = cur->next )
    {
        /* ipodtrack - provides libgpod itdb info */
        /* track - the new track whose data is being set up */
        Itdb_Track *ipodtrack = ( Itdb_Track * )cur->data;
        QString format( ipodtrack->filetype );
        IpodTrackPtr track( new IpodTrack( m_memColl, format ) );

        /* 1-liner info retrieval */

        track->setTitle( QString::fromUtf8( ipodtrack->title ) );
        track->setLength( ( ipodtrack->tracklen ) / 1000 );
        track->setTrackNumber( ipodtrack->track_nr );
        track->setComment( QString::fromUtf8(  ipodtrack->comment ) );
        track->setDiscNumber( ipodtrack->cd_nr );
        track->setBitrate( ipodtrack->bitrate );
        track->setBpm( ipodtrack->BPM );

        QString path = QString( ipodtrack->ipod_path ).split( ":" ).join( "/" );
        path = m_mountPoint + path;
        track->setPlayableUrl( path );

        /* map-related info retrieval */
        QString album( QString::fromUtf8( ipodtrack->album ) );
        IpodAlbumPtr albumPtr;

        if ( albumMap.contains( album ) )
            albumPtr = IpodAlbumPtr::staticCast(  albumMap.value(  album ) );

        else
        {
            albumPtr = IpodAlbumPtr(  new IpodAlbum(  album ) );
            albumMap.insert(  album,  AlbumPtr::staticCast(  albumPtr ) );
        }

        albumPtr->addTrack(  track );
        track->setAlbum(  albumPtr );

        QString artist ( QString::fromUtf8( ipodtrack->artist ) );
        IpodArtistPtr artistPtr;

        if (  artistMap.contains(  artist ) )
        {
            artistPtr = IpodArtistPtr::staticCast(  artistMap.value(  artist ) );
        }
        else
        {
            artistPtr = IpodArtistPtr(  new IpodArtist(  artist ) );
            artistMap.insert(  artist,  ArtistPtr::staticCast(  artistPtr ) );
        }

        artistPtr->addTrack(  track );
        track->setArtist(  artistPtr );

        QString composer ( QString::fromUtf8( ipodtrack->composer ) );
        IpodComposerPtr composerPtr;

        if ( composerMap.contains( composer ) )
        {
            composerPtr = IpodComposerPtr::staticCast( composerMap.value( composer ) );
        }
        else
        {
            composerPtr = IpodComposerPtr( new IpodComposer( composer ) );
            composerMap.insert( composer, ComposerPtr::staticCast( composerPtr ) );
        }

        QString year;
        year = year.setNum( ipodtrack->year );
        IpodYearPtr yearPtr;
        if (  yearMap.contains(  year ) )
            yearPtr = IpodYearPtr::staticCast(  yearMap.value(  year ) );
        else
        {
            yearPtr = IpodYearPtr(  new IpodYear(  year ) );
            yearMap.insert(  year,  YearPtr::staticCast(  yearPtr ) );
        }
        yearPtr->addTrack(  track );
        track->setYear(  yearPtr );

        QString genre = ipodtrack->genre;
        IpodGenrePtr genrePtr;

        if (  genreMap.contains(  genre ) )
            genrePtr = IpodGenrePtr::staticCast(  genreMap.value(  genre ) );

        else
        {
            genrePtr = IpodGenrePtr(  new IpodGenre(  genre ) );
            genreMap.insert(  genre,  GenrePtr::staticCast(  genrePtr ) );
        }

        genrePtr->addTrack( track );
        track->setGenre( genrePtr );


    }

    m_memColl->acquireWriteLock();
    m_memColl->setTrackMap(  trackMap );
    m_memColl->setArtistMap(  artistMap );
    m_memColl->setAlbumMap(  albumMap );
    m_memColl->setGenreMap(  genreMap );
    m_memColl->setComposerMap(  composerMap );
    m_memColl->setYearMap(  yearMap );
    m_memColl->releaseLock();

}
