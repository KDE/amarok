/****************************************************************************************
 * Copyright (c) 2005,2006,2009 Martin Aumueller <aumuell@reserv.at>                    *
 * Copyright (c) 2004 Christian Muehlhaeuser <chris@chris.de>                           *
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef IPHONEOSHANDLER_H
#define IPHONEOSHANDLER_H

#include "MediaDeviceMeta.h"
#include "MediaDeviceHandler.h"

#include <KIO/Job>
#include "kjob.h"
#include <ctime> // for kjob.h
#include <KTempDir>
#include <threadweaver/Job.h>

#include <QObject>
#include <QMap>
#include <QMultiMap>
#include <QMutex>

class QString;
class QMutex;

class IphoneOsCollection;

namespace Handler
{
    class IphoneOsReadCapability;
    class IphoneOsPlaylistCapability;
}

struct sqlite3;


typedef QMultiMap<QString, Meta::TrackPtr> TitleMap;

namespace Meta
{
/* The libgpod backend for all Ipod calls */
class IphoneOsHandler : public MediaDeviceHandler
{
    Q_OBJECT

public:
    IphoneOsHandler( IphoneOsCollection *mc, const QString& mountPoint );
    virtual ~IphoneOsHandler();

    virtual void init(); // collection

    virtual QString prettyName() const;

    /// Capability-related methods

    virtual bool hasCapabilityInterface( Handler::Capability::Type type ) const;
    virtual Handler::Capability* createCapabilityInterface( Handler::Capability::Type type );

    friend class Handler::IphoneOsReadCapability;
    friend class Handler::IphoneOsPlaylistCapability;

public:
    bool isWritable() const { return false; }

    /* Set Methods */

    QString mountPoint() const
    {
        return m_mountPoint;
    }

    void setMountPoint( const QString &mp )
    {
        m_mountPoint = mp;
    }

    /* Methods Provided for Collection */
private:

    QString           m_mountPoint; // directory where device is mounted

    QStringList       m_pids; // list of all persistent ids

    int               m_currentPidIndex;
    QString           m_cachedPid;
    MediaDeviceTrackPtr  m_currentMeta;
    QHash<Meta::MediaDeviceTrackPtr, QString> m_trackhash;

    sqlite3          *m_libraryDb; // Library.itpl
    sqlite3          *m_locationsDb; // Locations.itpl
    sqlite3          *m_dynamicDb; // Dynamic.itpl

    QStringList query(sqlite3 *db, const QString &sqlstatement);
    MediaDeviceTrackPtr &metaForPid(const QString &pid);
    MediaDeviceTrackPtr &metaForTrack( const Meta::MediaDeviceTrackPtr &track );

    /* used by ReadCapability */
    void prepareToParseTracks();
    bool isEndOfParseTracksList();
    void prepareToParseNextTrack();
    void nextTrackToParse();
    void setAssociateTrack(Meta::MediaDeviceTrackPtr);
};
}
#endif
