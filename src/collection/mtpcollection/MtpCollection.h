/* 
   Copyright (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef MTPCOLLECTION_H
#define MTPCOLLECTION_H

#include <libmtp.h>

#include "MtpHandler.h"

#include "Debug.h"
#include "Collection.h"
#include "MemoryCollection.h"

#include <QtGlobal>
#include <QMap>

class MtpCollection;

class MtpCollectionFactory : public Amarok::CollectionFactory
{
    Q_OBJECT
    public:
        MtpCollectionFactory();
        virtual ~MtpCollectionFactory();

        virtual void init();

    public slots:

        void slotCollectionSucceeded( MtpCollection *coll );
        void slotCollectionFailed( MtpCollection *coll );

    private slots:

    void mtpDetected( const QString &serial, const QString &udi );
    void deviceRemoved( const QString &udi );
    void slotCollectionReady();
    void slotCollectionDisconnected( const QString & udi );

    private:

    QMap<QString, MtpCollection*> m_collectionMap;

};

class MtpCollection : public Amarok::Collection, public MemoryCollection
{
    Q_OBJECT
	public:

    MtpCollection( const QString &serial, const QString &udi );
    virtual ~MtpCollection();

    void init(); // called by factory

    void copyTrackToDevice( const Meta::TrackPtr &track );
 //   bool deleteTrackFromDevice( const Meta::MtpTrackPtr &track );
    void removeTrack( const Meta::MtpTrackPtr &track );

    QString getTempFileName( const Meta::MtpTrackPtr track, const QString &tempDir );
    int getTrackToFile( const Meta::MtpTrackPtr track, const QString & filename );

    void setTrackToDelete( const Meta::MtpTrackPtr &track ); // hackish function used since a list of tracks to be deleted can't be passed by a custom capability

    void copyTracksCompleted();

    void deviceRemoved();

    virtual void startFullScan();
    virtual QueryMaker* queryMaker();

    QString udi() const;
    QString serial() const { return m_serial; }

    virtual CollectionLocation* location() const;

    virtual QString collectionId() const;
    virtual QString prettyName() const;

    virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
    virtual Meta::Capability* asCapabilityInterface( Meta::Capability::Type type );

    virtual void collectionUpdated() { DEBUG_BLOCK emit updated(); }

    Mtp::MtpHandler* handler() { return m_handler; }

    void updateTags( Meta::MtpTrack *track);
    void writeDatabase();

 signals:
    void collectionSucceeded( MtpCollection *coll );
    void collectionFailed( MtpCollection *coll );
    void collectionReady();
    void collectionDisconnected( const QString &udi );

    public slots:
//    void deleteTrackToDelete();
//    void deleteTrackSlot( Meta::MtpTrackPtr track);
    void deleteTracksSlot( Meta::TrackList tracklist );
    void slotDeleteTracksCompleted();

    void slotDisconnect();

    private slots:
        void handlerSucceeded();
        void handlerFailed();

 private:

    Meta::MtpTrackPtr m_trackToDelete;
    QString           m_serial;
    QString           m_udi;
    Mtp::MtpHandler *m_handler;

};

#endif
