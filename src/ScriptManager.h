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

#include "amarok_export.h"
#include "statusbar/PopupWidget.h"

#include <KPluginInfo>
#include <KUrl>

#include <QScriptValue>
#include <QSemaphore>

class ScriptItem;
class ScriptableServiceScript;
class QTimer;
namespace AmarokScript {
    class AmarokScript;
}

class QScriptContext;
class QScriptEngine;
class QTimerEvent;

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

        void configChanged( bool changed );

        KPluginInfo::List scripts( const QString &category ) const;

        /** Returns a list of all currently running scripts. Used by the DCOP handler. */
        QStringList listRunningScripts() const;

        /** Returns the path of the spec file of the given script */
        QString specForScript( const QString& name ) const;

        /** Returns whether or not there is a lyrics script running */
        bool lyricsScriptRunning() const;

        QString scriptNameForEngine( const QScriptEngine *engine ) const;

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

        static bool minimumBindingsAvailable();

        typedef QHash<QString, ScriptItem*> ScriptMap;
        ScriptMap      m_scripts;
        QString        m_lyricsScript;

    signals:
        // needed so the lyrics script can connect to this
        void fetchLyrics( const QString&, const QString&, const QString& url );
        void lyricsScriptStarted();

    private slots:
        bool slotRunScript( const QString &name, bool silent = false );
        void handleException( const QScriptValue &value );

        /** Finds installed scripts, updates them, and loads them */
        void updateAllScripts();
        void updaterFinished( const QString &scriptPath );

    private:
        explicit ScriptManager( QObject* parent );
        virtual ~ScriptManager();

        /// \return false if loadScript failed.
        bool loadScript( const QString& path );

        static QScriptValue ScriptableServiceScript_prototype_ctor( QScriptContext *context, QScriptEngine *engine );
        static QScriptValue ScriptableServiceScript_prototype_populate( QScriptContext *context, QScriptEngine *engine );

        /////////////////////////////////////////////////////////////////////////////////////
        // DATA MEMBERS
        /////////////////////////////////////////////////////////////////////////////////////
        static ScriptManager*  s_instance;

        bool           m_configChanged;
        QStringList    m_changedScripts;

        // count returning ScriptUpdaters in a thread-safe way
        QSemaphore     m_updateSemaphore;
        // memorize how many scripts were found and tried to be updated
        int            m_nScripts;

};

class ScriptTerminatorWidget : public PopupWidget
{
    Q_OBJECT
public:
    ScriptTerminatorWidget( const QString &message );

signals:
    void terminate();
};

class ScriptItem : public QObject
{
    Q_OBJECT
public:
    ScriptItem( QObject *parent, const QString &name, const QString &path, const KPluginInfo &info );

    QScriptEngine* engine() { return m_engine; }
    ScriptableServiceScript* servicePtr() { return m_servicePtr; }
    KUrl url() const{ return m_url; }
    KPluginInfo info() const { return m_info; }
    bool running() const { return m_running; }
    QString specPath() const;

    bool start( bool silent );

public slots:
    void stop();

private slots:
        void timerEvent ( QTimerEvent *event );

signals:
    void signalHandlerException(QScriptValue);

private:
    QString                                         m_name;
    KUrl                                            m_url;
    KPluginInfo                                     m_info;
    QScriptEngine*                                  m_engine;
    /** Currently activated in the Script Manager */
    bool                                            m_running;
    bool                                            m_evaluating;
    ScriptableServiceScript*                        m_servicePtr;
    QStringList                                     m_log;
    int                                             m_runningTime;
    int                                             m_timerId;
    ScriptTerminatorWidget                          *m_popupWidget;

    /**
     * Initialize QScriptEngine and load wrapper classes
     */
    void initializeScriptEngine();
};

#endif /* AMAROK_SCRIPTMANAGER_H */

