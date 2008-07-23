/***************************************************************************
 *   Copyright (C) 2004-2007 by Mark Kretschmann <markey@web.de>           *
 *                      2005 by Seb Ruiz <ruiz@kde.org>                    *
 *                      2008 by Peter ZHOU <peterzhoulei@gmail.com>        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef AMAROK_SCRIPTMANAGER_H
#define AMAROK_SCRIPTMANAGER_H

#include "EngineObserver.h"   //baseclass

#include <KDialog>      //baseclass

#include <KUrl>

#include <QList>
#include <QMap>
#include <QScriptValue>

class KArchiveDirectory;
class KPluginInfo;
class KPluginSelector;
class QScriptEngine;
namespace Amarok {
    class AmarokScript;
    class AmarokScriptableServiceScript;
};

class AMAROK_EXPORT ScriptManager : public KDialog, public EngineObserver
{
    Q_OBJECT

    public:
        static ScriptManager* instance();
        virtual ~ScriptManager();

        /**
         * Runs the script with the given name.
         * @param name The name of the script.
         * @return True if successful.
         */
        bool runScript( const QString& name, bool silent = false );

        /**
         * Stops the script with the given name.
         * @param name The name of the script.
         * @return True if successful.
         */
        bool stopScript( const QString& name );

        /** Returns a list of all currently running scripts. Used by the DCOP handler. */
        QStringList listRunningScripts();

        /** Returns the path of the spec file of the given script */
        QString specForScript( const QString& name );

        void ServiceScriptPopulate( QString name, int level, int parent_id, QString path, QString filter );

    private slots:
        /** Finds all installed scripts and adds them to the listview */
        void findScripts();

        bool slotInstallScript( const QString& path = QString() );
        void slotRetrieveScript();
        void slotUninstallScript();
        bool slotRunScript( bool silent = false );
        void slotStopScript();
        void slotConfigureScript();

        void scriptFinished( QString name );

    private:
        explicit ScriptManager( QWidget* parent );

        bool loadScript( const QString& path ); //return false if loadScript failed.

        /** Copies the file permissions from the tarball and loads the script */
        void recurseInstall( const KArchiveDirectory* archiveDir, const QString& destination );

        void startScriptEngine( QString name);

        /////////////////////////////////////////////////////////////////////////////////////
        // DATA MEMBERS
        /////////////////////////////////////////////////////////////////////////////////////
        static ScriptManager*  s_instance;
        KPluginSelector*       m_scriptSelector;
        bool                   m_installSuccess;

        struct ScriptItem {
            KPluginInfo*                                    info;
            QScriptEngine*                                  engine;
            KUrl                                            url;
            bool                                            running;
            Amarok::AmarokScript*                           globalPtr;
            Amarok::AmarokScriptableServiceScript*          servicePtr;
            QString                                         log;
            QList<QObject*>                                 guiPtrList;
            QList<QObject*>                                 wrapperList;
            ScriptItem() :                                  running( false ){}
        };

        typedef QMap<QString, ScriptItem> ScriptMap;

        ScriptMap      m_scripts;
        QScriptValue   m_global;

};

#endif /* AMAROK_SCRIPTMANAGER_H */


