/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
#include "MediaDeviceCollection.h"
#include "MemoryCollection.h"
#include "core-impl/meta/proxy/MetaProxy.h"

#include <QAction>
#include <QObject>
#include <QUrl>

#include <KIO/UDSEntry>

class MediaDeviceInfo;
class KJob;
namespace Collections {

class AudioCdCollection;

class AudioCdCollectionFactory : public MediaDeviceCollectionFactory<AudioCdCollection>
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_collection-audiocdcollection.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

public:
    AudioCdCollectionFactory();
    ~AudioCdCollectionFactory() override {}

/*    virtual void init();

private Q_SLOTS:
    void audioCdAdded( const QString &uid );
    void deviceRemoved( const QString &uid );

private:

    QString m_currentUid;
    AudioCdCollection * m_collection;*/

};


/**
 * This is a Memorycollection subclass that uses the KIO audiocd:/ slave to
 * populate itself whenever it detects a CD.
 *
 * @author Nikolaj Hald Nielsen <nhn@kde.org>
 */
class AudioCdCollection : public MediaDeviceCollection
{
    Q_OBJECT
public:

    enum { WAV, FLAC, OGG, MP3 } EncodingFormat;

    explicit AudioCdCollection( MediaDeviceInfo* info );
    ~AudioCdCollection() override;

    QString encodingFormat() const;
    QString copyableFilePath( const QString &fileName ) const;

    void setEncodingFormat( int format ) const;

    QString collectionId() const override;
    QString prettyName() const override;
    QIcon icon() const override;

    CollectionLocation* location() override;

    bool possiblyContainsTrack( const QUrl &url ) const override;
    Meta::TrackPtr trackForUrl( const QUrl &url ) override;

    void cdRemoved();

    virtual void startFullScan(); //Override this one as I really don't want to move parsing to the handler atm.
    void startFullScanDevice() override { startFullScan(); }

public Q_SLOTS:
    void eject() override;

private Q_SLOTS:
    void audioCdEntries( KIO::Job *job, const KIO::UDSEntryList &list );
    void slotEntriesJobDone( KJob *job );
    void infoFetchComplete( KJob *job );
    void checkForStartPlayRequest();

private:
    void readAudioCdSettings();

    // Helper function to build the audiocd url.
    QUrl audiocdUrl( const QString &path = QStringLiteral("") ) const;
    // The file name of the track without extension
    QString trackBaseFileName( int i ) const;
    // The file name of the track in .wav format
    QString trackWavFileName( int i ) const;
    // The name of the track that should be displayed
    QString trackDisplayName( int i ) const;
    // The length of the track in milliseconds
    qint64 trackLength( int i ) const;

    /**
     * Clear collection and read the CD currently in the drive, adding Artist, Album,
     * Genre, Year and whatnot as detected by audiocd using CDDB.
     */
    void readCd();

    void noInfoAvailable();

    void updateProxyTracks();

    QMap<int, QUrl> m_cddbTextFiles;

    QString m_cdName;
    QString m_discCddbId;
    QString m_udi;
    QString m_device;
    mutable int m_encodingFormat;

    QString m_fileNamePattern;
    QString m_albumNamePattern;

    QMap<QUrl, MetaProxy::Track*> m_proxyMap;
};

} //namespace Collections

#endif
