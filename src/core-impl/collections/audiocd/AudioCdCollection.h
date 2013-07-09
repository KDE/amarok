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

#ifndef AUDIOCDCOLLECTION_H
#define AUDIOCDCOLLECTION_H

#include "core/collections/Collection.h"
#include "core/meta/Observer.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "meta/AudioCdTrack.h"
#include "helpers/MetaDataHelper.h"

#include <KDirWatch>
#include <KIcon>
#include <KUrl>
#include <ThreadWeaver/Job>

#include <kio/job.h>
#include <kio/netaccess.h>
#include <kio/udsentry.h>

#include <solid/block.h>
#include <solid/device.h>
#include <solid/opticaldisc.h>

#include <cdio/cdio.h>

class QAction;
class KJob;

using namespace Collections;

class KJob;
class AudioCdCollectionLoaderJob;

/**
 * Stores data for tracks and disc
 */
struct EntityInfo
{
    EntityInfo( const QString &defaultFiledValue = "" )
        : artist( defaultFiledValue )
        , title( defaultFiledValue )
        , year( defaultFiledValue )
        , genre( defaultFiledValue )
        , composer( defaultFiledValue )
    {
    }

    /**
     * For every field in @newInfo it checks that
     * this field isn't empty, then updates value of
     * this field
     */
    void update( const EntityInfo &newInfo );

    QString artist;
    QString title;
    QString year;
    QString genre;
    QString composer;
};

class AudioCdCollection: public Collection
{
    Q_OBJECT

    friend class AudioCdCollectionLoaderJob;

    public:

        enum { WAV, FLAC, OGG, MP3 } EncodingFormat;

        AudioCdCollection( const QString &udi );
        virtual ~AudioCdCollection();

        void setEncodingFormat( int format ) const;

        /* TrackProvider methods */
        virtual bool possiblyContainsTrack( const KUrl &url ) const;

        /* Collection methods */
        virtual QueryMaker *queryMaker();
        virtual QString uidUrlProtocol() const;

        virtual QString collectionId() const;
        virtual QString prettyName() const;
        virtual KIcon icon() const;

        virtual bool hasCapacity() const;
        virtual float totalCapacity() const;

        virtual bool isWritable() const { return false; }
        virtual bool isOrganizable() const { return isWritable(); };

        Collections::CollectionLocation* location();

        /* Capability-related methods */
        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability *createCapabilityInterface(
                Capabilities::Capability::Type type );

        virtual QSharedPointer<MemoryCollection> memoryCollection() const { return m_mc; }
        QString getDeviceName() const;
    signals:
        void loaded(bool, AudioCdCollection*);
    public slots:
        /**
         * Destroy the collection
         */
        void slotDestroy();
        /**
         * Destroy the collection and try to eject the device from system
         */
        void slotEject();
        /**
         * Emits remove() so that this collection is destroyed by
         * CollectionManager. No other method is allowed to emit remove()!
         */
        void slotRemove();
        /**
         * Checks if collection was loaded sucesfully and emits loaded
         */
        void slotCollectionLoaded();
        /**
         * Reloads collection using metadata
         * source specified by sender
         */
        void slotSetNewMetadata();
        void slotCollectionUpdated();
        /**
         * Shows possible encoding for collection
         */
        void showEncodingDialog();
        /**
         * Reloads the collection using
         * encoding specified by sender
         */
        void onEncodingSelected( QString& );
    private:
        KUrl audiocdUrl( const QString &path = "" ) const;
        /** Stops playback if current track came from CD*/
        void stopPlayback() const;
        /**
         * Inits collection loading.
         * Creates loader job and connects proper signals
         * ro the proper slots
         */
        void updateCollection();

        Solid::Block* getBlockDevice() const;
        Solid::Device m_device;

        QSharedPointer<MemoryCollection> m_mc;

        QString m_collectionId;
        QAction *m_ejectAction;
        QAction *m_setCDDB;
        QAction *m_setCDTEXT;
        QAction *m_encodingDialog;

        QString m_discCddbId;
        QMap<KUrl, AudioCdTrackPtr> m_tracks;
        QVector<QString> m_encodings;
        QString m_currentEncoding;
        QString m_currentMetadata;
        QByteArray m_sample;
};

/**
 * Used for threading during cd enumeration and metadata fetching.
 * Auto-deletes when its work is done.
 */
class AudioCdCollectionLoaderJob :  public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        AudioCdCollectionLoaderJob( AudioCdCollection *audiocd );

    protected:
        void run();

    private:
        bool initEnumeration( track_t& i_first_track, track_t& i_last_track, msf_t toc[CDIO_CDROM_LEADOUT_TRACK + 1] );
        /**
         * Return metadata source based on prefferences provided by user
         * or avalability of an information in a source.
         */
        MetaDataHelperPtr selectMetadataSource( CdIo_t* cdio, track_t firstTrack, track_t lastTrack,
                                                const QString& metaDataPreferences,
                                                const QString& encodingPreferences ) const;
        AudioCdCollection* m_audiocd;
        CdIo_t *m_cdio;
};
#endif
