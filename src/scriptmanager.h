/***************************************************************************
 *   Copyright (C) 2004-2005 by Mark Kretschmann <markey@web.de>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef AMAROK_SCRIPTMANAGER_H
#define AMAROK_SCRIPTMANAGER_H

#include "engineobserver.h"   //baseclass
#include "playlistwindow.h"

#include <qmap.h>

#include <kdialogbase.h>      //baseclass
#include <kurl.h>

class MetaBundle;
class ScriptManagerBase;
class QListViewItem;
class KProcess;


/**
 * @class ScriptManager
 * @short Script management widget and backend
 * @author Mark Kretschmann <markey@web.de>
 *
 * Script notifications, sent to stdin:
 *   configure
 *   engineStateChange: {empty|idle|paused|playing}
 *   trackChange
 *
 */
class ScriptManager : public KDialogBase, public EngineObserver
{
        Q_OBJECT

    public:
        ScriptManager( QWidget *parent = 0, const char *name = 0 );
        virtual ~ScriptManager();

        static ScriptManager* instance() { return s_instance ? s_instance : new ScriptManager( PlaylistWindow::self() ); }

    private slots:
        void slotCurrentChanged( QListViewItem* );

        void slotAddScript();
        void slotRemoveScript();
        void slotEditScript();
        void slotRunScript();
        void slotStopScript();
        void slotConfigureScript();
        void slotAboutScript();

        void scriptFinished( KProcess* process );

    private:
        /** Sends a string message to all running scripts */
        void notifyScripts( const QString& message );

        void loadScript( const QString& path );

        /** Saves all script paths to amarokrc */
        void saveScripts();

        /** Restores all scripts from amarokrc */
        void restoreScripts();

        /** Observer reimplementations **/
        void engineStateChanged( Engine::State state );
        void engineNewMetaData( const MetaBundle& /*bundle*/, bool /*trackChanged*/ );

        static ScriptManager* s_instance;
        ScriptManagerBase*    m_base;

        struct ScriptItem {
            KURL           url;
            KProcess*      process;
            QListViewItem* li;
        };

        typedef QMap<QString, ScriptItem> ScriptMap;

        ScriptMap m_scripts;
};


#endif /* AMAROK_SCRIPTMANAGER_H */


