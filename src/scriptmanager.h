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

#include "playlistwindow.h"

#include <qmap.h>

#include <kdialogbase.h>    //baseclass
#include <kurl.h>

class ScriptManagerBase;
class QListViewItem;
class KProcess;


/**
 * @class ScriptManager
 * @short Script managment widget and backend
 * @author Mark Kretschmann <markey@web.de>
 */

class ScriptManager : public KDialogBase
{
        Q_OBJECT

    public:
        ScriptManager( QWidget *parent = 0, const char *name = 0 );
        virtual ~ScriptManager();

        static ScriptManager* instance() { return s_instance ? s_instance : new ScriptManager( PlaylistWindow::self() ); }

    public slots:

    private slots:
        void slotAddScript();
        void slotRemoveScript();
        void slotEditScript();
        void slotRunScript();
        void slotStopScript();
        void slotConfigureScript();
        void scriptFinished( KProcess* process );

    private:
        void loadScript( const QString& path );
        void saveScripts();
        void restoreScripts();

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


