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

#include "shared/amarok_export.h"

#include <KPluginInfo>
#include <KUrl>

#include <QScriptValue>
#include <QSemaphore>

class ScriptItem;
class ScriptableServiceScript;
namespace AmarokScript {
    class AmarokScript;
}

class QScriptContext;
class QScriptEngine;

class AMAROK_EXPORT ScriptManager : public QObject
{
    Q_OBJECT

    public:
        static ScriptManager* instance();
        static void destroy();

        /**
         * Runs the script with the given name.
         * @param name The pluginName of the script.
         * @return True if successful.
         */
        bool runScript( const QString& name, bool silent = false );

        /**
         * Stops the script with the given name.
         * @param name The name of the script.
         * @return True if successful.
         */
        bool stopScript( const QString& name );

        bool uninstallScript( const QString &name );

        void configChanged( bool changed );

        KPluginInfo::List scripts( const QString &category );

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

        void ServiceScriptPopulate( const QString &name,
                                    int level,
                                    int parent_id,
                                    const QString &callbackString,
                                    const QString &filter );

        void ServiceScriptRequestInfo( const QString &name, int level, const QString &callbackString );

        void ServiceScriptCustomize( const QString &name );

        typedef QHash<QString, ScriptItem*> ScriptMap;
        ScriptMap      m_scripts;
        QString        m_lyricsScript;

    signals:
        // needed so the lyrics script can connect to this
        void fetchLyrics( const QString&, const QString&, const QString& url );
        void lyricsScriptStarted();

    private slots:
        bool slotRunScript( const QString &name, bool silent = false );
        void slotStopScript( const QString &name );
        void scriptFinished( const QString &name );

        /** Finds installed scripts, updates them, and loads them */
        void updateAllScripts();
        void updaterFinished( const QString &scriptPath );

    private:
        explicit ScriptManager( QObject* parent );
        virtual ~ScriptManager();

        bool loadScript( const QString& path ); //return false if loadScript failed.

        void startScriptEngine( const QString &name);

        static QScriptValue ScriptableServiceScript_prototype_ctor( QScriptContext *context, QScriptEngine *engine );
        static QScriptValue ScriptableServiceScript_prototype_populate( QScriptContext *context, QScriptEngine *engine );
        /////////////////////////////////////////////////////////////////////////////////////
        // DATA MEMBERS
        /////////////////////////////////////////////////////////////////////////////////////
        static ScriptManager*  s_instance;

        QScriptValue   m_global;
        bool           m_configChanged;
        QStringList    m_changedScripts;

        // count returning ScriptUpdaters in a thread-safe way
        QSemaphore     m_updateSemaphore;
        // memorize how many scripts were found and tried to be updated
        int            m_nScripts;

};

class ScriptItem
{
public:
    ScriptItem();
    ~ScriptItem();

    KPluginInfo                                     info;
    QScriptEngine*                                  engine;
    KUrl                                            url;
    /** Currently activated in the Script Manager */
    bool                                            running;
    /** Currently being evaluated by the script engine */
    bool                                            evaluating;
    AmarokScript::AmarokScript*                     globalPtr;
    ScriptableServiceScript*                        servicePtr;
    QStringList                                     log;
    QList<QObject*>                                 guiPtrList;
    QList<QObject*>                                 wrapperList;
};

#endif /* AMAROK_SCRIPTMANAGER_H */
