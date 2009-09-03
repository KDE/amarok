/****************************************************************************************
 * Copyright (c) 2008 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_STATE_H
#define AMAROK_STATE_H

#include <QString>

class CollectionManager;
class EngineController;
class MountPointManager;
class ServiceBrowser;
namespace Playlist {
    class Model;
}

namespace Amarok {
    
    class ApplicationContext
    {
    public:
        CollectionManager* collectionManager() const { return m_collectionManager; }
        EngineController* engineController() const { return m_engineController; }
        MountPointManager* mountPointManager() const { return m_mountPointManager; }
        Playlist::Model* playlistModel() const { return m_playlistModel; }
        ServiceBrowser* serviceBrowser() const { return m_serviceBrowser; }
        
        void setCollectionManager( CollectionManager *mgr ) { m_collectionManager = mgr; }
        void setEngineController( EngineController *ec ) { m_engineController = ec; }
        void setMountPointManager( MountPointManager *mpm ) { m_mountPointManager = mpm; }
        void setPlaylistModel( Playlist::Model *pm ) { m_playlistModel = pm; }
        void setServiceBrowser( ServiceBrowser *sb ) { m_serviceBrowser = sb; }
        
    private:
        CollectionManager* m_collectionManager;
        EngineController* m_engineController;
        MountPointManager* m_mountPointManager;
        Playlist::Model* m_playlistModel;
        ServiceBrowser* m_serviceBrowser;
        
    };
    
    class State : public QObject {
    Q_OBJECT
    public:
        
        QString name() const;
        /**
         * implement behaviour when a state was activated (i.e. the state will become the new application state).
         * Please note that states can only make a limited assumptions about the previous state of Amarok. For example,
         * a LowPower state would have to stop all timers, regardless of whether a timer was actually running.
         * It usually can make assumptions about the availability of subsystems, because these will be set up during the
         * startup phase. If, for some reason, a state tries to access a subsystem that was not set up during the startup phase,
         * Amarok *should* crash because that's a major bug!
         */
        virtual void activated() = 0;
        
        virtual CollectionManager* collectionManager() const = 0;
        virtual EngineController* engineController() const = 0;
        virtual MountPointManager* mountPointManager() const = 0;
        virtual Playlist::Model* playlistModel() const = 0;
        virtual ServiceBrowser* serviceBrowser() const = 0;
        
        public void setContext( ApplicationContext *context );
        
    protected:
        State( const QString &name );
        ApplicationContext* context() const { return m_context; }
    
    private:
        QString m_stateName;
        ApplicationContext *m_context;
    };
    
}

#endif
