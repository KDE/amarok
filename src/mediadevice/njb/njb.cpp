/**
 * Nomad Jukebox KIO::Slave
 * @copyright Copyright 2004 Shaun Jackman <sjackman@debian.org>
 * @copyright Copyright 2005 Ace Jones <acejones@users.sf.net>
 * @copyright Copyright 2005 Rob Walker <rob@tenfoot.co.uk>
 * @author Shaun Jackman <sjackman@debian.org>
 */
static const char* rcsid __attribute__((unused)) =
"$Id: njb.cpp,v 1.16.2.14 2005/07/07 01:26:07 acejones Exp $";


// kionjb
#include "mp3.h"
#include "njb.h"
#include "playlist.h"
#include "track.h"
#include "config.h"

// libnjb
#include <libnjb.h>

// libid3
#include <id3/tag.h>

// kde
#include <kdebug.h>
#include <kinstance.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kurl.h>

// qt
#include <qcstring.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qptrlist.h>

// posix
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// This function has NOT handled the request, so other functions may be called
// upon to do so
const int NJB_NOTHANDLED = 0;

// This function has handled the request, so no further processing is needed.
const int NJB_HANDLED = -1;

njb_t* theNjb = NULL;
trackValueList* theTracks = NULL;

using namespace KIO;
extern "C" {

    int
        kdemain( int argc, char **argv)
        {
            KInstance instance( argv[0] );

            kdDebug( 7182) << "*** Starting " << argv[0] << endl;

            if( argc != 4) {
                kdDebug( 7182)
                    << "Usage: " << argv[0] << " protocol domain-socket1 domain-socket2"
                    << endl;
                exit( -1);
            }

            kio_njbProtocol slave( argv[0], argv[2], argv[3]);
            slave.dispatchLoop();

            kdDebug( 7182) << "*** " << argv[0] << " Done" << endl;
            return 0;
        }

} // extern "C"


/* ------------------------------------------------------------------------ */
kio_njbProtocol::kio_njbProtocol( const QCString& name,
        const QCString& pool_socket, const QCString& app_socket)
: SlaveBase( name, pool_socket, app_socket),
    m_busy( false )
{
    kdDebug( 7182) << "constructor: pid=" << getpid() << endl;
    m_njb = NULL;
    m_captured = false;
    m_libcount = 0;
    theTracks = &trackList;

    NJB_Set_Debug(0); // or try DD_SUBTRACE

}


/* ------------------------------------------------------------------------ */
kio_njbProtocol::~kio_njbProtocol( void)
{
    kdDebug( 7182) << "deconstructor: pid=" << getpid() << endl;

    trackList.clear();
    playlistList.clear();
    dataFileList.clear();

    disconnect(true);
}

/* ------------------------------------------------------------------------ */
void
kio_njbProtocol::err( int status, const QString& msg)
{
    QString fullMsg( msg);
    if( status == ERR_COULD_NOT_CONNECT)
        fullMsg = "Nomad Jukebox";
    if( !m_errMsg.isEmpty())
        fullMsg += '\n' + m_errMsg;
    error( status, fullMsg);
    m_errMsg = "";
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::open( void)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;

    if( m_njb)
        return NJB_SUCCESS;

    njb_t njbs[ NJB_MAX_DEVICES];
    int n;

    m_njb = new njb_t;
    theNjb = m_njb;

    kdDebug( 7182) << __func__ << ": sizeof(njb_t)=" << sizeof(njb_t) << endl;

    if( NJB_Discover( njbs, 0, &n) == -1 || n == 0) {
        kdDebug( 7182) << __func__ << ": no NJBs found\n";
        theNjb = m_njb = NULL;
        return NJB_FAILURE;
    }

    m_njb = new njb_t;
    *m_njb = njbs[0];
    theNjb = m_njb;

    if( NJB_Open( m_njb) == -1) {
        kdDebug( 7182) << __func__ << ": couldn't open\n";
        kdDebug( 7182) << __func__ << ": deleting " << m_njb << "\n";
        delete m_njb;
        theNjb = m_njb = NULL;
        return NJB_FAILURE;
    }

    return NJB_SUCCESS;
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::capture( void)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if( m_captured)
        return NJB_SUCCESS;
    if( NJB_Capture( m_njb) == -1) {
        kdDebug( 7182) << __func__ << ": couldn't capture\n";
        return -1;
    }
    m_captured = true;
    return NJB_SUCCESS;
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::connect( bool docapture)
{
    setTimeoutSpecialCommand(-1);

    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    infoMessage( i18n( "Connecting to <b>%1</b>...").arg( "Nomad JukeBox"));

    if( open())
        return ERR_COULD_NOT_CONNECT;
    if( docapture && capture())
        return ERR_COULD_NOT_CONNECT;

    infoMessage( i18n( "Connected to <b>%1</b>").arg( "Nomad JukeBox"));
    kdDebug( 7182) << __func__ << ": Connected" << " m_njb=" << m_njb << endl;
    return NJB_SUCCESS;

}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::disconnect( bool force)
{
    if ( force)
    {
        setTimeoutSpecialCommand(-1);
        kdDebug( 7182) << __func__ << ": pid=" << getpid() << " m_njb=" << m_njb << endl;
        if( m_captured) {
            NJB_Release( m_njb);
            m_captured = false;
        }
        if( m_njb) {
            NJB_Close( m_njb);
            kdDebug( 7182) << __func__ << ": deleting m_njb " << m_njb << endl;
            delete m_njb;
            m_njb = NULL;
            theNjb = m_njb;
        }

        kdDebug( 7182) << __func__ << ": disconnected, pid=" << getpid() << endl;
    }
    else
    {
        kdDebug( 7182) << __func__ << ": disconnect timer set, pid=" << getpid() << endl;
        setTimeoutSpecialCommand(5);
    }
    return NJB_SUCCESS;
}


/* ------------------------------------------------------------------------ */
void
kio_njbProtocol::closeConnection( void)
{
    kdDebug( 7182) << __func__ << endl;
    disconnect(true);
}


/* ------------------------------------------------------------------------ */
void
kio_njbProtocol::dataQString( const QString& string)
{
    QByteArray a( (QCString)string);
    a.resize( a.size() - 1);
    data( a);
}

/* ------------------------------------------------------------------------ */
void
kio_njbProtocol::dataQStringList( const QStringList& stringlist)
{
    QStringList::const_iterator it_string = stringlist.begin();
    while ( it_string != stringlist.end() )
    {
        dataQString(*it_string + "\n");
        ++it_string;
    }
}

/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::progressCallback( u_int64_t sent, u_int64_t total, const char* /*buf*/, unsigned /*len*/, void* data)
{
    kio_njbProtocol* kio = reinterpret_cast<kio_njbProtocol*>(data);
    kio->totalSize( total);
    kio->processedSize( sent);
    unsigned secs = time( 0) - kio->m_progressStart;
    kio->speed( secs ? sent / secs : 0);
    kio->infoMessage(kio->m_progressMessage.arg( (unsigned)(100 * sent / total)));
    return NJB_SUCCESS;
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::getAlbum( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if( url.path().right( 4) != ".m3u")
        return NJB_NOTHANDLED;
    if( !url.path().startsWith( "/artists/") &&
            !url.path().startsWith( "/albums/"))
        return NJB_NOTHANDLED;

    int status = readJukeboxMusic();
    if( status)
        return status;

    QString albumName = url.filename();
    albumName = albumName.left(albumName.length()-4);

    //Getting the album track listing
    // TODO: check by artist if available
    QMap<int,Track> albumTracks;
    trackValueList::const_iterator it;
    for( it = trackList.begin(); it != trackList.end(); it++)
        if( (*it).getAlbum() == albumName)
            albumTracks[(*it).getTrackNum()] = *it;

    //Finally display album tracks ordered by track number
    QMap<int,Track>::const_iterator it_track = albumTracks.begin();
    while ( it_track != albumTracks.end() )
    {
        dataQString( (*it_track).getFilename() + QString( "\n"));
        ++it_track;
    }

    return NJB_HANDLED;
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::getEtc( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if( url.directory() != "/etc")
        return NJB_NOTHANDLED;

    int status;

    if ( url.fileName() == "counter" ) 
    {
        if( (status = connect()))
            return status;

        u_int64_t counter = NJB_Get_NJB1_Libcounter(m_njb);
        if ( ! counter )
            dataQString( "No counter found\n");
        else
            dataQString( QString::number( counter ) + "\n");
    }
    else if( url.fileName() == "diskfree") {
        if( (status = connect(false /*capture*/)))
            return status;
        u_int64_t total64, free64;
        int status = NJB_Get_Disk_Usage( m_njb, &total64, &free64);
        if( status) {
            kdDebug( 7182) << "getEtc: NJB_Get_Disk_Usage failed\n";
            return ERR_COULD_NOT_READ;
        }
        mimeType( "text/plain");
        unsigned long total = total64 / 1024;
        unsigned long free = free64 / 1024;
        unsigned long used = total - free;
        dataQString( "1k-blocks      Used Available Use%\n");
        QString df;
        df.sprintf( "%9lu %9lu %9lu %3lu%%\n",
                total, used, free, 100*used/total);
        dataQString( df);
    } else
        if( url.fileName() == "eax") {
            // xxx FIXME unimplemented
            mimeType( "text/plain");
            dataQString( "eax info (unimplemented)\n");
        } else
            if( url.fileName() == "firmware") {
                if( (status = connect(false /*capture*/)))
                    return status;
                NJB_Ping(m_njb);
                mimeType( "text/plain");

                u_int8_t fwMajor, fwMinor, fwRel;
                NJB_Get_Firmware_Revision(m_njb,&fwMajor,&fwMinor,&fwRel);
                QString devicename = NJB_Get_Device_Name( m_njb, 0 );

                QString result( QString::number( fwMajor) + "." 
                        + QString::number( fwMinor) + "." 
                        + QString::number( fwRel) + " for " 
                        + devicename);
                dataQString( result );
                kdDebug( 7182) << __func__ << ": returned " << result << endl;
            } else
                if( url.fileName() == "owner") {
                    if( (status = connect(false /*capture*/)))
                        return status;
                    char* owner = NJB_Get_Owner_String( m_njb);
                    if( !owner) {
                        kdDebug( 7182) << __func__ << ": NJB_Get_Owner_String failed\n";
                        return ERR_COULD_NOT_READ;
                    }
                    mimeType( "text/plain");
                    dataQString( (QString)owner + "\n");
                    free( owner);
                } else
                    if( url.fileName() == "serial") {
                        if( (status = connect(false /*capture*/)))
                            return status;
                        mimeType( "text/plain");
                        NJB_Ping(m_njb);
                        u_int8_t hwMajor, hwMinor, hwRel;
                        NJB_Get_Hardware_Revision(m_njb,&hwMajor,&hwMinor,&hwRel);
                        QString productname = NJB_Get_Device_Name( m_njb, 1 );
                        QString result( productname + " " 
                                + QString::number( hwMajor) + "."
                                + QString::number( hwMinor) + "." 
                                + QString::number( hwRel));
                        dataQString( result );
                        kdDebug( 7182) << __func__ << ": returned " << result << endl;                                
                    } else
                        if( url.fileName() == "version") {
                            mimeType( "text/plain");
                            dataQString( (QString)"kionjb-" + VERSION);
                        } else
                            return ERR_DOES_NOT_EXIST;
                        return NJB_HANDLED;
}

/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::getPlaylist( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if( url.directory() != "/playlists")
        return NJB_NOTHANDLED;

    int status = readJukeboxMusic();
    if( status != NJB_SUCCESS )
        return NJB_HANDLED;

    mimeType( "text/plain");
    // First, find the requested playlist
    playlistValueList::const_iterator it;
    for( it = playlistList.begin();it != playlistList.end(); it++)
        if( *it == url.filename())
        {
            QStringList tracknames = (*it).trackNames();
            dataQStringList( tracknames );
        }

    return NJB_HANDLED;
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::getFile( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << " url=" << url.prettyURL()<< endl;

    if(!url.directory().startsWith("/data"))
        return NJB_NOTHANDLED;

    if( m_busy)
        return ERR_COULD_NOT_READ;

    int status = readJukeboxDatas();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    QString targetPath = url.path(-1).mid(5).replace('/', '\\');
    QString targetFolder = targetPath.left(targetPath.findRev('\\')) + '\\';
    kdDebug(7182) << __func__ << " path=" << targetPath << " folder=" << targetFolder << endl;

    dataFileValueList::const_iterator it;
    for (it = dataFileList.begin(); it != dataFileList.end(); it++)
    {
        QString filePath = (*it).folder + ( *it).filename;
        if (filePath == targetPath)
            break;
    }

    if( it == dataFileList.end())
        return ERR_DOES_NOT_EXIST;

    u_int64_t filesize = (*it).filesize;
    kdDebug(7182) << __func__ << " size=" << static_cast<unsigned long>(filesize) << endl;
    totalSize( filesize);

    status = connect();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    KTempFile tmpfile;
    m_busy = true;
    m_progressStart = time( 0);
    m_progressMessage = "Retrieving %1%...";	

    status = NJB_Get_File( m_njb, ( *it).dfid, filesize,
            tmpfile.name(), progressCallback, this);
    m_busy = false;
    if( status == -1) {
        kdDebug( 7182) << __func__ << ": NJB_Get_File failed\n";
        if (NJB_Error_Pending(m_njb)) 
        {
            const char* error;
            while ((error = NJB_Error_Geterror(m_njb)))
                kdError( 7182) << __func__ << ": " << error << endl;
        }
        else
            kdDebug( 7182) << __func__ << ": No reason for failure reported.\n";
        QDir().remove( tmpfile.name());
        return ERR_COULD_NOT_READ;
    }

    const unsigned chunksize = 1024;
    QByteArray buf(chunksize);
    while (filesize > chunksize)
    {
        tmpfile.dataStream()->readRawBytes(buf.data(), chunksize) ;
        data(buf);
        filesize -= chunksize;
    }
    if (filesize > 0)
    {
        // get last chunk (<1024 bytes)
        buf.resize(filesize);
        tmpfile.dataStream()->readRawBytes(buf.data(), filesize) ;
        data(buf);
    }
    //Cleanup
    QDir().remove( tmpfile.name());
    data( QByteArray()); // empty array means we're done sending the data
    finished();

    return NJB_HANDLED;
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::getTrack( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;

    if( url.directory() != "/all" &&
            !url.path().startsWith( "/artists/") &&
            !(url.path().startsWith( "/albums/") && url.path(-1).contains("/") == 3))
        return NJB_NOTHANDLED;

    if( m_busy)
        // njb is already in the middle of a transfer
        return ERR_COULD_NOT_READ;

    int status = readJukeboxMusic();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    trackValueList::const_iterator it_track = theTracks->findTrackByName( url.fileName() );
    if( it_track == theTracks->end() )
        return ERR_DOES_NOT_EXIST;

    //	kdDebug( 7182) << "getTrack: trying to get track : " << track.getTitle() << endl;

    totalSize( (*it_track).getSize());

    status = connect();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    KTempFile tmpfile;
    mimeType( "audio/x-mp3");
    m_busy = true;
    m_progressStart = time( 0);
    m_progressMessage = "Retrieved %1%...";

    status = NJB_Get_Track( m_njb, (*it_track).getId(), (*it_track).getSize(),
            tmpfile.name(), progressCallback, this);
    m_busy = false;
    if( status == -1) {
        kdDebug( 7182) << __func__ << ": NJB_Get_Track failed\n";
        if (NJB_Error_Pending(m_njb)) 
        {
            const char* error;
            while ((error = NJB_Error_Geterror(m_njb)))
                kdError( 7182) << __func__ << ": " << error << endl;
        }
        else
            kdDebug( 7182) << __func__ << ": No reason for failure reported.\n";
        QDir().remove( tmpfile.name());
        return ERR_COULD_NOT_READ;
    }

    const unsigned chunksize = 1024;
    QByteArray buf(chunksize);
    unsigned size = (*it_track).getSize();
    while ( size > chunksize )
    {
        tmpfile.dataStream()->readRawBytes( buf.data(), chunksize) ;
        data( buf);
        size -= chunksize;
    }
    if (size > 0)
    {
        buf.resize(size);
        tmpfile.dataStream()->readRawBytes( buf.data(), size) ;
        data( buf);
    }
    QDir().remove( tmpfile.name());
    data( QByteArray()); // empty array means we're done sending the data
    finished();

    return NJB_HANDLED;
}

/* ------------------------------------------------------------------------ */
void
kio_njbProtocol::get( const KURL& url )
{
    kdDebug( 7182) << __func__ << ": " << url.prettyURL() << endl;

    int status = 0;
    if( !status) status = getAlbum( url);
    if( !status) status = getEtc( url);
    if( !status) status = getPlaylist( url);
    if( !status) status = getTrack( url);
    if( !status) status = getFile( url);
    if( status < 0) {
        data( QByteArray());
        finished();
    } else
        err( status ? status : ERR_DOES_NOT_EXIST, url.fileName());
    disconnect();
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::delEtc( const KURL& url)
{
    if( url.directory() != "/etc")
        return NJB_NOTHANDLED;
    if( url.fileName() == "owner") {
        int status = connect();
        if( status != NJB_SUCCESS)
            return NJB_HANDLED;
        status = NJB_Set_Owner_String( m_njb, "");
        if( status != NJB_SUCCESS) {
            kdDebug( 7182) << "delEtc: NJB_Set_Owner_String failed\n";
            return ERR_CANNOT_DELETE;
        }
        return NJB_HANDLED;
    }
    return ERR_DOES_NOT_EXIST;
}


/* ----------------------------------------------------------------------- */
int
kio_njbProtocol::delPlaylist( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": " << url.prettyURL() << endl;
    if( url.directory() != "/playlists")
        return NJB_NOTHANDLED;

    int status = readJukeboxMusic();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    playlistValueList::const_iterator it;
    for( it = playlistList.begin(); it != playlistList.end(); it++)
        if( *it == url.fileName())
            break;

    if( it == playlistList.end()) {
        warning("Playlist has not been found in the jukebox!\n" );
        return NJB_HANDLED;
    }

    status = connect();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    kdDebug( 7182) << __func__ << ": deleting id " << ( *it).getId() << endl;

    // Then delete it
    status = NJB_Delete_Playlist( m_njb, ( *it).getId());
    if( status) {
        kdDebug( 7182) << __func__ << ": NJB_Delete_Playlist failed\n";
        return ERR_CANNOT_DELETE;
    }

    return NJB_HANDLED;
}

/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::delFile( const KURL& url)
{	
    kdDebug( 7182) << __func__ << endl;

    if(!url.directory().startsWith("/data"))
        return NJB_NOTHANDLED;

    int status = readJukeboxDatas();
    if( status)
        return status;

    QString targetPath = url.path(-1).mid(5).replace('/', '\\');

    status = connect();
    if (status)
        return status;

    dataFileValueList::iterator it;
    bool found = false;
    for (it = dataFileList.begin(); it != dataFileList.end(); )
    {
        QString filePath = (*it).folder + ( *it).filename;
        if (filePath == targetPath ||
                (*it).folder.startsWith(targetPath + '\\'))
        {
            kdDebug( 7182) << __func__ << " " << filePath << endl;

            if ((*it).dfid != 0)
            {
                status = NJB_Delete_Datafile(m_njb, (*it).dfid);
                if (status) {
                    kdDebug( 7182) << __func__ << ": NJB_Delete_File failed";
                    return ERR_CANNOT_DELETE;
                }
            }
            it = dataFileList.erase(it);
            found = true;
        }
        else
            ++it;
    }

    if (!found)
        return ERR_DOES_NOT_EXIST;
    else
        return NJB_HANDLED;
}

/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::delTrack( const KURL& url)
{
    kdDebug( 7182) << __func__ << endl;

    if( url.directory() != "/all" &&
            !url.path().startsWith( "/artists/") &&
            !(url.path().startsWith( "/albums/") && url.path(-1).contains("/") == 3))
        return NJB_NOTHANDLED;

    int status = readJukeboxMusic();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    // If we're supposed to delete "artists/xxx/*", delete all tracks of this artist
    if ( url.path().startsWith("/artists/") && url.fileName() == "*" )
    {
        QStringList splitpath = QStringList::split('/',url.path());
        QString artist = splitpath[1];
        kdDebug( 7182) << __func__ << ": Deleting artist " << artist << endl;
        status = connect();
        if( status != NJB_SUCCESS)
            return NJB_HANDLED;
        trackValueList::iterator it_track = trackList.begin();
        while (it_track != trackList.end())
        {
            if ( (*it_track).getArtist() == artist )
            {
                status = NJB_Delete_Track( m_njb, (*it_track).getId());
                if( status != NJB_SUCCESS) {
                    kdDebug( 7182) << __func__ << ": NJB_Delete_Track failed" << endl;
                    return ERR_CANNOT_DELETE;
                }
                it_track = trackList.remove(it_track);
            }
            else
                ++it_track;
        }
        return NJB_HANDLED;
    }

    trackValueList::iterator it_track = theTracks->findTrackByName( url.fileName() );
    if( it_track == theTracks->end() )
        return ERR_DOES_NOT_EXIST;

    status = connect();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;
    status = NJB_Delete_Track( m_njb, (*it_track).getId());
    if( status != NJB_SUCCESS) {
        kdDebug( 7182) << __func__ << ": NJB_Delete_Track failed" << endl;
        return ERR_CANNOT_DELETE;
    }

    // remove from the cache
    trackList.remove(it_track);

    kdDebug( 7182) << __func__ << ": OK" << endl;

    return NJB_HANDLED;
}


/* ------------------------------------------------------------------------ */
void
kio_njbProtocol::del( const KURL& url, bool /*isfile*/)
{
    kdDebug( 7182) << __func__ << ": " << url.prettyURL() << endl;

    int status = NJB_NOTHANDLED;
    if( status == NJB_NOTHANDLED) status = delEtc( url);
    if( status == NJB_NOTHANDLED) status = delPlaylist( url);
    if( status == NJB_NOTHANDLED) status = delTrack( url);
    if( status == NJB_NOTHANDLED) status = delFile( url);
    kdDebug( 7182) << __func__ << ": status = " << status << endl;
    if( status == NJB_HANDLED)
        finished();
    else
        err( status ? status : ERR_DOES_NOT_EXIST, url.fileName());
    disconnect();
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::putEtc( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if( url.path() != "/etc/owner")
        return NJB_NOTHANDLED;

    int status = connect();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;
    kdDebug( 7182) << "put datareq" << endl;
    dataReq();
    kdDebug( 7182) << "put readdata" << endl;
    QCString owner( OWNER_STRING_LENGTH);
    int len = readData( owner);
    if( len < 0)
        return ERR_COULD_NOT_READ;
    owner.resize( len);
    kdDebug( 7182) << "put owner: " << owner << endl;
    status = NJB_Set_Owner_String( m_njb, owner);
    if( status) {
        kdDebug( 7182) << __func__ << ": NJB_Set_Owner_String failed";
        return ERR_COULD_NOT_WRITE;
    }
    return NJB_HANDLED;
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::putPlaylist( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;	
    if( url.directory() != "/playlists")
        return NJB_NOTHANDLED;
    kdDebug( 7182) << __func__ << ": " << url.fileName() << endl;

    int status = readJukeboxMusic();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    status = connect();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    // read in the file
    KTempFile tmpfile;
    unsigned length = 0;
    for(;;) {
        QByteArray buf;
        dataReq();
        int len = readData( buf);
        if( len < 0)
            return ERR_COULD_NOT_READ;
        if( !len)
            break;
        tmpfile.dataStream()->writeRawBytes( buf.data(), len);
        length += len;
    }
    tmpfile.close();
    kdDebug( 7182) << __func__ << ": read " << length << " bytes into " << tmpfile.name() << endl;

    // FIXME (acejones) Just keep the entire playlist in memory instead of writing to a tempfile
    QFile file(tmpfile.name());
    if ( ! file.open(IO_ReadOnly) )
    {
        return NJB_HANDLED;
    }
    QTextStream stream( &file );

    Playlist playlist;
    status = playlist.setName( url.fileName());

    if( status != NJB_SUCCESS)
    {
        file.close();
        return status; // pass this through, it may be an ERR_*
    }
    for(;;) {
        QString line = stream.readLine();
        if( line.isEmpty())
            break;

        status = playlist.addTrack( line);
        if( status == NJB_FAILURE)
        {
            int result = messageBox( SlaveBase::WarningContinueCancel,
                    i18n( "Could not find track in library\n%1").arg( line),
                    i18n( "Uploading Playlist"),
                    i18n( "C&ontinue"));
            if( result == KMessageBox::Cancel)
                return ERR_COULD_NOT_WRITE;
        }
        else if( status != NJB_SUCCESS)
        {
            file.close();
            return status;
        }
    }
    status = playlist.update();
    if( status != NJB_SUCCESS)
    {
        file.close();
        return status;
    }
    file.close();

    // update cache
    playlistList.readFromDevice();
    return NJB_HANDLED;
}



/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::putFile( const KURL& url, bool overwrite)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;	
    if(!url.directory().startsWith("/data"))
        return NJB_NOTHANDLED;

    if( m_busy)
        return ERR_COULD_NOT_CONNECT;

    int status = readJukeboxDatas();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    QString targetPath = url.path(-1).mid(5).replace('/', '\\');
    QString targetFolder = targetPath.left(targetPath.findRev('\\')) + '\\';
    kdDebug(7182) << __func__ << " path=" << targetPath << " folder=" << targetFolder << endl;

    dataFileValueList::const_iterator it;
    for (it = dataFileList.begin(); it != dataFileList.end(); it++)
    {
        QString filePath = (*it).folder + ( *it).filename;
        if (filePath == targetPath || (*it).folder.startsWith(targetPath + '\\'))
            break;
    }

    if (it != dataFileList.end())
    {
        if (overwrite)
            delFile(url);
        else
            return ERR_FILE_ALREADY_EXIST;
    }

    // read in the file
    KTempFile tmpfile;
    unsigned length = 0;
    for (;;)
    {
        QByteArray buf;
        dataReq();
        int len = readData( buf);
        if (len < 0)
        {
            tmpfile.close();
            QDir().remove( tmpfile.name());
            return ERR_COULD_NOT_READ;
        }
        if (!len)
            break;
        tmpfile.dataStream()->writeRawBytes( buf.data(), len);
        length += len;
    }
    tmpfile.close();
    kdDebug( 7182) << __func__ << ": read " << length << " bytes into " << tmpfile.name() << endl;
    QString src = tmpfile.name();

    status = connect();
    if (status != NJB_SUCCESS)
    {
        QDir().remove( src);
        return NJB_HANDLED;
    }

    // send the track
    kdDebug( 7182) << __func__ << ": sending..." << endl;
    m_busy = true;
    infoMessage( i18n( "Sending <b>%1</b> to the NJB\nPlease wait...").arg(
                url.fileName()));
    u_int32_t id;
    m_progressStart = time( 0);
    m_progressMessage = "Sent %1%...";

    status = NJB_Send_File(m_njb, src, url.fileName(), targetFolder,
            progressCallback, this, &id);

    if( status != NJB_SUCCESS)
    {
        kdDebug( 7182) << __func__ << ": NJB_Send_File failed\n";
        if (NJB_Error_Pending(m_njb)) 
        {
            const char* error;
            while ((error = NJB_Error_Geterror(m_njb)))
                kdError( 7182) << __func__ << ": " << error << endl;
        }
        else
            kdDebug( 7182) << __func__ << ": No reason for failure reported.\n";

        m_busy = false;
        QDir().remove( src);
        return ERR_COULD_NOT_WRITE;
    }
    m_busy = false;

    // FIXME (acejones) I'm not sure this is right.  If putFile is called to overwrite
    // an exising file, it looks like the cache will get TWO copies of the file.

    // update cache
    DataFile dataFile;
    dataFile.filename = url.filename();
    dataFile.folder	  = targetFolder;
    dataFile.dfid     = id;
    dataFile.filesize = length;
    dataFileList.append(dataFile);

    // cleanup
    QDir().remove( src);

    return NJB_HANDLED;
}

/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::putTrack( const KURL& dest, bool overwrite)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << " dest=" << dest.prettyURL() << endl;

    if( !dest.path().startsWith("/all/") &&
            !dest.path().startsWith( "/artists/") &&
            !(dest.path().startsWith( "/albums/") && dest.path(-1).contains("/") == 3))
        return NJB_NOTHANDLED;

    if( m_busy) {
        // njb is already in the middle of a transfer
        return ERR_COULD_NOT_CONNECT;
    }

    int status = readJukeboxMusic();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    trackValueList::const_iterator it_track = theTracks->findTrackByName( dest.fileName() );
    if( it_track != theTracks->end() ) 
    {
        if( overwrite)
            delTrack( dest);
        else
        {
            kdDebug( 7182) << __func__ << ": file already exists" << endl;
            return ERR_FILE_ALREADY_EXIST;
        }
    }

    // read in the file
    KTempFile tmpfile;
    unsigned length = 0;
    for(;;) {
        QByteArray buf;
        dataReq();
        int len = readData( buf);
        if( len < 0)
        {
            tmpfile.close();
            QDir().remove( tmpfile.name());
            return ERR_COULD_NOT_READ;
        }
        if( !len)
            break;
        tmpfile.dataStream()->writeRawBytes( buf.data(), len);
        length += len;
    }
    tmpfile.close();
    kdDebug( 7182) << __func__ << ": read " << length << " bytes into " << tmpfile.name() << endl;
    QString src = tmpfile.name();

    // read the mp3 header
    unsigned duration = getDuration( src);
    if( !duration) {
        kdDebug( 7182) << __func__ << ": " << src << " is not a valid mp3 file." << endl;
        m_errMsg = i18n( "Not a valid mp3 file");
        QDir().remove( src);
        return ERR_COULD_NOT_READ;
    }
    // read the id3 tags
    ID3_Tag tag;
    tag.Link(src);
    Track taggedTrack( tag);
    // filename
    taggedTrack.setFilename( dest.fileName() );
    // file size in bytes
    taggedTrack.setSize( QFileInfo(src).size() );
    // duration in seconds
    taggedTrack.setDuration( duration );

    status = connect();
    if( status != NJB_SUCCESS)
    {
        QDir().remove( src);
        return NJB_HANDLED;
    }

    // send the track
    //kdDebug( 7182) << __func__ << ": sending " << taggedTrack.getTitle() << "..." << endl;
    m_busy = true;
    kdDebug( 7182) << __func__ << ": sending " << src << " as "
        << dest.fileName().latin1() << " info: "
        << taggedTrack.getTitle() << " / " << taggedTrack.getAlbum() << " / "
        << taggedTrack.getGenre() << " / " << taggedTrack.getArtist() << endl;
    infoMessage( i18n( "Sending <b>%1</b> to the NJB\nPlease wait...").arg(dest.fileName()));
    u_int32_t id;
    m_progressStart = time( 0);
    m_progressMessage = "Sent %1%...";

    njb_songid_t* songid = NJB_Songid_New();
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Filename(dest.fileName().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Filesize(taggedTrack.getSize()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Codec(taggedTrack.getCodec().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Title(taggedTrack.getTitle().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Album(taggedTrack.getAlbum().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Genre(taggedTrack.getGenre().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Artist(taggedTrack.getArtist().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Length(taggedTrack.getDuration()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Tracknum(taggedTrack.getTrackNum()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Year(taggedTrack.getYear().toUInt()));

    if (NJB_Send_Track (m_njb, src, songid, progressCallback, this, &id) != NJB_SUCCESS) 
    {
        kdDebug( 7182) << __func__ << ": NJB_Send_Track failed\n";
        if (NJB_Error_Pending(m_njb)) 
        {
            const char* error;
            while ((error = NJB_Error_Geterror(m_njb)))
                kdError( 7182) << __func__ << ": " << error << endl;
        }
        else
            kdDebug( 7182) << __func__ << ": No reason for failure reported.\n";

        m_busy = false;
        NJB_Songid_Destroy( songid );
        QDir().remove( src);
        return ERR_COULD_NOT_WRITE;
    }
    m_busy = false;
    NJB_Songid_Destroy( songid );

    // Update cache
    taggedTrack.setId( id );
    trackList.append( taggedTrack);

    // cleanup
    QDir().remove( src);

    kdDebug( 7182) << __func__ << ": OK\n";

    return NJB_HANDLED;
}


/* ----------------------------------------------------------------------- */
void
kio_njbProtocol::put( const KURL& url,
        int /*permissions*/, bool overwrite, bool /*resume*/)
{
    kdDebug( 7182) << __func__ << ": " << url.prettyURL() << endl;
    int status = 0;
    if( !status) status = putEtc( url);
    if( !status) status = putPlaylist( url);
    if( !status) status = putTrack( url, overwrite);
    if( !status) status = putFile( url, overwrite);
    kdDebug( 7182) << __func__ << ": status = " << status << endl;
    if( status < 0)
        finished();
    else
        err( status ? status : ERR_COULD_NOT_WRITE, url.fileName());
    disconnect();
}


/* ----------------------------------------------------------------------- */
int
kio_njbProtocol::copyTrack( const KURL& src, const KURL& dst, bool overwrite)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << " src=" << src.prettyURL() << " dst=" << dst.prettyURL() << endl;
    if( dst.directory() != "/all" &&
            !dst.path().startsWith( "/artists/") &&
            !(dst.path().startsWith( "/albums/") && dst.path(-1).contains("/") == 3))
        return NJB_NOTHANDLED;

    if( m_busy) {
        // njb is already in the middle of a transfer
        kdDebug( 7182) << __func__ << ": njb is already in the middle of a transfer" << endl;
        return ERR_COULD_NOT_CONNECT;
    }

    int status = readJukeboxMusic();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    // FIXME (acejones) shouldn't we be searching for dst.filename()?
    trackValueList::const_iterator it_track = theTracks->findTrackByName( src.fileName() );
    if( it_track != theTracks->end() ) {
        if( overwrite)
            delTrack( dst);
        else
            return ERR_FILE_ALREADY_EXIST;
    }

    status = connect();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    // read the mp3 header
    unsigned duration = getDuration( src.path());
    if( !duration) {
        m_errMsg = i18n( "Not a valid mp3 file");
        return ERR_COULD_NOT_READ;
    }
    // read the id3 tags
    ID3_Tag tag;
    tag.Link(src.path());
    Track taggedTrack( tag);
    taggedTrack.setSize( QFileInfo( src.path()).size() );
    taggedTrack.setDuration( duration );
    taggedTrack.setFilename( src.fileName() );

    // send the track
    totalSize( taggedTrack.getSize());
    kdDebug( 7182) << "copyTrack: sending..." << endl;
    kdDebug( 7182) << "copyTrack: "
        << taggedTrack.getTitle() << " " << taggedTrack.getAlbum() << " "
        << taggedTrack.getGenre() << " " 
        << "size:" << taggedTrack.getSize() << " " 
        << taggedTrack.getArtist() << endl;
    u_int32_t id;
    m_progressStart = time( 0);
    m_progressMessage = "Copying / Sent %1%...";

    njb_songid_t* songid = NJB_Songid_New();
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Filename(dst.filename().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Filesize(taggedTrack.getSize()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Codec(taggedTrack.getCodec().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Title(taggedTrack.getTitle().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Album(taggedTrack.getAlbum().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Genre(taggedTrack.getGenre().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Artist(taggedTrack.getArtist().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Length(taggedTrack.getDuration()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Tracknum(taggedTrack.getTrackNum()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Year(taggedTrack.getYear().toUInt()));

    m_busy = true;
    kdDebug( 7182) << __func__ << ": m_njb is " << m_njb << "\n";
    if (NJB_Send_Track (m_njb, src.path(), songid, progressCallback, this, &id) != NJB_SUCCESS) 
    {
        kdDebug( 7182) << __func__ << ": NJB_Send_Track failed\n";
        if (NJB_Error_Pending(m_njb)) 
        {
            const char* error;
            while ((error = NJB_Error_Geterror(m_njb)))
                kdError( 7182) << __func__ << ": " << error << endl;
        }
        else
            kdDebug( 7182) << __func__ << ": No reason for failure reported.\n";

        m_busy = false;
        NJB_Songid_Destroy( songid );

        return ERR_COULD_NOT_WRITE;
    }

    m_busy = false;
    NJB_Songid_Destroy( songid );
    taggedTrack.setId( id );

    // cache the track
    trackList.append( taggedTrack);;

    return NJB_HANDLED;
}


/* ----------------------------------------------------------------------- */
void
kio_njbProtocol::copy( const KURL& src, const KURL& dst,
        int /*permissions*/, bool overwrite)
{
    kdDebug( 7182) << __func__ << ": " << src.prettyURL() << " to "
        << dst.prettyURL() << endl;
    int status = NJB_NOTHANDLED;
    if( src.protocol() != "file")
        status = ERR_UNSUPPORTED_ACTION;
    if( status != NJB_NOTHANDLED) 
        status = copyTrack( src, dst, overwrite);
    kdDebug( 7182) << __func__ << ": status = " << status << endl;
    if( status == NJB_HANDLED)
        finished();
    else
        err( status ? status : ERR_UNSUPPORTED_ACTION, src.fileName());
    disconnect();
}


/* ----------------------------------------------------------------------- */
UDSEntry
kio_njbProtocol::createUDSEntry( const KURL& url)
{
#if 0 // very noisy
    kdDebug( 7182) << "createUDSEntry: " << url.prettyURL() << endl;
#endif
    UDSEntry entry;
    UDSAtom atom;

    int type = 0;
    const char* mime = 0;
    int access = 0444;
    int size = 0;

    if( url.path() == "/") {
        type = S_IFDIR;
        access = 0555;
    } else
        if( url.path( -1) == "/albums") {
            type = S_IFDIR;
            access = 0555;
        } else
            if( url.path( -1) == "/all") {
                type = S_IFDIR;
                access = 0777;
            } else
                if( url.path( -1) == "/artists") {
                    type = S_IFDIR;
                    access = 0777;
                } else
                    if( url.path( -1) == "/etc") {
                        type = S_IFDIR;
                        access = 0777;
                    } else
                        if( url.path( -1) == "/playlists") {
                            type = S_IFDIR;
                            access = 0777;
                        } else
                            if( url.path( -1) == "/data") {
                                type = S_IFDIR;
                                access = 0777;
                            } else
                                if((url.path().startsWith("/albums/") ||
                                            url.path().startsWith("/artists/")) &&
                                        url.fileName().right(4) == ".m3u") {
                                    // an album listing
                                    //Getting the size of the file
                                    //Note that this size calculation needs to be kept in sync with the
                                    //logic that getAlbum uses to build the file.
                                    QString albumName = url.filename();
                                    albumName = albumName.left(albumName.length()-4);
                                    uint albumSize = 0;
                                    trackValueList::const_iterator it;
                                    for( it = trackList.begin(); it != trackList.end(); it++)
                                        if( !QString::compare( (*it).getAlbum(), albumName))
                                            albumSize += ( *it).getFilename().length() + 1;
                                    type = S_IFREG;
                                    mime = "text/plain";
                                    access = 0444;
                                    size = albumSize;
                                } else
                                    if( (url.directory() == "/all" ||
                                                url.path().startsWith("/albums/") ||
                                                url.path().startsWith("/artists/")) &&
                                            url.fileName().right(4) == ".mp3"	) {
                                        // a track
                                        trackValueList::const_iterator it;
                                        for( it = trackList.begin(); it != trackList.end(); it++)
                                            if(!QString::compare( ( *it).getFilename(), url.fileName()))
                                                break;
                                        if( it == trackList.end()) {
                                            err( ERR_DOES_NOT_EXIST, url.path(-1));
                                            return UDSEntry();
                                        }
                                        type = S_IFREG;
                                        mime = "audio/x-mp3";
                                        access = 0666;
                                        size = ( *it).getSize();
                                    } else
                                        if( url.directory() == "/artists") {
                                            // an artist
                                            type = S_IFDIR;
                                            access = 0555;
                                        } else
                                            if( url.directory() == "/albums" ||
                                                    (url.path().startsWith( "/artists/") &&
                                                     url.path(-1).contains("/") == 3))
                                            {
                                                // album directory
                                                type = S_IFDIR;
                                                access = 0777;
                                            } else
                                                if( url.directory() == "/etc") {
                                                    type = S_IFREG;
                                                    mime = "text/plain";
                                                    if( url.fileName() == "owner")
                                                        access = 0666;
                                                    else
                                                        access = 0444;
                                                } else
                                                    if( url.directory() == "/playlists") {
                                                        type = S_IFREG;
                                                        mime = "text/plain";
                                                        access = 0666;
                                                    } else
                                                        if( url.path().startsWith("/data/")) {
                                                            QString targetPath = url.path(-1).mid(5).replace('/', '\\');
                                                            bool isDir = false;
                                                            dataFileValueList::const_iterator it;
                                                            for( it = dataFileList.begin(); it != dataFileList.end(); it++)
                                                            {
                                                                QString filePath = (*it).folder + ( *it).filename;
                                                                if(filePath == targetPath)
                                                                    break;
                                                                else if ((*it).folder.startsWith(targetPath + '\\'))
                                                                {
                                                                    isDir = true;
                                                                    break;
                                                                }
                                                            }

                                                            if (isDir)
                                                            {
                                                                type = S_IFDIR;
                                                                access = 0777;
                                                            }
                                                            else if(it == dataFileList.end())
                                                            {
                                                                err( ERR_DOES_NOT_EXIST, url.path(-1));
                                                                return UDSEntry();
                                                            }
                                                            else
                                                            {
                                                                type = S_IFREG;
                                                                access = 0666;
                                                                size = (*it).filesize;
                                                            }
                                                        } else
                                                            /* default */ {
                                                                err( ERR_DOES_NOT_EXIST, url.path(-1));
                                                                return UDSEntry();
                                                            }

                                                        atom.m_uds = UDS_NAME;
                                                        atom.m_str = url.fileName();
                                                        entry.append( atom);

                                                        atom.m_uds = UDS_FILE_TYPE;
                                                        atom.m_long = type;
                                                        entry.append( atom);

                                                        if( mime) {
                                                            atom.m_uds = UDS_MIME_TYPE;
                                                            atom.m_str = mime;
                                                            entry.append( atom);
                                                        }

                                                        atom.m_uds = UDS_ACCESS;
                                                        atom.m_long = access;
                                                        entry.append( atom);

                                                        atom.m_uds = UDS_SIZE;
                                                        atom.m_long = size;
                                                        entry.append( atom);

                                                        // (acejones) Testing.  I would like to include extra information about the
                                                        // songs in the 'extra' fields.  But sofar, this has not been working.

                                                        // 	atom.m_uds = UDS_EXTRA;
                                                        // 	atom.m_str = "Test 1";
                                                        // 	atom.m_long = 0L;
                                                        // 	entry.append( atom);
                                                        // 	
                                                        // 	atom.m_uds = UDS_EXTRA;
                                                        // 	atom.m_str = "Test 2";
                                                        // 	atom.m_long = 0L;
                                                        // 	entry.append( atom);

#if 0 // very noisy
                                                        kdDebug( 7182) << "createUDSEntry: " << url.prettyURL() << " : finished" << endl;
#endif

                                                        return entry;
}


/* ------------------------------------------------------------------------ */
UDSEntry
kio_njbProtocol::createUDSEntry( const KURL& url, const QString& filename)
{
#if 0 // very noisy
    kdDebug( 7182) << "createUDSEntry: "
        << url.prettyURL() << " " << filename << endl;
#endif
    KURL file( url);
    file.addPath( filename);
    return createUDSEntry( file);
}


/* ------------------------------------------------------------------------ */
void
kio_njbProtocol::stat( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": " << url.prettyURL() << endl;
    readJukeboxMusic();
    readJukeboxDatas();
    UDSEntry entry = createUDSEntry(url);
    if (entry.empty())
        return;
    kdDebug( 7182) << __func__ << ": stat OK." << endl;
    statEntry(entry);
    finished();
    disconnect();
    kdDebug( 7182) << __func__ << ": finished." << endl;
}

int
kio_njbProtocol::listTracks( const KURL& url, const QString& artist, const QString& album )
{
    kdDebug( 7182) << __func__ << ": url=" << url << ", "
        << "artist=" << artist << ", "
        << "album=" << album << endl;

    int status = readJukeboxMusic();
    if( status == NJB_SUCCESS)
    {
        trackValueList::const_iterator it;
        for( it = trackList.begin(); it != trackList.end(); it++)
            if ((artist.isNull() || (*it).getArtist() == artist) &&
                    (album.isNull() || (*it).getAlbum() == album))
                listEntry( createUDSEntry( url, ( *it).getFilename()), false);
    }

    return NJB_HANDLED;
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::listAlbums( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if( url.path( -1) != "/albums")
        return NJB_NOTHANDLED;

    int status = readJukeboxMusic();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    QStringList albumsList;
    trackValueList::const_iterator it;
    //Build a list with unique occurences of each album
    for( it = trackList.begin(); it != trackList.end(); it++)
        if( !albumsList.contains( ( *it).getAlbum()))
            albumsList.append( ( *it).getAlbum());

    QStringList::const_iterator album;
    for( album = albumsList.begin(); album != albumsList.end(); ++album)
        listEntry( createUDSEntry( url, *album), false);

    return NJB_HANDLED;
}

int
kio_njbProtocol::listAlbum( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if(!url.path( -1).startsWith("/albums/") &&
            !(url.path().startsWith( "/artists/") && url.path(-1).contains("/") == 3))
        return NJB_NOTHANDLED;

    QString artist = QString::null;
    if (url.path().startsWith( "/artists/"))
    {
        artist = url.directory().mid(9);
    }

    listTracks(url, artist, url.filename());

    // album playlist file
    listEntry(createUDSEntry(url, url.filename() + ".m3u"), false);

    return NJB_HANDLED;
}

/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::listAll( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if( url.path( -1) != "/all")
        return NJB_NOTHANDLED;

    listTracks(url, QString::null, QString::null);
    return NJB_HANDLED;
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::listArtists( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if( url.path( -1) != "/artists")
        return 0;

    int status = readJukeboxMusic();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    QStringList artistsList;
    trackValueList::const_iterator it;
    //Build a list with unique occurences of each artist
    for( it = trackList.begin(); it != trackList.end(); it++)
        if( !artistsList.contains( ( *it).getArtist()))
            artistsList.append( ( *it).getArtist());

    QStringList::const_iterator artist;
    for( artist = artistsList.begin(); artist != artistsList.end(); ++artist)
        listEntry( createUDSEntry( url, *artist), false);

    return NJB_HANDLED;
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::listArtist( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if( url.directory() != "/artists")
        return NJB_NOTHANDLED;

    listTracks(url, url.filename(), QString::null);

    QStringList albums;
    trackValueList::const_iterator it;
    for( it = trackList.begin(); it != trackList.end(); it++) {
        if(!QString::compare( ( *it).getArtist(), url.filename())) {
            if (!albums.contains(( *it).getAlbum()))
                albums.append(( *it).getAlbum());
        }
    }

    QStringList::const_iterator a;
    for (a = albums.begin(); a != albums.end(); ++a)
        listEntry(createUDSEntry(url, *a), false);

    return NJB_HANDLED;
}


/* ----------------------------------------------------------------------- */
int
kio_njbProtocol::listPlaylists( const KURL& url)
{
    kdDebug( 7182) << __PRETTY_FUNCTION__ << ": pid=" << getpid() << endl;
    if( url.path( -1) != "/playlists")
        return NJB_NOTHANDLED;

    int status = readJukeboxMusic();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    playlistValueList::const_iterator it;
    for( it = playlistList.begin(); it != playlistList.end(); it++)
    {
        kdDebug( 7182) << __PRETTY_FUNCTION__ << ": listing " << (*it).getName() << endl;
        listEntry( createUDSEntry( url, (*it).getName()), false);
    }

    return NJB_HANDLED;
}

/* ----------------------------------------------------------------------- */
int
kio_njbProtocol::listDatas( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    kdDebug( 7182) << __func__ << url << endl;

    if(!url.path( -1).startsWith("/data"))
        return NJB_NOTHANDLED;

    int status = readJukeboxDatas();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    QString dir = url.path(1).mid(5).replace('/', '\\');

    QStringList subDirs;
    dataFileValueList::const_iterator it;
    for( it = dataFileList.begin(); it != dataFileList.end(); it++)
    {
        // files in this directory
        if ((*it).folder == dir && !(*it).filename.isNull())
        {
            kdDebug(7182) << "listDatas Add" << endl;
            listEntry( createUDSEntry( url, ( *it).filename), false);
        }

        // subdirs of this directory
        if ((*it).folder.startsWith(dir))
        {
            // find next '\' and get subdir name
            QString subPath = (*it).folder.mid(dir.length());
            int slash = subPath.find('\\', 1);
            if (slash > 0)
            {
                QString subDir = subPath.left(slash);
                if (!subDirs.contains(subDir))
                    subDirs.append(subDir);
            }
        }
    }

    // create subdir entries
    QStringList::const_iterator d;
    for (d = subDirs.begin(); d != subDirs.end(); ++d)
        listEntry(createUDSEntry(url, *d), false);

    return NJB_HANDLED;
}

/* ----------------------------------------------------------------------- */
int
kio_njbProtocol::listEtc( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if( url.path( -1) != "/etc")
        return NJB_NOTHANDLED;

    totalSize( 5);

    listEntry( createUDSEntry( url, "counter"), false);
    listEntry( createUDSEntry( url, "diskfree"), false);
    listEntry( createUDSEntry( url, "eax"), false);
    listEntry( createUDSEntry( url, "firmware"), false);
    listEntry( createUDSEntry( url, "owner"), false);
    listEntry( createUDSEntry( url, "serial"), false);
    listEntry( createUDSEntry( url, "version"), false);
    return NJB_HANDLED;
}


/* ------------------------------------------------------------------------ */
int
kio_njbProtocol::listRoot( const KURL& url) {
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;
    if( url.path() != "/")
        return NJB_NOTHANDLED;

    totalSize( 6);
    listEntry( createUDSEntry( url, "albums/"), false);
    listEntry( createUDSEntry( url, "all/"), false);
    listEntry( createUDSEntry( url, "artists/"), false);
    listEntry( createUDSEntry( url, "etc/"), false);
    listEntry( createUDSEntry( url, "playlists/"), false);
    listEntry( createUDSEntry( url, "data/"), false);
    return NJB_HANDLED;
}


/* ------------------------------------------------------------------------ */
void
kio_njbProtocol::listDir( const KURL& url)
{
    kdDebug( 7182) << __func__ << ": " << url.prettyURL() << endl;

    int status = NJB_NOTHANDLED;
    if( status == NJB_NOTHANDLED ) status = listRoot( url);
    if( status == NJB_NOTHANDLED ) status = listAlbums( url);
    if( status == NJB_NOTHANDLED ) status = listAlbum( url);
    if( status == NJB_NOTHANDLED ) status = listAll( url);
    if( status == NJB_NOTHANDLED ) status = listArtists( url);
    if( status == NJB_NOTHANDLED ) status = listPlaylists( url);
    if( status == NJB_NOTHANDLED ) status = listEtc( url);
    if( status == NJB_NOTHANDLED ) status = listArtist( url);
    if( status == NJB_NOTHANDLED ) status = listDatas( url);
    kdDebug( 7182) << __func__ << ": status = " << status << endl;
    if( status == NJB_HANDLED ) {
        listEntry( UDSEntry(), true);
        finished();
    } else
        err( status ? status : ERR_DOES_NOT_EXIST, url.fileName());
    disconnect();
}

int
kio_njbProtocol::mkDataDir( const KURL& url, int /*permissions*/)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;	
    if(!url.directory().startsWith("/data"))
        return NJB_NOTHANDLED;

    if( m_busy)
        return ERR_COULD_NOT_CONNECT;

    int status = readJukeboxDatas();
    if( status != NJB_SUCCESS)
        return NJB_HANDLED;

    QString targetPath = url.path(-1).mid(5).replace('/', '\\');
    kdDebug( 7182) << __func__ << ": checking " << targetPath << endl;

    // see if it already exists
    dataFileValueList::const_iterator it;
    for( it = dataFileList.begin(); it != dataFileList.end(); it++)
    {
        QString filePath = (*it).folder + ( *it).filename;
        kdDebug( 7182) << __func__ << targetPath << ", " << filePath << ", " << (*it).folder << endl;
        if (filePath == targetPath)
        {
            err(ERR_FILE_ALREADY_EXIST, url.path(-1));
            break;
        }
        if ((*it).folder.startsWith(targetPath + '\\'))
        {
            err(ERR_DIR_ALREADY_EXIST, url.path(-1));
            break;
        }
    }

    if (it != dataFileList.end())
        return NJB_HANDLED;

    kdDebug( 7182) << __func__ << ": creating " << targetPath << endl;

    DataFile dataFile;
    dataFile.filename = QString::null; // use null filename to indicate empty dir
    dataFile.folder	  = targetPath + '\\';
    dataFile.dfid     = 0;
    dataFile.filesize = 0;
    dataFileList.append(dataFile);

    return NJB_HANDLED;
}

/* ------------------------------------------------------------------------ */
/** Allows a "mkdir" anywhere in the device.  This is useful so you can copy
 * whole directories of files up to the device. 
 */

void 
kio_njbProtocol::mkdir( const KURL& url, int permissions)
{
    kdDebug( 7182) << __func__ << ": " << url.prettyURL() << endl;

    // try making a data dir
    mkDataDir( url, permissions);

    // no matter what, tell the system that we succeeded, because it's
    // legal to make a directory anywhere on the device
    finished();

    disconnect();

    kdDebug( 7182) << __func__ << ": ok " << endl;
}

/* ------------------------------------------------------------------------ */
/** Transfer data files info from the njb to a local structure */
int
kio_njbProtocol::readJukeboxDatas( void)
{
    // FIXME (acejones) Move the work into datalist->readFromDevice

    if(dataFileList.isEmpty()) {
        kdDebug( 7182) << __func__ << ": reading from device\n";
        int status = connect();
        if( status != NJB_SUCCESS)
            return NJB_FAILURE;
        // Don't get extended metadatas
        NJB_Get_Extended_Tags(m_njb, 0);

        // Gestion des playlists
        NJB_Reset_Get_Datafile_Tag( m_njb);
        while( njb_datafile_t* dl = NJB_Get_Datafile_Tag( m_njb) ) {
            DataFile df;
            df.filename = dl->filename;
            df.folder		= dl->folder;
            df.dfid			= dl->dfid;
            df.filesize = dl->filesize;
            dataFileList.append(df);
            NJB_Datafile_Destroy(dl);
        }
    }

    kdDebug( 7182) << __func__ << ": cached\n";
    return NJB_SUCCESS;
}

/* ------------------------------------------------------------------------ */
/** Transfer musical info from the njb to local structures */
int
kio_njbProtocol::readJukeboxMusic( void)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;

    int result = NJB_SUCCESS;

    // First, read jukebox tracks
    if(trackList.isEmpty()) {
        int status = connect();
        if( status != NJB_SUCCESS)
            return status;

        int result = trackList.readFromDevice();

        // Then read jukebox playlists
        if ( result == NJB_SUCCESS )
            result = playlistList.readFromDevice();

        // After loading, disconnect.  This will give the player a chance to
        // catch up.  Otherwise we'll start trying to write while it's
        // still working on the track database, which seems to cause hard
        // system freezes
        disconnect();
    }

    kdDebug( 7182) << __func__ << ": return " << result << endl;
    return result;
}


void
kio_njbProtocol::special( const QByteArray & /*data*/ )
{
    kdDebug( 7182) << __func__ << ": disconnect on timeout" << endl;
    disconnect(true);
}
