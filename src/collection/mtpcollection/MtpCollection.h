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

#include "Debug.h"

#include "Collection.h"
#include "MemoryCollection.h"
#include "MtpHandler.h"

#include <QtGlobal>
#include <QMap>

class MtpCollection;

class MtpCollectionFactory : public CollectionFactory
{
    Q_OBJECT
    public:
        MtpCollectionFactory();
        virtual ~MtpCollectionFactory();

        virtual void init();

    private:

    private slots:

    void mtpDetected( const QString &udi, const QString &serial );
    void deviceRemoved( const QString &udi );
    void slotCollectionReady();
    void slotCollectionDisconnected( const QString & udi );
    
    private:

        
        
    QMap<QString, MtpCollection*> m_collectionMap;
    

};

class MtpCollection : public Collection, public MemoryCollection
{
    Q_OBJECT
	public:

    MtpCollection( const QString &udi, const QString &serial );
    virtual ~MtpCollection();

    void copyTrackToDevice( const Meta::TrackPtr &track );
    bool deleteTrackFromDevice( const Meta::MtpTrackPtr &track );
    void removeTrack( const Meta::MtpTrackPtr &track );

    void setTrackToDelete( const Meta::MtpTrackPtr &track );

    void copyTracksCompleted();

    void deviceRemoved();

    virtual void startFullScan();
    virtual QueryMaker* queryMaker();

    QString udi() const;
    
    virtual CollectionLocation* location() const;

    virtual QString collectionId() const;
    virtual QString prettyName() const;

    virtual void collectionUpdated() { DEBUG_BLOCK emit updated(); }

    Mtp::MtpHandler* handler() { return m_handler; }

    void updateTags( Meta::MtpTrack *track);
    void writeDatabase();
    
 signals:
    void collectionReady();
    void collectionDisconnected( const QString &udi );
    
    public slots:
	void deleteTrackToDelete();
	void deleteTrackSlot( Meta::MtpTrackPtr track);

	void slotDisconnect();
	
//    private slots:
	
 private:

    Meta::MtpTrackPtr m_trackToDelete;
//    QString           m_mountPoint;
    QString           m_udi;
    QString           m_serial;
    Mtp::MtpHandler *m_handler;


    

};

#endif
