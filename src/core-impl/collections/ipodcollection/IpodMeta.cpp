/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz                                       *
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

#include "IpodMeta.h"

#include "IpodCollection.h"
#include "IpodMetaEditCapability.h"
#include "jobs/WriteTagsJob.h"
#include "config-ipodcollection.h"
#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/ArtistHelper.h"
#include "FileType.h"

#include <KTemporaryFile>
#include <ThreadWeaver/Weaver>

#include <cmath>
#include <gpod/itdb.h>
#ifdef GDKPIXBUF_FOUND
#undef signals  // gdbusintrospection.h uses a member named signals, prevent build fail
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <QBuffer>
#endif


using namespace IpodMeta;

gpointer AmarokItdbUserDataDuplicateFunc( gpointer userdata )
{
    Q_UNUSED( userdata )
    return 0; // we never copy our userdata
}

Track::Track( Itdb_Track *ipodTrack )
    : m_track( ipodTrack )
    , m_tempImageFile( 0 )
{
    Q_ASSERT( m_track != 0 );
    m_track->usertype = m_gpodTrackUserTypeAmarokTrackPtr;
    m_track->userdata = this;
    m_track->userdata_duplicate = AmarokItdbUserDataDuplicateFunc;
}

Track::Track( const Meta::TrackPtr &origTrack )
    : m_track( itdb_track_new() )
    , m_tempImageFile( 0 )
{
    Q_ASSERT( m_track != 0 );
    m_track->usertype = m_gpodTrackUserTypeAmarokTrackPtr;
    m_track->userdata = this;
    m_track->userdata_duplicate = AmarokItdbUserDataDuplicateFunc;

    Meta::AlbumPtr origAlbum = origTrack->album();

    setTitle( origTrack->name() );
    // url is set in setCollection()
    setAlbum( origAlbum ? origAlbum->name() : QString() );
    setArtist( origTrack->artist() ? origTrack->artist()->name() : QString() );
    setComposer( origTrack->composer() ? origTrack->composer()->name() : QString() );
    setGenre( origTrack->genre() ? origTrack->genre()->name() : QString() );
    setYear( origTrack->year() ? origTrack->year()->year() : 0 );

    QString albumArtist;
    bool isCompilation = false;
    if ( origAlbum )
    {
        isCompilation = origAlbum->isCompilation();
        if( origAlbum->hasAlbumArtist() && origAlbum->albumArtist() )
            albumArtist = origAlbum->albumArtist()->name();

        if( origAlbum->hasImage() )
            setImage( origAlbum->image() );
    }
    if( isCompilation && albumArtist.isEmpty() )
        // iPod doesn't handle empy album artist well for compilation albums (splits these albums)
        albumArtist = i18n( "Various Artists" );
    else
        albumArtist = ArtistHelper::bestGuessAlbumArtist( albumArtist, artist()->name(),
                                                          genre()->name(), composer()->name() );
    setAlbumArtist( albumArtist );
    setIsCompilation( isCompilation, /* doCommit */ false );

    setBpm( origTrack->bpm() );
    setComment( origTrack->comment() );

    setScore( origTrack->score() );
    setRating( origTrack->rating(), /* doCommit */ false );

    setLength( origTrack->length() );
    // filesize is set in finalizeCopying(), which could be more accurate
    setSampleRate( origTrack->sampleRate() );
    setBitrate( origTrack->bitrate() );

    setCreateDate( QDateTime::currentDateTime() );  // createDate == added to collection
    setModifyDate( origTrack->modifyDate() );

    setTrackNumber( origTrack->trackNumber() );
    setDiscNumber( origTrack->discNumber() );

    setLastPlayed( origTrack->lastPlayed() );
    setFirstPlayed( origTrack->firstPlayed() );
    setPlayCount( origTrack->playCount() );

    setReplayGain( Meta::ReplayGain_Track_Gain, origTrack->replayGain( Meta::ReplayGain_Track_Gain ) );
    setReplayGain( Meta::ReplayGain_Track_Peak, origTrack->replayGain( Meta::ReplayGain_Track_Peak ) );
    setReplayGain( Meta::ReplayGain_Album_Gain, origTrack->replayGain( Meta::ReplayGain_Album_Gain ) );
    setReplayGain( Meta::ReplayGain_Album_Peak, origTrack->replayGain( Meta::ReplayGain_Album_Peak ) );

    setType( origTrack->type() );
    m_changedFields.clear();  // some of the set{Something} insert to m_changedFields, not
                              // desirable for constructor
}

Track::~Track()
{
    itdb_track_free( m_track );
    delete m_tempImageFile;
}

bool
Track::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    switch( type )
    {
        case Capabilities::Capability::Editable:
            return true;
        default:
            break;
    }
    return Meta::MetaCapability::hasCapabilityInterface( type );
}

Capabilities::Capability*
Track::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::Editable:
            return new EditCapability( KSharedPtr<Track>( this ) );
        default:
            break;
    }
    return Meta::MetaCapability::createCapabilityInterface( type );
}

QString
Track::name() const
{
    QReadLocker locker( &m_trackLock );
    return QString::fromUtf8( m_track->title );
}

void
Track::setTitle( const QString &newTitle )
{
    QWriteLocker locker( &m_trackLock );
    g_free( m_track->title );
    m_track->title = g_strdup( newTitle.toUtf8() );
    m_changedFields.insert( Meta::valTitle, newTitle );
}

KUrl
Track::playableUrl() const
{
    if( m_mountPoint.isEmpty() || !m_track->ipod_path || m_track->ipod_path[0] == '\0' )
        return KUrl();
    QReadLocker locker( &m_trackLock );
    gchar *relPathChar = g_strdup( m_track->ipod_path );
    itdb_filename_ipod2fs( relPathChar ); // in-place
    QString relPath = QFile::decodeName( relPathChar );
    g_free( relPathChar );
    return KUrl( m_mountPoint + relPath );
}

QString
Track::prettyUrl() const
{
    const KUrl &url = playableUrl();
    if( url.isLocalFile() )
        return url.toLocalFile();

    QString collName = m_coll ? m_coll.data()->prettyName() : i18n( "Unknown Collection" );
    QString artistName = artist() ? artist()->prettyName() : i18n( "Unknown Artist" );
    QString trackName = !name().isEmpty() ? name() : i18n( "Unknown track" );

    return  QString( "%1: %2 - %3" ).arg( collName, artistName, trackName );
}

QString
Track::uidUrl() const
{
    return QString( "%1://%2" )
           .arg( IpodCollection::s_uidUrlProtocol )
           .arg( playableUrl().toLocalFile() );
}

bool
Track::isPlayable() const
{
    KUrl trackUrl = playableUrl();
    QFileInfo trackFileInfo = QFileInfo( trackUrl.path() );
    if( trackFileInfo.exists() && trackFileInfo.isFile() && trackFileInfo.isReadable() )
        return true;

    return false;
}

Meta::AlbumPtr
Track::album() const
{
    // we may not store KSharedPtr to Album because it would create circular reference
    return Meta::AlbumPtr( new Album( const_cast<Track*>( this ) ) );
}

void
Track::setAlbum( const QString &newAlbum )
{
    QWriteLocker locker( &m_trackLock );
    g_free( m_track->album );
    m_track->album = g_strdup( newAlbum.toUtf8() );
    m_changedFields.insert( Meta::valAlbum, newAlbum );
}

void
Track::setAlbumArtist( const QString &newAlbumArtist )
{
    QWriteLocker locker( &m_trackLock );
    g_free( m_track->albumartist );
    m_track->albumartist = g_strdup( newAlbumArtist.toUtf8() );
    m_changedFields.insert( Meta::valAlbumArtist, newAlbumArtist );
}

void
Track::setIsCompilation( bool newIsCompilation, bool doCommit )
{
    // libgpod says: m_track->combination: True if set to 0x1, false if set to 0x0.
    if( m_track->compilation == newIsCompilation )
        return;  // nothing to do

    // we need to call commitChanges() without m_trackLock held, so create extra scope for it
    {
        QWriteLocker locker( &m_trackLock );
        m_track->compilation = newIsCompilation ? 0x1 : 0x0;
        m_changedFields.insert( Meta::valCompilation, newIsCompilation );
    }
    if( doCommit )
         // setIsCompilation() is not a part of EditCapability, we have to commit ourselves:
        commitChanges();
}

void
Track::setImage( const QImage &newImage )
{
    if( newImage.isNull() )
    {
        itdb_track_remove_thumbnails( m_track );
        delete m_tempImageFile;
        m_tempImageFile = 0;
        return;
    }

    // we set artwork even for devices that don't support it, everyone has new-enough iPod nowadays
    const int maxSize = AmarokConfig::writeBackCoverDimensions();
    QImage image;
    if( newImage.width() > maxSize || newImage.height() > maxSize )
        image = newImage.scaled( maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    else
        image = newImage;

    delete m_tempImageFile; // delete possible previous temporary file
    m_tempImageFile = new KTemporaryFile();
    m_tempImageFile->setSuffix( QString( ".png" ) );
    if( !m_tempImageFile->open() )
        return;
    // we save the file to disk rather than pass image data to save several megabytes of RAM
    if( !image.save( m_tempImageFile, "PNG" ) )
        return;
    /* this function remembers image path, it also fogots previous images (if any)
     * and sets artwork_size, artwork_count and has_artwork m_track fields */
    itdb_track_set_thumbnails( m_track, QFile::encodeName( m_tempImageFile->fileName() ) );
}

Meta::ArtistPtr
Track::artist() const
{
    QReadLocker locker( &m_trackLock );
    return Meta::ArtistPtr( new Artist( QString::fromUtf8( m_track->artist ) ) );
}

void
Track::setArtist( const QString &newArtist )
{
    QWriteLocker locker( &m_trackLock );
    g_free( m_track->artist );
    m_track->artist = g_strdup( newArtist.toUtf8() );
    m_changedFields.insert( Meta::valArtist, newArtist );
}

Meta::ComposerPtr
Track::composer() const
{
    QReadLocker locker( &m_trackLock );
    return Meta::ComposerPtr( new Composer( QString::fromUtf8( m_track->composer ) ) );
}

void
Track::setComposer( const QString &newComposer )
{
    QWriteLocker locker( &m_trackLock );
    g_free( m_track->composer );
    m_track->composer = g_strdup( newComposer.toUtf8() );
    m_changedFields.insert( Meta::valComposer, newComposer );
}

Meta::GenrePtr
Track::genre() const
{
    QReadLocker locker( &m_trackLock );
    return Meta::GenrePtr( new Genre( QString::fromUtf8( m_track->genre ) ) );
}

void
Track::setGenre( const QString &newGenre )
{
    QWriteLocker locker( &m_trackLock );
    g_free( m_track->genre );
    m_track->genre = g_strdup( newGenre.toUtf8() );
    m_changedFields.insert( Meta::valGenre, newGenre );
}

Meta::YearPtr
Track::year() const
{
    // no need for lock here, reading integer should be atomic
    return Meta::YearPtr( new Year( QString::number( m_track->year ) ) );
}

void Track::setYear( int newYear )
{
    QWriteLocker locker( &m_trackLock );
    m_track->year = newYear;
    m_changedFields.insert( Meta::valYear, newYear );
}

qreal
Track::bpm() const
{
    // no need for lock here, integer read
    return m_track->BPM;
}

void Track::setBpm( const qreal newBpm )
{
    QWriteLocker locker( &m_trackLock );
    m_track->BPM = newBpm;
    m_changedFields.insert( Meta::valBpm, newBpm );
}

QString
Track::comment() const
{
    QReadLocker locker( &m_trackLock );
    return QString::fromUtf8( m_track->comment );
}

void Track::setComment( const QString &newComment )
{
    QWriteLocker locker( &m_trackLock );
    g_free( m_track->comment );
    m_track->comment = g_strdup( newComment.toUtf8() );
    m_changedFields.insert( Meta::valComment, newComment );
}

double
Track::score() const
{
    return 0.0; // cannot be represented on iPod
}

void
Track::setScore( double newScore )
{
    Q_UNUSED(newScore)
}

int
Track::rating() const
{
    /* (rating/RATING_STEP) is a number of stars, Amarok uses numer of half-stars.
     * the order of multiply and divide operations is significant because of rounding */
    return ( ( m_track->rating * 2 ) / ITDB_RATING_STEP );
}

void
Track::setRating( int newRating, bool doCommit )
{
    newRating = ( newRating * ITDB_RATING_STEP ) / 2;
    if( newRating == (int) m_track->rating ) // casting prevents compiler waring about signedness
        return; // nothing to do, do not notify observers

    // we need to call commitChanges() without m_trackLock held, so create extra scope for it
    {
        QWriteLocker locker( &m_trackLock );
        m_track->rating = newRating;
        m_changedFields.insert( Meta::valRating, newRating );
    }
    if( doCommit )
        commitChanges(); // setRating() is not a part of EditCapability, we have to commit ourselves
}

qint64
Track::length() const
{
    return m_track->tracklen;
}

void
Track::setLength( qint64 newLength )
{
    m_track->tracklen = newLength;
}

int
Track::filesize() const
{
    return m_track->size;
}

int
Track::sampleRate() const
{
    return m_track->samplerate;
}

void
Track::setSampleRate( int newSampleRate )
{
    m_track->samplerate = newSampleRate;
}

int
Track::bitrate() const
{
    return m_track->bitrate;
}

void
Track::setBitrate( int newBitrate )
{
    m_track->bitrate = newBitrate;
}

QDateTime
Track::createDate() const
{
    time_t time = m_track->time_added;
    if( time == 0 )
        return QDateTime();  // 0 means "no reasonable time", so return invalid QDateTime
    return QDateTime::fromTime_t( time );
}

void
Track::setCreateDate( const QDateTime &newDate )
{
    m_track->time_added = newDate.isValid() ? newDate.toTime_t() : 0;
}

QDateTime
Track::modifyDate() const
{
    time_t time = m_track->time_modified;
    if( time == 0 )
        return QDateTime();  // 0 means "no reasonable time", so return invalid QDateTime
    return QDateTime::fromTime_t( time );
}

void Track::setModifyDate( const QDateTime &newDate )
{
    // this method _cannot_ lock m_trackLock or deadlock will occur in commitChanges()
    m_track->time_modified = newDate.isValid() ? newDate.toTime_t() : 0;
}

int
Track::trackNumber() const
{
    // no need for lock here, integer read
    return m_track->track_nr;
}

void
Track::setTrackNumber( int newTrackNumber )
{
    QWriteLocker locker( &m_trackLock );
    m_track->track_nr = newTrackNumber;
    m_changedFields.insert( Meta::valTrackNr, newTrackNumber );
}

int
Track::discNumber() const
{
    // no need for lock here, integer read
    return m_track->cd_nr;
}

void
Track::setDiscNumber( int newDiscNumber )
{
    QWriteLocker locker( &m_trackLock );
    m_track->cd_nr = newDiscNumber;
    m_changedFields.insert( Meta::valDiscNr, newDiscNumber );
}

QDateTime
Track::lastPlayed() const
{
    return m_track->time_played ? QDateTime::fromTime_t( m_track->time_played ) : QDateTime();
}

void
Track::setLastPlayed( const QDateTime &time )
{
    QWriteLocker locker( &m_trackLock );
    m_track->time_played = time.isValid() ? time.toTime_t() : 0;
    m_changedFields.insert( Meta::valLastPlayed, time );
}

QDateTime
Track::firstPlayed() const
{
    // we abuse time_released for this, which should be okay for non-podcast tracks
    // TODO: return QDateTime for podcast tracks
    return m_track->time_released ? QDateTime::fromTime_t( m_track->time_released ) : QDateTime();
}

void
Track::setFirstPlayed( const QDateTime &time )
{
    QWriteLocker locker( &m_trackLock );
    m_track->time_released = time.isValid() ? time.toTime_t() : 0;
    m_changedFields.insert( Meta::valFirstPlayed, time );
}

int
Track::playCount() const
{
    return m_track->playcount;
}

void
Track::setPlayCount( const int playcount )
{
    QWriteLocker locker( &m_trackLock );
    m_track->playcount = playcount;
    m_changedFields.insert( Meta::valLastPlayed, playcount );
}

qreal
Track::replayGain( Meta::ReplayGainTag mode ) const
{
    // iPods are not able to differentiante between different replay gain modes (track & album)
    switch( mode )
    {
        case Meta::ReplayGain_Track_Gain:
        case Meta::ReplayGain_Album_Gain:
            break; // fall to the computation
        case Meta::ReplayGain_Track_Peak:
        case Meta::ReplayGain_Album_Peak:
            return 0.0; // perhaps return -replayGain assuming there was enough headroom
    }

    if( m_track->soundcheck == 0 )
        return 0.0; // libgpod: The value 0 is special, treated as "no Soundcheck"
    // libgpod: X = 1000 * 10 ^ (-.1 * Y)
    // where Y is the adjustment value in dB and X is the value that goes into the SoundCheck field
    return 30.0 - 10.0 * std::log10( m_track->soundcheck );
}

void
Track::setReplayGain( Meta::ReplayGainTag mode, qreal newReplayGain )
{
    guint32 soundcheck;
    switch( mode )
    {
        case Meta::ReplayGain_Track_Gain:
            if( newReplayGain == 0.0 )
                // libgpod: The value 0 is special, treated as "no Soundcheck"
                soundcheck = 0;
            else
                // libgpod: X = 1000 * 10 ^ (-.1 * Y)
                // where Y is the adjustment value in dB and X is the value that goes into the SoundCheck field
                soundcheck = 1000 * std::pow( 10.0, -0.1 * newReplayGain );
            m_track->soundcheck = soundcheck;
            break;
        case Meta::ReplayGain_Album_Gain:
        case Meta::ReplayGain_Track_Peak:
            // we should somehow abuse Itdb_Track to store this, it is really needed
        case Meta::ReplayGain_Album_Peak:
            break;
    }
}

QString
Track::type() const
{
    QReadLocker locker( &m_trackLock );
    return QString::fromUtf8( m_track->filetype );
}

void
Track::setType( const QString &newType )
{
    QWriteLocker locker( &m_trackLock );
    g_free( m_track->filetype );
    m_track->filetype = g_strdup( newType.toUtf8() );
}

void Track::finishedPlaying( double playedFraction )
{
    // following code is more or less copied from SqlMeta::Track::finishedPlaying()
    bool doUpdate;
    if( length() < 60000 ) // less than 1min
        doUpdate = ( playedFraction >= 0.9 );
    else
        doUpdate = ( playedFraction >= 0.7 );
    if( !doUpdate )
        return;

    setPlayCount( playCount() + 1 );
    if( !firstPlayed().isValid() )
        setFirstPlayed( QDateTime::currentDateTime() );
    setLastPlayed( QDateTime::currentDateTime() );
    commitChanges();
}

bool
Track::inCollection() const
{
    return !m_coll.isNull();
}

Collections::Collection*
Track::collection() const
{
    return m_coll.data();
}

Meta::TrackPtr
Track::fromIpodTrack( const Itdb_Track *ipodTrack )
{
    if( !ipodTrack )
        return Meta::TrackPtr();
    if( ipodTrack->usertype != m_gpodTrackUserTypeAmarokTrackPtr )
        return Meta::TrackPtr();
    if( !ipodTrack->userdata )
        return Meta::TrackPtr();
    return Meta::TrackPtr( static_cast<Track *>( ipodTrack->userdata ) );
}

Itdb_Track*
Track::itdbTrack() const
{
    return m_track;
}

bool
Track::finalizeCopying( const gchar *mountPoint, const gchar *filePath )
{
    GError *error = 0;
    // we cannot use m_mountPoint, we are not yet in collection
    Itdb_Track *res = itdb_cp_finalize( m_track, mountPoint, filePath, &error );
    if( error )
    {
        warning() << "Failed to finalize copying of iPod track:" << error->message;
        g_error_free( error );
        error = 0;
    }
    return res == m_track;
}

void
Track::setCollection( QWeakPointer<IpodCollection> collection )
{
    m_coll = collection;
    if( !collection )
        return;
    { // scope for locker
        QWriteLocker locker( &m_trackLock );
        // paranoia: collection may become null while we were waiting for lock...
        m_mountPoint = collection ? collection.data()->mountPoint() : QString();
    }

    // m_track->filetype field may have been set by someone else, rather check it (if set
    // by us, it can be more accurate than file extension, so we prefer it)
    if( !Amarok::FileTypeSupport::possibleFileTypes().contains( type() ) )
        setType( Amarok::extension( playableUrl().path() ) );
        // we dont make the datbase dirty, this can be recomputed every time
}

void
Track::commitChanges()
{
    static const QSet<qint64> statFields = ( QSet<qint64>() << Meta::valFirstPlayed <<
        Meta::valLastPlayed << Meta::valPlaycount << Meta::valScore << Meta::valRating );

    QString path = playableUrl().path(); // needs to be here because it locks m_trackLock too
    // extra scope for m_trackLock locker:
    {
        QWriteLocker locker( &m_trackLock ); // guard access to m_changedFields
        if( m_changedFields.isEmpty() )
            return;

        if( AmarokConfig::writeBackStatistics() ||
            !(QSet<qint64>::fromList( m_changedFields.keys() ) - statFields).isEmpty() )
        {
            setModifyDate( QDateTime::currentDateTime() );
        }

        // write tags to file in a thread in order not to block
        WriteTagsJob *job = new WriteTagsJob( path, m_changedFields );
        job->connect( job, SIGNAL(done(ThreadWeaver::Job*)), job, SLOT(deleteLater()) );
        ThreadWeaver::Weaver::instance()->enqueue( job );

        m_changedFields.clear();
    }
    notifyObservers();
}

// IpodMeta:Album

Album::Album( Track *track )
    : m_track( track )
    , m_albumArtist( 0 ) // for lazy-reading album artist
{
}

Album::~Album()
{
    delete m_albumArtist;
}

QString Album::name() const
{
    QReadLocker locker( &m_track->m_trackLock );
    return QString::fromUtf8( m_track->m_track->album );
}

bool
Album::isCompilation() const
{
    readAlbumArtist();
    if( m_albumArtist->isEmpty() )
        // set compilation flag, otherwise the album would be invisible in collection
        // browser if "Album Artist / Album" view is selected.
        return true;
    return m_track->m_track->compilation == 0x1;
}

bool
Album::hasAlbumArtist() const
{
    readAlbumArtist();
    return !m_albumArtist->isEmpty();
}

Meta::ArtistPtr
Album::albumArtist() const
{
    readAlbumArtist();
    if( m_albumArtist->isEmpty() )
        return Meta::ArtistPtr();
    return Meta::ArtistPtr( new Artist( *m_albumArtist ) );
}

bool
Album::hasImage( int size ) const
{
    Q_UNUSED(size)
    if( m_track->m_track->has_artwork != 0x01 )
        return false; // libgpod: has_artwork: True if set to 0x01, false if set to 0x02.
    return itdb_track_has_thumbnails( m_track->m_track ); // should be false if GdbPixBuf is not available
}

QImage
Album::image( int size ) const
{
    Q_UNUSED(size) // MemoryMeta does scaling for us

    QImage albumImage;
#ifdef GDKPIXBUF_FOUND
    do {
        if( m_track->m_track->has_artwork != 0x01 )
            break; // libgpod: has_artwork: True if set to 0x01, false if set to 0x02.

        // it reads "thumbnail", but this is the correct function to call
        GdkPixbuf *pixbuf = (GdkPixbuf*) itdb_track_get_thumbnail( m_track->m_track, -1, -1 );
        if( !pixbuf )
            break;
        if( gdk_pixbuf_get_colorspace( pixbuf ) != GDK_COLORSPACE_RGB )
        {
            warning() << __PRETTY_FUNCTION__ << "Unsupported GTK colorspace.";
            gdk_pixbuf_unref( pixbuf );
            break;
        }
        if( gdk_pixbuf_get_bits_per_sample( pixbuf ) != 8 )
        {
            warning() << __PRETTY_FUNCTION__ << "Unsupported number of bits per sample.";
            gdk_pixbuf_unref( pixbuf );
            break;
        }
        int n_channels = gdk_pixbuf_get_n_channels( pixbuf );
        bool has_alpha = gdk_pixbuf_get_has_alpha( pixbuf );
        QImage::Format format;
        if( n_channels == 4 && has_alpha )
            format = QImage::Format_ARGB32;
        else if( n_channels == 3 && !has_alpha )
            format = QImage::Format_RGB888;
        else
        {
            warning() << __PRETTY_FUNCTION__ << "Unsupported n_channels / has_alpha combination.";
            gdk_pixbuf_unref( pixbuf );
            break;
        }

        // cost cast needed to choose QImage constructor that takes read-only image data
        albumImage = QImage( const_cast<const uchar *>( gdk_pixbuf_get_pixels( pixbuf ) ),
                             gdk_pixbuf_get_width( pixbuf ),
                             gdk_pixbuf_get_height( pixbuf ),
                             gdk_pixbuf_get_rowstride( pixbuf ),
                             format );
        // force deep copy so that memory from gdk pixbuf can be unreferenced:
        albumImage.setDotsPerMeterX( 2835 );
        gdk_pixbuf_unref( pixbuf );
    } while( false );
#endif
    return albumImage;
}

void
Album::readAlbumArtist() const
{
    if( m_albumArtist )
        return;
    QReadLocker( &m_track->m_trackLock );
    /* We have to guess albumArtist, because setting empty album artist (like Amarok does
     * for compilations) breaks compilations feature on iPods. */
    m_albumArtist = new QString( ArtistHelper::bestGuessAlbumArtist(
        QString::fromUtf8( m_track->m_track->albumartist ),
        QString::fromUtf8( m_track->m_track->artist ),
        QString::fromUtf8( m_track->m_track->genre ),
        QString::fromUtf8( m_track->m_track->composer ) ) );
}
