/****************************************************************************************
 * Copyright (c) 2004-2010 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2005 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#ifndef AMAROK_SCRIPTMANAGER_H
#define AMAROK_SCRIPTMANAGER_H

#include "scriptengine/AmarokScript.h"
#include "scriptengine/AmarokScriptableServiceScript.h"
#include "EngineObserver.h"   //baseclass
#include "ScriptSelector.h"
#include "ScriptUpdater.h"

#include <KDialog>      //baseclass

#include <KUrl>

#include <QList>
#include <QMap>
#include <QScriptValue>

class KArchiveDirectory;
class KPluginInfo;
class KPluginSelector;
class QScriptEngine;
class ScriptUpdater;

class AMAROK_EXPORT ScriptManager : public KDialog, public EngineObserver
{
    Q_OBJECT

    public:
        static ScriptManager* instance();
        static void destroy();

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

        /** Returns whether or not there is a lyrics script running */
        bool lyricsScriptRunning();

        /** Notifies any running lyric scripts to fetch lyrics */
        void notifyFetchLyrics( const QString& artist, const QString& title );
        /** Notifies any running lyric scripts to fetch desired lyric from given URL */
        void notifyFetchLyricsByUrl( const QString& artist, const QString& title, const QString& url );

        void ServiceScriptPopulate( QString name, int level, int parent_id, QString callbackString, QString filter );

        void ServiceScriptRequestInfo( QString name, int level, QString callbackString );

        void ServiceScriptCustomize( QString name );

        struct ScriptItem {
            KPluginInfo                                     info;
            QScriptEngine*                                  engine;
            KUrl                                            url;
            /** Currently activated in the Script Manager */
            bool                                            running;
            /** Currently being evaluated by the script engine */
            bool                                            evaluating;
            AmarokScript::AmarokScript*                     globalPtr;
            ScriptableServiceScript*                        servicePtr;
            QString                                         log;
            QList<QObject*>                                 guiPtrList;
            QList<QObject*>                                 wrapperList;
            ScriptItem() : engine( 0 ), running( false ), globalPtr( 0 ), servicePtr( 0 ) {}
        };

        typedef QMap<QString, ScriptItem> ScriptMap;
        ScriptMap      m_scripts;
        QString        m_lyricsScript;

    signals:
        // needed so the lyrics script can connect to this
        void fetchLyrics( const QString&, const QString&, const QString& url );

    private slots:
        bool slotInstallScript( const QString& path = QString() );
        void slotRetrieveScript();
        void slotUninstallScript();
        bool slotRunScript( QString name, bool silent = false );
        void slotStopScript( QString name );
        //emmit when start/stop button is clicked
        void slotConfigChanged( bool changed );
        //emmit when configuration button is clicked
        void slotConfigComitted( const QByteArray & name );
        void scriptFinished( QString name );

        /** Finds installed scripts, updates them, and loads them */
        void updateAllScripts();
        void updaterFinished( QString scriptPath );
        void slotUpdateSettingChanged( bool enabled );

       /**
        * MOCKUP: This dialog shows a warning, if a script stalls,
        * and allows the user to terminate this script.
        * Purpose: Getting around upcoming string freeze, then implementing fully.
        */
        void showScriptStalledDialog();

    private:
        explicit ScriptManager( QWidget* parent );
        virtual ~ScriptManager();

        /** Finds all loaded scripts and adds them to the listview */
        void findScripts();

        bool loadScript( const QString& path ); //return false if loadScript failed.

        /** Copies the file permissions from the tarball and loads the script */
        void recurseInstall( const KArchiveDirectory* archiveDir, const QString& destination );

        void startScriptEngine( QString name);

        static QScriptValue ScriptableServiceScript_prototype_ctor( QScriptContext *context, QScriptEngine *engine );
        static QScriptValue ScriptableServiceScript_prototype_populate( QScriptContext *context, QScriptEngine *engine );
        /////////////////////////////////////////////////////////////////////////////////////
        // DATA MEMBERS
        /////////////////////////////////////////////////////////////////////////////////////
        static ScriptManager*  s_instance;
        ScriptSelector*        m_scriptSelector;
        bool                   m_installSuccess;

        QScriptValue   m_global;
        bool           m_configChanged;
        QStringList    m_changedScripts;

        // count returning ScriptUpdaters in a thread-safe way
        QSemaphore     m_updateSemaphore;
        // this is actually an array. The ScriptUpdaters must live longer than the
        // method that creates them, so they're class members
        ScriptUpdater* m_updaters;
        // memorize how many scripts were found and tried to be updated
        int            m_nScripts;

};

#endif /* AMAROK_SCRIPTMANAGER_H */


