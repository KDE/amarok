/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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

#ifndef UPNPBROWSECOLLECTION_H
#define UPNPBROWSECOLLECTION_H

#include "UpnpCollectionBase.h"
#include "MemoryCollection.h"
#include "core/capabilities/CollectionScanCapability.h"

#include <QMap>
#include <QHash>
#include <QHostInfo>
#include <QPointer>
#include <QQueue>
#include <QtGlobal>
#include <QSharedPointer>

#include <QIcon>
#include <KDirNotify>
#include <kio/udsentry.h>

namespace KIO {
  class Job;
}
class KJob;

class QTimer;

namespace Collections {

class UpnpCache;

class UpnpBrowseCollection : public UpnpCollectionBase
{
  Q_OBJECT
  public:
    explicit UpnpBrowseCollection( const DeviceInfo& );
    ~UpnpBrowseCollection() override;

    QueryMaker* queryMaker() override;

    bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
    Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

    Meta::TrackPtr trackForUrl( const QUrl &url ) override;
    QIcon icon() const override { return QIcon::fromTheme("network-server"); }

    QSharedPointer<MemoryCollection> memoryCollection() const { return m_mc; }

  Q_SIGNALS:
    void incrementProgress();
    void totalSteps( int );
    void endProgressOperation( QObject * );

  public Q_SLOTS:
    virtual void startFullScan();
    virtual void startIncrementalScan( const QString &directory = QString() );

  private Q_SLOTS:
    void entries( KIO::Job *, const KIO::UDSEntryList& );
    void done( KJob * );
    void createTrack( const KIO::UDSEntry &, const QString &baseUrl );
    void removeTrack( const Meta::TrackPtr &t );
    void invalidateTracksIn( const QString &dir );
    void updateMemoryCollection();
    void slotFilesChanged(const QStringList &);
    void processUpdates();

  private:
    QSharedPointer<MemoryCollection> m_mc;

    QTimer *m_fullScanTimer;
    bool m_fullScanInProgress;

    // associates each track with its UPNP Parent <container>
    // when a update occurs on a <container>
    // invalidate all tracks, and rescan
    // it remains to be seen how badly this
    // affects performance or memory
    QHash<QString, Meta::TrackList> m_tracksInContainer;

    QQueue<QString> m_updateQueue;

    UpnpCache *m_cache;
};

class UpnpBrowseCollectionScanCapability : public Capabilities::CollectionScanCapability
{
    Q_OBJECT
    public:
        explicit UpnpBrowseCollectionScanCapability( UpnpBrowseCollection* collection );
        ~UpnpBrowseCollectionScanCapability() override;

        void startFullScan() override;
        void startIncrementalScan( const QString &directory = QString() ) override;
        void stopScan() override;

    private:
        UpnpBrowseCollection *m_collection;
};


} //namespace Collections

#endif
