/****************************************************************************************
 * Copyright (c) 2006-2007 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#ifndef AMAROK_MOUNTPOINTMANAGER_H
#define AMAROK_MOUNTPOINTMANAGER_H

#include "core-impl/collections/db/sql/amarok_sqlcollection_export.h"

#include <KSharedConfig>

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QSharedPointer>
#include <QUrl>

class DeviceHandler;
class DeviceHandlerFactory;
class SqlStorage;
class QUrl;
namespace Solid {
    class Device;
}

typedef QList<int> IdList;
typedef QList<DeviceHandlerFactory*> FactoryList;
typedef QMap<int, DeviceHandler*> HandlerMap;

class DeviceHandlerFactory : public QObject
{
    Q_OBJECT

public:
    explicit DeviceHandlerFactory( QObject *parent ) : QObject( parent ) {}
    ~DeviceHandlerFactory() override {}

    /**
     * checks whether a DeviceHandler subclass can handle a given Medium.
     * @param device the connected solid volume
     * @return true if the DeviceHandler implementation can handle the medium,
     * false otherwise
     */
    virtual bool canHandle( const Solid::Device &device ) const = 0;

    /**
     * tells the MountPointManager whether it makes sense to ask the factory to
     * create a Devicehandler when a new Medium was connected
     * @return true if the factory can create DeviceHandlers from Medium instances
     */
    virtual bool canCreateFromMedium() const = 0;

    /**
     * creates a DeviceHandler which represents the Medium.
     * @param device the Volume for which a DeviceHandler is required
     * @param udi the device UUID
     * @param s SQL storage
     * @return a DeviceHandler or 0 if the factory cannot handle the Medium
     */
    virtual DeviceHandler* createHandler( const Solid::Device &device, const QString &udi, QSharedPointer<SqlStorage> s ) const = 0;

    virtual bool canCreateFromConfig() const = 0;

    virtual DeviceHandler* createHandler( const KSharedConfigPtr &c, QSharedPointer<SqlStorage> s ) const = 0;

    /**
     * returns the type of the DeviceHandler. Should be the same as the value used in
     * ~/.config/amarokrc
     * @return a QString describing the type of the DeviceHandler
     */
    virtual QString type() const = 0;
};

class DeviceHandler
{
public:
    DeviceHandler() {}
    virtual ~DeviceHandler() {}


    virtual bool isAvailable() const = 0;

    /**
     * returns the type of the DeviceHandler. Should be the same as the value used in
     * ~/.kde/share/config/amarokrc
     * @return a QString describing the type of the DeviceHandler
     */
    virtual QString type() const = 0;

    /**
     * returns an absolute path which is guaranteed to be playable by amarok's current engine. (based on an
     * idea by andrewt512: this method would only be called when we actually want to play the file, not when we
     * simply want to show it to the user. It could for example download a file using KIO and return a path to a
     * temporary file. Needs some more thought and is not actually used at the moment.
     * @param absoluteUrl
     * @param relativeUrl
     */
    virtual void getPlayableURL( QUrl &absoluteUrl, const QUrl &relativeUrl ) = 0;

    /**
     * builds an absolute path from a relative path and DeviceHandler specific information. The absolute path
     * is not necessarily playable! (based on an idea by andrewt512: allows better handling of files stored in remote  * collections. this method would return a "pretty" URL which might not be playable by amarok's engines.
     * @param absoluteUrl the not necessarily playbale absolute path
     * @param relativeUrl the device specific relative path
     */
    virtual void getURL( QUrl &absoluteUrl, const QUrl &relativeUrl ) = 0;

    /**
     * retrieves the unique database id of a given Medium. Implementations are responsible
     * for generating a (sufficiently) unique value which identifies the Medium.
     * Additionally, implementations must recognize unknown mediums and store the necessary
     * information to recognize them the next time they are connected in the database.
     * @return unique identifier which can be used as a foreign key to the media table.
     */
    virtual int getDeviceID() = 0;

    virtual const QString &getDevicePath() const = 0;

    /**
     * allows MountPointManager to check if a device handler handles a specific medium.
     * @param udi
     * @return true if the device handler handles the Medium m
     */
    virtual bool deviceMatchesUdi( const QString &udi ) const = 0;
};

/**
 *	@author Maximilian Kossick <maximilian.kossick@googlemail.com>
 */
class AMAROK_SQLCOLLECTION_EXPORT MountPointManager : public QObject
{
    Q_OBJECT

public:
    MountPointManager( QObject *parent, QSharedPointer<SqlStorage> storage );
    ~MountPointManager() override;

    /**
     *
     * @param url
     * @return
     */
    virtual int getIdForUrl( const QUrl &url );

    /**
     *
     * @param id
     * @return
     */
    virtual QString getMountPointForId( const int id ) const;

    /**
     * builds the absolute path from the mount point of the medium and the given relative
     * path.
     * @param deviceId the medium(device)'s unique id
     * @param relativePath relative path on the medium
     * @return the absolute path
     */
    virtual QString getAbsolutePath( const int deviceId, const QString& relativePath ) const;

    /**
     * calculates a file's/directory's relative path on a given device.
     * @param deviceId the unique id which identifies the device the file/directory is supposed to be on
     * @param absolutePath the file's/directory's absolute path
     */
    virtual QString getRelativePath( const int deviceId, const QString& absolutePath ) const;

    /**
     * allows calling code to access the ids of all active devices
     * @return the ids of all devices which are currently mounted or otherwise accessible
     */
    virtual IdList getMountedDeviceIds() const;

    virtual QStringList collectionFolders() const;
    virtual void setCollectionFolders( const QStringList &folders );

Q_SIGNALS:
    void deviceAdded( int id );
    void deviceRemoved( int id );

private:
    void createDeviceFactories();

    /**
     * Old Amarok versions used to have "Use MusicLocation" config option which caused
     * problems (bug 316216). This method converts out of it and needs to be run after
     * MountPointManager has initialized.
     */
    void handleMusicLocation();

    /**
     * checks whether a medium identified by its unique database id is currently mounted.
     * Note: does not handle deviceId = -1! It only checks real devices
     * @param deviceId the mediums unique id
     * @return true if the medium is mounted, false otherwise
     */
    bool isMounted ( const int deviceId ) const;

    QSharedPointer<SqlStorage> m_storage;
    /**
     * maps a device id to a mount point. does only work for mountable filesystems and needs to be
     * changed for the real Dynamic Collection implementation.
     */
    HandlerMap m_handlerMap;
    mutable QMutex m_handlerMapMutex;
    FactoryList m_mediumFactories;
    FactoryList m_remoteFactories;
    bool m_ready;

//Solid specific
    void createHandlerFromDevice( const Solid::Device &device, const QString &udi );
private Q_SLOTS:
    void slotDeviceAdded( const QString &udi );
    void slotDeviceRemoved( const QString &udi );

};

#endif
