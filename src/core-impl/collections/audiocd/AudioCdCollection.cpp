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
#define DEBUG_PREFIX "AudioCdCollection"

#include "AudioCdCollection.h"
#include "AudioCdCollectionLocation.h"

#include "amarokconfig.h"
#include "EngineController.h"
#include "EncodingSelectionDialog.h"
#include "dialogs/OrganizeCollectionDialog.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/MemoryMeta.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"
#include "covermanager/CoverFetcher.h"
#include "helpers/CDDBHelper.h"
#include "helpers/CDTEXTHelper.h"
#include "meta/AudioCdAlbum.h"

#include <KUrl>
#include <KEncodingProber>
#include <ThreadWeaver/Weaver>

#include <QTextCodec>

#include <solid/opticaldrive.h>

#include <cdio/mmc.h>
#include <cdio/cd_types.h>
#include <cdio++/enum.hpp>

void
EntityInfo::update( const EntityInfo &newInfo )
{
    if ( !newInfo.artist.isEmpty() ) artist = newInfo.artist;
    if ( !newInfo.title.isEmpty() ) title = newInfo.title;
    if ( !newInfo.year.isEmpty() ) year =  newInfo.year;
    if ( !newInfo.genre.isEmpty() ) genre = newInfo.genre;
    if ( !newInfo.composer.isEmpty() ) composer = newInfo.composer;
}

AudioCdCollection::AudioCdCollection( const QString &udi )
                 : Collection()
                 , m_device( udi )
                 , m_mc( new Collections::MemoryCollection() )
                 , m_collectionId( "AudioCd" )
{
    debug() << "Creating AudioCdCollection for device with udi: " << m_device.udi();

    m_ejectAction = new QAction( KIcon( "media-eject" ), i18n( "&Eject Device" ), this );
    m_ejectAction->setProperty( "popupdropper_svg_id", "eject" );
    connect( m_ejectAction, SIGNAL(triggered()), SLOT(slotEject()) );

    m_setCDDB = new QAction( i18n( "&Use CDDB Metadata" ), this );
    connect( m_setCDDB, SIGNAL(triggered()), SLOT(slotSetNewMetadata()) );

    m_setCDTEXT = new QAction( i18n( "&Use CD-TEXT Metadata" ), this );
    connect( m_setCDTEXT, SIGNAL(triggered()), SLOT(slotSetNewMetadata()) );

    m_encodingDialog = new QAction( i18n( "&Select Encoding" ), this );
    connect( m_encodingDialog, SIGNAL(triggered()), SLOT(showEncodingDialog()) );

    AudioCdCollectionLoaderJob *loader = new AudioCdCollectionLoaderJob( this );
    ThreadWeaver::Weaver::instance()->enqueue( loader );
    connect( loader, SIGNAL(done(ThreadWeaver::Job*)), this, SLOT(slotCollectionLoaded()) );
}

AudioCdCollection::~AudioCdCollection()
{
    DEBUG_BLOCK
}

Solid::Block*
AudioCdCollection::getBlockDevice() const
{
    Solid::Device parent( m_device.parent() );
    if( !parent.isValid() )
    {
        debug() << "Device has no parent";
        return 0;
    }
    else
        return  parent.as<Solid::Block>();
}

QString
AudioCdCollection::getDeviceName() const
{
    QString name_device;
    Solid::Block* sb = getBlockDevice();
    if( !sb  )
        debug() << "Failed to get block device, returning null string as device name";
    else
        name_device = sb->device();
    return name_device;
}

KUrl
AudioCdCollection::audiocdUrl( const QString &path ) const
{
    KUrl url("audiocd:/");
    url.addPath( path );
    QString name_device = getDeviceName();

    if( !name_device.isEmpty() )
        url.addQueryItem( "device", name_device );

    return url;
}

void
AudioCdCollection::updateCollection()
{
    DEBUG_BLOCK

    AudioCdCollectionLoaderJob *loader = new AudioCdCollectionLoaderJob( this );
    ThreadWeaver::Weaver::instance()->enqueue( loader );
    connect( loader, SIGNAL(done(ThreadWeaver::Job*)), this, SLOT(slotCollectionUpdated()) );
}


bool
AudioCdCollection::possiblyContainsTrack( const KUrl &url ) const
{
    return url.protocol() == "audiocd";
}

QueryMaker*
AudioCdCollection::queryMaker()
{
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

QString
AudioCdCollection::uidUrlProtocol() const
{
    return QString( "audiocd:/" );
}

QString
AudioCdCollection::collectionId() const
{
    return m_collectionId;
}

QString
AudioCdCollection::prettyName() const
{
    QString actualName;
    if( !m_collectionId.isEmpty() )
        actualName = m_collectionId;
    else if( !m_device.description().isEmpty() )
        actualName = m_device.description();
    else
    {
        actualName = m_device.vendor().simplified();
        if( !actualName.isEmpty() )
            actualName += " ";
        actualName += m_device.product().simplified();
    }
    return actualName;
}

KIcon
AudioCdCollection::icon() const
{
    if( m_device.icon().isEmpty() )
        return KIcon( "media-optical-audio" );
    else
        return KIcon( m_device.icon() );
}

bool
AudioCdCollection::hasCapacity() const
{
    return m_device.isValid() &&
           m_device.is<Solid::OpticalDisc>() &&
           m_device.as<Solid::OpticalDisc>()->isAppendable();
}

float
AudioCdCollection::totalCapacity() const
{
    return m_device.isValid() &&
           m_device.is<Solid::OpticalDisc>() &&
           m_device.as<Solid::OpticalDisc>()->capacity();
}

bool
AudioCdCollection::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
        case Capabilities::Capability::Transcode:
            return true;
        default:
            return false;
    }
}

Collections::CollectionLocation*
AudioCdCollection::location()
{
    return new AudioCdCollectionLocation( QWeakPointer<AudioCdCollection>( this ) );
}

Capabilities::Capability *
AudioCdCollection::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
        {
            QList<QAction *> actions;
            actions << m_ejectAction;
            actions << m_setCDDB;
            actions << m_setCDTEXT;
            actions << m_encodingDialog;
            return new Capabilities::ActionsCapability( actions );
        }
        default:
            return 0;
    }
}

void
AudioCdCollection::slotCollectionUpdated()
{
    emit updated();
    m_setCDDB->setEnabled( true );
    m_setCDTEXT->setEnabled( true );
}

void
AudioCdCollection::slotSetNewMetadata()
{
    DEBUG_BLOCK
    QAction *action = qobject_cast<QAction *>( sender() );
    if ( action == m_setCDDB)
        m_currentMetadata = "CDDB";
    if ( action == m_setCDTEXT )
        m_currentMetadata = "CDTEXT";
    m_setCDDB->setDisabled(true);
    m_setCDTEXT->setDisabled(true);
    updateCollection();
}

void
AudioCdCollection::onEncodingSelected( QString& encoding )
{
    m_currentEncoding = encoding;
    updateCollection();
}

void
AudioCdCollection::stopPlayback() const
{
    //we need to do a quick check if we are currently playing from this cd, if so, stop playback and then eject
    Meta::TrackPtr track = The::engineController()->currentTrack();

    if ( track && track->playableUrl().url().startsWith( "audiocd:/" ) )
        The::engineController()->stop();
}

void
AudioCdCollection::slotDestroy()
{
    DEBUG_BLOCK

    stopPlayback();
    slotRemove();
}

void
AudioCdCollection::slotEject()
{
    DEBUG_BLOCK

    stopPlayback();

    Solid::OpticalDrive *drive = m_device.parent().as<Solid::OpticalDrive>();
    if( drive )
        drive->eject();
    else
        debug() << "disc has no drive";

    slotRemove();
}

void
AudioCdCollection::slotRemove()
{
    DEBUG_BLOCK

    emit remove();
}

void
AudioCdCollection::slotCollectionLoaded()
{
    bool emptyCollection = true;
    if ( memoryCollection().data() )
        emptyCollection = memoryCollection().data()->trackMap().empty();
    emit loaded(!emptyCollection, this);
}

void
AudioCdCollection::showEncodingDialog()
{
    DEBUG_BLOCK
    
    EncodingSelectionDialog *dlg = new EncodingSelectionDialog( m_encodings, m_sample );
    connect( dlg, SIGNAL(encodingSelected(QString&)), this, SLOT(onEncodingSelected(QString&)) );
    dlg->show();
}

AudioCdCollectionLoaderJob::AudioCdCollectionLoaderJob( AudioCdCollection* audiocd )
    : m_audiocd( audiocd )
{
    connect( this, SIGNAL(done(ThreadWeaver::Job*)), this, SLOT(deleteLater()) );
}

bool
AudioCdCollectionLoaderJob::initEnumeration( track_t& firstTrack,
                                             track_t& lastTrack,
                                             msf_t toc[CDIO_CDROM_LEADOUT_TRACK + 1] )
{
    firstTrack = cdio_get_first_track_num( m_cdio );
    lastTrack = firstTrack + cdio_get_num_tracks( m_cdio );

    if ( CDIO_INVALID_TRACK == firstTrack || CDIO_INVALID_TRACK == lastTrack )
    {
        error() << "Get invalid tracks";
        return false;
    }

    for ( track_t iTrack = firstTrack; iTrack < CDIO_CDROM_LEADOUT_TRACK; iTrack++ )
        cdio_get_track_msf( m_cdio, iTrack, &toc[iTrack] );
    return true;
}

MetaDataHelperPtr
AudioCdCollectionLoaderJob::selectMetadataSource( CdIo_t* cdio, track_t firstTrack, track_t lastTrack,
                                                  const QString& metaDataPreferences,
                                                  const QString& encodingPreferences ) const
{
    if ( metaDataPreferences == "CDDB" )
    {
        CDDBHelperPtr cddb( new CDDBHelper( cdio, firstTrack, lastTrack, encodingPreferences ) );
        return MetaDataHelperPtr::staticCast( cddb );
    }
    else if ( metaDataPreferences == "CDTEXT" )
    {
        CDTEXTHelperPtr cdtext( CDTEXTHelperPtr( new CDTEXTHelper( cdio, encodingPreferences ) ) );
        return MetaDataHelperPtr::staticCast( cdtext );
    }
    else
    {
        CDDBHelperPtr cddb( new CDDBHelper( cdio, firstTrack, lastTrack, encodingPreferences ) );
        CDTEXTHelperPtr cdtext( CDTEXTHelperPtr( new CDTEXTHelper( cdio, encodingPreferences ) ) );
        if ( cddb.data()->isAvailable() )
            return MetaDataHelperPtr::staticCast( cddb );
        else if ( cdtext.data()->isAvailable() )
            return MetaDataHelperPtr::staticCast( cdtext );
    }
    return MetaDataHelperPtr( 0 );
}

void
AudioCdCollectionLoaderJob::run()
{
    DEBUG_BLOCK

    // readCd
    if ( !( m_cdio = cdio_open( 0, DRIVER_DEVICE ) ) )
    {
        error() << "Libcdio couldn't find CD";
        return;
    }

    track_t firstTrack, lastTrack;
    msf_t toc[CDIO_CDROM_LEADOUT_TRACK + 1];

    if ( !initEnumeration( firstTrack, lastTrack, toc ) )
    {
        error() << "Error in track enumeration";
        return;
    }

    EntityInfo trackInfo( i18n( "Unknown" ) );
    EntityInfo discInfo( i18n( "Unknown" ) );

    MetaDataHelperPtr metadataSource = selectMetadataSource( m_cdio, firstTrack, lastTrack,
                                                          m_audiocd->m_currentMetadata,
                                                          m_audiocd->m_currentEncoding );

    m_audiocd->m_discCddbId = "unknown";
    if ( !metadataSource.isNull() )
        discInfo.update( metadataSource.data()->getDiscInfo() );

    if ( discInfo.title != "Unknown" )
        m_audiocd->m_sample = metadataSource.data()->getRawDiscTitle();

    for ( track_t trackNum = firstTrack; trackNum < lastTrack; trackNum++ )
    {
        // for example, in case of mixed data CD we are interested
        // only in tracks
        if ( TRACK_FORMAT_AUDIO != cdio_get_track_format( m_cdio, trackNum ) )
            continue;

        QString prefix( trackNum < 10 ? "0" : "" );
        trackInfo.title = "Track " % prefix % QString::number( trackNum );
        QString baseUrl = m_audiocd->uidUrlProtocol() %
                          m_audiocd->m_discCddbId % '/' % QString::number( trackNum );

        if ( !metadataSource.isNull() )
            trackInfo.update( metadataSource.data()->getTrackInfo( trackNum ) );
        
        qint64 seconds = cdio_audio_get_msf_seconds( &toc[trackNum + 1] ) -
                         cdio_audio_get_msf_seconds( &toc[trackNum] );

        debug() << "Got track with baseUrl" << baseUrl
                << "trackName" << trackInfo.title
                << "duration" << seconds;

        AudioCdTrackPtr trackPtr;
        if ( m_audiocd->m_tracks.contains( KUrl(baseUrl) ) )
            trackPtr = m_audiocd->m_tracks.value( KUrl(baseUrl) );
        else
        {
            trackPtr = AudioCdTrackPtr( new AudioCdTrack( m_audiocd, baseUrl ) );
            m_audiocd->m_tracks[KUrl(baseUrl)] = trackPtr;
        }
        AudioCdAlbumPtr albumPtr( new AudioCdAlbum( discInfo.title, discInfo.artist )  );

        trackPtr->setTrackNumber( trackNum );
        trackPtr->setLength( seconds * 1000 );

        if ( trackInfo.artist.isEmpty() || trackInfo.artist == discInfo.artist )
            trackPtr->setArtist( discInfo.artist );
        else
        {
            albumPtr->setCompilation( true );
            trackPtr->setArtist( trackInfo.artist );
        }
        trackPtr->setTitle( trackInfo.title );
        trackPtr->setComposer( trackInfo.composer );
        trackPtr->setGenre( trackInfo.genre );
        trackPtr->setYear( trackInfo.year );
        trackPtr->setAlbum( albumPtr );

        Meta::TrackPtr metaTrackPtr( Meta::TrackPtr::staticCast( trackPtr ) );
        if ( m_audiocd->memoryCollection().data()->trackMap().count() >= trackNum )
            MemoryMeta::MapChanger( m_audiocd->memoryCollection().data() ).trackChanged( metaTrackPtr );
        else
            MemoryMeta::MapChanger( m_audiocd->memoryCollection().data() ).addTrack( metaTrackPtr );
        
    }
    m_audiocd->m_encodings.clear();
    metadataSource.data()->getEncodings( m_audiocd->m_encodings );
}
