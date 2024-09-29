/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#define DEBUG_PREFIX "AudioCdCollection"

#include "AudioCdCollection.h"

#include "MainWindow.h"
#include "amarokconfig.h"
#include "AudioCdCollectionLocation.h"
#include "AudioCdMeta.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"
#include "covermanager/CoverFetcher.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "MediaDeviceMonitor.h"
#include "MemoryQueryMaker.h"
#include "handler/AudioCdHandler.h"
#include "support/AudioCdConnectionAssistant.h"
#include "support/AudioCdDeviceInfo.h"

#include <KIO/Job>
#include <KIO/ListJob>
#include <KIO/StatJob>
#include <KIO/StoredTransferJob>
#include <KIO/UDSEntry>

#include <solid/device.h>
#include <solid/opticaldrive.h>

#include <KConfigGroup>
#include <KEncodingProber>
#include <KSharedConfig>

#include <QDir>
#include <QTextCodec>
#include <QUrlQuery>

using namespace Collections;

static const QString unknownCddbId( QStringLiteral("unknown") );

AudioCdCollectionFactory::AudioCdCollectionFactory()
    : MediaDeviceCollectionFactory<AudioCdCollection>( new AudioCdConnectionAssistant() )
{}

AudioCdCollection::AudioCdCollection( MediaDeviceInfo* info )
   : MediaDeviceCollection()
   , m_encodingFormat( OGG )
{
    DEBUG_BLOCK
    // so that `amarok --cdplay` works:
    connect( this, &AudioCdCollection::collectionReady,
             this, &AudioCdCollection::checkForStartPlayRequest );

    debug() << "Getting Audio CD info";
    AudioCdDeviceInfo *cdInfo = qobject_cast<AudioCdDeviceInfo *>( info );
    m_udi = cdInfo->udi();
    m_device = cdInfo->device();

    readAudioCdSettings();

    m_handler = new Meta::AudioCdHandler( this );
}


AudioCdCollection::~AudioCdCollection()
{
}


QUrl
AudioCdCollection::audiocdUrl( const QString &path ) const
{
    QUrl url(QStringLiteral("audiocd:/") + path);

    if( !m_device.isEmpty() )
    {
        QUrlQuery query;
        query.addQueryItem( QStringLiteral("device"), m_device );
        url.setQuery( query );
    }

    return url;
}


void
AudioCdCollection::readCd()
{
    DEBUG_BLOCK
    //get the CDDB info file if possible.
    KIO::ListJob *listJob = KIO::listRecursive( audiocdUrl(), KIO::HideProgressInfo, false );
    connect( listJob, &KIO::ListJob::entries, this, &AudioCdCollection::audioCdEntries );
    connect( listJob, &KIO::ListJob::result, this, &AudioCdCollection::slotEntriesJobDone );
}

void
AudioCdCollection::audioCdEntries( KIO::Job *job, const KIO::UDSEntryList &list )
{
    DEBUG_BLOCK
    Q_UNUSED( job )
    for( KIO::UDSEntryList::ConstIterator it = list.begin(); it != list.end(); ++it )
    {
        const KIO::UDSEntry &entry = *it;
        QString name = entry.stringValue( KIO::UDSEntry::UDS_NAME );
        if( name.endsWith( QLatin1String(".txt") ) )
        {
            debug() << "got possible cddb entry" << name << "(" << entry.numberValue( KIO::UDSEntry::UDS_SIZE ) << ")";
            m_cddbTextFiles.insert( entry.numberValue( KIO::UDSEntry::UDS_SIZE ), audiocdUrl( name ) );
        }
    }
}

void
AudioCdCollection::slotEntriesJobDone( KJob *job )
{
    DEBUG_BLOCK
    if( job->error() )
        warning() << __PRETTY_FUNCTION__ << job->errorString() << job->errorText();

    if( m_cddbTextFiles.isEmpty() )
    {
        warning() << __PRETTY_FUNCTION__ << "haven't found .txt file under audiocd:/, but continuing";
        noInfoAvailable();
        return;
    }

    int biggestTextFile = m_cddbTextFiles.keys().last();
    QUrl url = m_cddbTextFiles.value( biggestTextFile );
    m_cddbTextFiles.clear(); // save memory
    KIO::StoredTransferJob *tjob = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
    connect( tjob, &KIO::StoredTransferJob::result, this, &AudioCdCollection::infoFetchComplete );
}

void
AudioCdCollection::infoFetchComplete( KJob *job )
{
    DEBUG_BLOCK
    if( job->error() )
    {
        error() << job->error() << job->errorString() << job->errorText();
        job->deleteLater();
        noInfoAvailable();
        return;
    }

    KIO::StoredTransferJob *tjob = static_cast<KIO::StoredTransferJob*>( job );
    QString cddbInfo;

    KEncodingProber prober;
    KEncodingProber::ProberState result = prober.feed( tjob->data() );
    if( result == KEncodingProber::FoundIt )
    {
        cddbInfo = QTextCodec::codecForName( prober.encoding() )->toUnicode( tjob->data() );
        debug() << "Encoding" << prober.encoding()<<"(confidence"<<prober.confidence()<<")";
    }
    else // Encoding detection failed. This is 2020's, try UTF8.
    {
        cddbInfo = QString::fromUtf8( tjob->data() );
        debug() << "Not sure about encoding (confidence" << prober.confidence() << "), so using UTF8.";
    }

    debug() << "got cddb info: " << cddbInfo;
    if (cddbInfo.length() == 0) {
        job->deleteLater();
        noInfoAvailable();
        return;
    }

    int startIndex;
    int endIndex;

    QString artist;
    QString album;
    QString year;
    QString genre;

    startIndex = cddbInfo.indexOf( QStringLiteral("DTITLE="), 0 );
    if ( startIndex != -1 )
    {
        startIndex += 7;
        endIndex = cddbInfo.indexOf( QStringLiteral("\n"), startIndex );
        QString compoundTitle = cddbInfo.mid( startIndex, endIndex - startIndex );

        debug() << "compoundTitle: " << compoundTitle;

        QStringList compoundTitleList = compoundTitle.split( QStringLiteral(" / ") );

        artist = compoundTitleList.at( 0 );
        album = compoundTitleList.at( 1 );
    }

    Meta::AudioCdArtistPtr artistPtr = Meta::AudioCdArtistPtr( new  Meta::AudioCdArtist( artist ) );
    memoryCollection()->addArtist( Meta::ArtistPtr::staticCast( artistPtr ) );
    Meta::AudioCdComposerPtr composerPtr = Meta::AudioCdComposerPtr( new Meta::AudioCdComposer( QString() ) );
    memoryCollection()->addComposer( Meta::ComposerPtr::staticCast( composerPtr ) );
    Meta::AudioCdAlbumPtr albumPtr = Meta::AudioCdAlbumPtr( new Meta::AudioCdAlbum( album ) );
    albumPtr->setAlbumArtist( artistPtr );
    memoryCollection()->addAlbum( Meta::AlbumPtr::staticCast( albumPtr ) );


    startIndex = cddbInfo.indexOf( QStringLiteral("DYEAR="), 0 );
    if ( startIndex != -1 )
    {
        startIndex += 6;
        endIndex = cddbInfo.indexOf( QStringLiteral("\n"), startIndex );
        year = cddbInfo.mid( startIndex, endIndex - startIndex );
    }

    Meta::AudioCdYearPtr yearPtr = Meta::AudioCdYearPtr( new Meta::AudioCdYear( year ) );
    memoryCollection()->addYear( Meta::YearPtr::staticCast( yearPtr ) );


    startIndex = cddbInfo.indexOf( QStringLiteral("DGENRE="), 0 );
    if ( startIndex != -1 )
    {
        startIndex += 7;
        endIndex = cddbInfo.indexOf( QStringLiteral("\n"), startIndex );
        genre = cddbInfo.mid( startIndex, endIndex - startIndex );
    }

    Meta::AudioCdGenrePtr genrePtr = Meta::AudioCdGenrePtr( new Meta::AudioCdGenre( genre ) );
    memoryCollection()->addGenre( Meta::GenrePtr::staticCast( genrePtr ) );

    m_discCddbId = unknownCddbId;

    startIndex = cddbInfo.indexOf( QStringLiteral("DISCID="), 0 );
    if ( startIndex != -1 )
    {
        startIndex += 7;
        endIndex = cddbInfo.indexOf( QStringLiteral("\n"), startIndex );
        m_discCddbId = cddbInfo.mid( startIndex, endIndex - startIndex );
    }

    //MediaDeviceMonitor::instance()->setCurrentCdId( m_discCddbId );

    //get the list of tracknames
    startIndex = cddbInfo.indexOf( QStringLiteral("TTITLE0="), 0 );
    if ( startIndex != -1 )
    {
        endIndex = cddbInfo.indexOf( QStringLiteral("\nEXTD="), startIndex );
        QString tracksBlock = cddbInfo.mid( startIndex, endIndex - startIndex );
        debug() << "Tracks block: " << tracksBlock;
        QStringList tracksBlockList = tracksBlock.split( QLatin1Char('\n') );

        int numberOfTracks = tracksBlockList.count();

        for ( int i = 0; i < numberOfTracks; i++ )
        {
            QString prefix = QStringLiteral("TTITLE") + QString::number( i ) + QLatin1Char('=');
            debug() << "prefix: " << prefix;
            QString trackName = tracksBlockList.at( i );
            trackName = trackName.remove( prefix );

            QString trackArtist;
            //check if a track artist is included in the track name:

            if ( trackName.contains( QStringLiteral(" / ") ) )
            {
                QStringList trackArtistList = trackName.split( QStringLiteral(" / ") );
                trackName = trackArtistList.at( 1 );
                trackArtist = trackArtistList.at( 0 );

            }

            debug() << "Track name: " << trackName;

            QString padding = (i + 1) < 10 ? QStringLiteral("0") : QString();

            QString baseFileName = m_fileNamePattern;
            debug() << "Track Base File Name (before): " << baseFileName;

            baseFileName.replace( QStringLiteral("%{title}"), trackName, Qt::CaseInsensitive );
            baseFileName.replace( QStringLiteral("%{number}"), padding  + QString::number( i + 1 ), Qt::CaseInsensitive );
            baseFileName.replace( QStringLiteral("%{albumtitle}"), album, Qt::CaseInsensitive );
            baseFileName.replace( QStringLiteral("%{trackartist}"), trackArtist, Qt::CaseInsensitive );
            baseFileName.replace( QStringLiteral("%{albumartist}"), artist, Qt::CaseInsensitive );
            baseFileName.replace( QStringLiteral("%{year}"), year, Qt::CaseInsensitive );
            baseFileName.replace( QStringLiteral("%{genre}"), genre, Qt::CaseInsensitive );

            //we hack the url so the engine controller knows what track on the CD to play..
            QUrl baseUrl = audiocdUrl( m_discCddbId + QLatin1Char('/') + QString::number( i + 1 ) );

            debug() << "Track Base File Name (after): " << baseFileName;
            debug() << "Track url: " << baseUrl;

            Meta::AudioCdTrackPtr trackPtr = Meta::AudioCdTrackPtr( new Meta::AudioCdTrack( this, trackName, baseUrl ) );

            trackPtr->setTrackNumber( i + 1 );
            trackPtr->setFileNameBase( baseFileName );
            trackPtr->setLength( trackLength( i + 1 ) );

            memoryCollection()->addTrack( Meta::TrackPtr::staticCast( trackPtr ) );

            artistPtr->addTrack( trackPtr );

            if ( trackArtist.isEmpty() )
                trackPtr->setArtist( artistPtr );
            else
            {
                albumPtr->setCompilation( true );

                Meta::AudioCdArtistPtr trackArtistPtr = Meta::AudioCdArtistPtr( new Meta::AudioCdArtist( trackArtist ) );
                trackArtistPtr->addTrack( trackPtr );
                trackPtr->setArtist( trackArtistPtr );
            }

            composerPtr->addTrack( trackPtr );
            trackPtr->setComposer( composerPtr );

            albumPtr->addTrack( trackPtr );
            trackPtr->setAlbum( albumPtr );

            genrePtr->addTrack( trackPtr );
            trackPtr->setGenre( genrePtr );

            yearPtr->addTrack( trackPtr );
            trackPtr->setYear( yearPtr );

        }
    }

    //lets see if we can find a cover for the album:
    if( AmarokConfig::autoGetCoverArt() )
        The::coverFetcher()->queueAlbum( Meta::AlbumPtr::staticCast( albumPtr ) );

    updateProxyTracks();
    Q_EMIT collectionReady( this );
}

void
AudioCdCollection::checkForStartPlayRequest()
{
    //be nice and check if MainWindow is just aching for an audio cd to start playing
    if( The::mainWindow()->isWaitingForCd() )
    {
        debug() << "Tell MainWindow to start playing us immediately.";
        The::mainWindow()->playAudioCd();
    }
}


QString
AudioCdCollection::trackBaseFileName( int i ) const
{
    return QStringLiteral( "Track%1" ).arg( i, 2, 10, QLatin1Char('0') );
}


QString
AudioCdCollection::trackWavFileName( int i ) const
{
    return trackBaseFileName( i ) + QStringLiteral(".wav");
}


QString
AudioCdCollection::trackDisplayName( int i ) const
{
    return i18n( "Track" ) + QLatin1Char(' ') + QString::number( i );
}


qint64
AudioCdCollection::trackLength( int i ) const
{
    QUrl kioUrl = audiocdUrl( trackWavFileName( i ) );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    KIO::StatJob *statJob = KIO::statDetails( kioUrl, KIO::StatJob::SourceSide );
#else
    KIO::StatJob *statJob = KIO::stat( kioUrl, KIO::StatJob::SourceSide);
#endif
    if ( statJob->exec() )
    {
        KIO::UDSEntry uds = statJob->statResult();
        qint64 samples = (uds.numberValue(KIO::UDSEntry::UDS_SIZE, 44) - 44) / 4;
        return (samples - 44) * 10 / 441;
    }
    return 0;
}

QString
AudioCdCollection::collectionId() const
{
    return QLatin1String( "AudioCd" );
}

QString
AudioCdCollection::prettyName() const
{
    return i18n( "Audio CD" );
}

QIcon
AudioCdCollection::icon() const
{
    return QIcon::fromTheme( QStringLiteral("media-optical-audio") );
}

void
AudioCdCollection::cdRemoved()
{
    Q_EMIT remove();
}

QString
AudioCdCollection::encodingFormat() const
{
    switch( m_encodingFormat )
    {
        case WAV:
            return QStringLiteral("wav");
        case FLAC:
            return QStringLiteral("flac");
        case OGG:
            return QStringLiteral("ogg");
        case MP3:
            return QStringLiteral("mp3");
    }
    return QString();
}

QString
AudioCdCollection::copyableFilePath( const QString &fileName ) const
{
    switch( m_encodingFormat )
    {
        case WAV:
            return audiocdUrl( fileName ).url();
        case FLAC:
            return audiocdUrl( QStringLiteral("FLAC/") + fileName ).url();
        case OGG:
            return audiocdUrl( QStringLiteral("Ogg Vorbis/") + fileName ).url();
        case MP3:
            return audiocdUrl( QStringLiteral("MP3/") + fileName ).url();
    }
    return QString();
}

void
AudioCdCollection::setEncodingFormat( int format ) const
{
    m_encodingFormat = format;
}

CollectionLocation *
AudioCdCollection::location()
{
    return new AudioCdCollectionLocation( this );
}

void
AudioCdCollection::eject()
{
    DEBUG_BLOCK

    //we need to do a quick check if we are currently playing from this cd, if so, stop playback and then eject
    Meta::TrackPtr track = The::engineController()->currentTrack();

    if ( track )
    {
        if( track->playableUrl().url().startsWith( QStringLiteral("audiocd:/") ) )
            The::engineController()->stop();
    }

    Solid::Device device = Solid::Device( m_udi );
    
    Solid::OpticalDrive *drive = device.parent().as<Solid::OpticalDrive>();
    if( drive )
        drive->eject();
    else
        debug() << "disc has no drive";
}

void
AudioCdCollection::noInfoAvailable()
{
    DEBUG_BLOCK

    m_discCddbId = unknownCddbId;

    //MediaDeviceMonitor::instance()->setCurrentCdId( m_discCddbId );

    QString artist = i18n( "Unknown" );
    QString album = i18n( "Unknown" );
    QString year = i18n( "Unknown" );
    QString genre = i18n( "Unknown" );

    Meta::AudioCdArtistPtr artistPtr = Meta::AudioCdArtistPtr( new Meta::AudioCdArtist( artist ) );
    memoryCollection()->addArtist( Meta::ArtistPtr::staticCast( artistPtr ) );
    Meta::AudioCdComposerPtr composerPtr = Meta::AudioCdComposerPtr( new Meta::AudioCdComposer( QString() ) );
    memoryCollection()->addComposer( Meta::ComposerPtr::staticCast( composerPtr ) );
    Meta::AudioCdAlbumPtr albumPtr = Meta::AudioCdAlbumPtr( new Meta::AudioCdAlbum( album ) );
    albumPtr->setAlbumArtist( artistPtr );
    memoryCollection()->addAlbum( Meta::AlbumPtr::staticCast( albumPtr ) );
    Meta::AudioCdYearPtr yearPtr = Meta::AudioCdYearPtr( new Meta::AudioCdYear( year ) );
    memoryCollection()->addYear( Meta::YearPtr::staticCast( yearPtr ) );
    Meta::AudioCdGenrePtr genrePtr = Meta::AudioCdGenrePtr( new Meta::AudioCdGenre( genre ) );
    memoryCollection()->addGenre( Meta::GenrePtr::staticCast( genrePtr ) );


    int i = 1;
    QString trackWav = trackWavFileName( i );

    // This will find also data tracks on mixed CDs:
    // a better way to discover the available audio tracks should be found
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    while( KIO::statDetails( audiocdUrl( trackWav ), KIO::StatJob::SourceSide )->exec() )
#else
    while( KIO::stat( audiocdUrl( trackWav ), KIO::StatJob::SourceSide )->exec() )
#endif
    {
        debug() << "got track url: " << audiocdUrl( trackWav );

        //we hack the url so the engine controller knows what track on the CD to play..
        QUrl baseUrl = audiocdUrl( m_discCddbId + QLatin1Char('/') + QString::number( i ) );

        Meta::AudioCdTrackPtr trackPtr = Meta::AudioCdTrackPtr( new Meta::AudioCdTrack( this, trackDisplayName( i ), baseUrl ) );

        trackPtr->setTrackNumber( i );
        trackPtr->setFileNameBase( trackBaseFileName( i ) );
        trackPtr->setLength( trackLength( i ) );

        memoryCollection()->addTrack( Meta::TrackPtr::staticCast( trackPtr ) );

        artistPtr->addTrack( trackPtr );
        trackPtr->setArtist( artistPtr );

        composerPtr->addTrack( trackPtr );
        trackPtr->setComposer( composerPtr );

        albumPtr->addTrack( trackPtr );
        trackPtr->setAlbum( albumPtr );

        genrePtr->addTrack( trackPtr );
        trackPtr->setGenre( genrePtr );

        yearPtr->addTrack( trackPtr );
        trackPtr->setYear( yearPtr );

        i++;
        trackWav = trackWavFileName( i );
    }

    updateProxyTracks();
    Q_EMIT collectionReady( this );
}

void
AudioCdCollection::readAudioCdSettings()
{
    KSharedConfigPtr conf = KSharedConfig::openConfig( QStringLiteral("kcmaudiocdrc") );
    KConfigGroup filenameConf = conf->group( QStringLiteral("FileName") );

    m_fileNamePattern = filenameConf.readEntry( "file_name_template", "%{trackartist} - %{number} - %{title}" );
    m_albumNamePattern = filenameConf.readEntry( "album_name_template", "%{albumartist} - %{albumtitle}" );
}

bool
AudioCdCollection::possiblyContainsTrack( const QUrl &url ) const
{
    return url.scheme() == QStringLiteral("audiocd");
}

Meta::TrackPtr
AudioCdCollection::trackForUrl( const QUrl &url )
{
    QReadLocker locker( memoryCollection()->mapLock() );
    if( memoryCollection()->trackMap().contains( url.url() ) )
        return memoryCollection()->trackMap().value( url.url() );

    QRegularExpression trackUrlScheme( QStringLiteral("^audiocd:/([a-zA-Z0-9]*)/([0-9]{1,})") );
    if( url.url().indexOf( trackUrlScheme ) != 0 )
    {
        warning() << __PRETTY_FUNCTION__ << url.url() << "doesn't have correct scheme" << trackUrlScheme;
        return Meta::TrackPtr();
    }
    QRegularExpressionMatch rmatch = trackUrlScheme.match( url.url() );

    const QString trackCddbId = rmatch.capturedTexts().value( 1 );
    const int trackNumber = rmatch.capturedTexts().value( 2 ).toInt();
    if( !trackCddbId.isEmpty() && trackCddbId != unknownCddbId &&
        !m_discCddbId.isEmpty() && m_discCddbId != unknownCddbId &&
        trackCddbId != m_discCddbId )
    {
        warning() << __PRETTY_FUNCTION__ << "track with cddbId" << trackCddbId
                  << "doesn't match our cddbId" << m_discCddbId;
        return Meta::TrackPtr();
    }

    for( const Meta::TrackPtr &track : memoryCollection()->trackMap() )
    {
        if( track->trackNumber() == trackNumber )
            return track;
    }

    warning() << __PRETTY_FUNCTION__ << "track with number" << trackNumber << "not found";
    return Meta::TrackPtr();
}

void
AudioCdCollection::updateProxyTracks()
{
    for( const QUrl &url : m_proxyMap.keys() )
    {

        QString urlString = url.url().remove( QStringLiteral("audiocd:/") );
        const QStringList &parts = urlString.split( QLatin1Char('/') );

        if( parts.count() != 2 )
            continue;

        const QString &discId = parts.at( 0 );

        if( discId != m_discCddbId )
            continue;

        const int trackNumber = parts.at( 1 ).toInt();

        for( const Meta::TrackPtr &track : memoryCollection()->trackMap().values() )
        {
            if( track->trackNumber() == trackNumber )
            {
                m_proxyMap.value( url )->updateTrack( track );
            }
        }

    }

    m_proxyMap.clear();
}

void AudioCdCollection::startFullScan()
{
    DEBUG_BLOCK
    readCd();
}
