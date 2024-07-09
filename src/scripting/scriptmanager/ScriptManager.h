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
#include "core/meta/forward_declarations.h"
#include "ScriptItem.h"

#include <KPluginMetaData>
#include <QUrl>

#include <QSemaphore>

namespace AmarokScript {
    class AmarokScript;
}
class QJSValue;

class AMAROK_EXPORT ScriptManager : public QObject
{
    Q_OBJECT

    public:
        static ScriptManager* instance();
        static void destroy();
        /** Reads plugin info from legacy .desktop format */
        static KPluginMetaData createMetadataFromSpec( const QString &specPath );

        /**
         * Runs the script with the given name.
         * @param name The pluginName of the script.
         * @param silent Whether to suppress the script output.
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

        QVector<KPluginMetaData> scripts( const QString &category ) const;

        /** Returns a list of all currently running scripts. Used by the DCOP handler. */
        QStringList listRunningScripts() const;

        /** Returns the path of the spec file of the given script */
        QString specForScript( const QString& name ) const;

        /** Returns whether or not there is a lyrics script running */
        bool lyricsScriptRunning() const;

        QString scriptNameForEngine( const QJSEngine *engine ) const;

        /** Notifies any running lyric scripts to fetch desired lyric from given URL */
        void notifyFetchLyrics( const QString& artist, const QString& title, const QString& url, const Meta::TrackPtr &track );

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

    public Q_SLOTS:
        /** Finds installed scripts, updates them, and loads them */
        void updateAllScripts();

    Q_SIGNALS:
        // needed so the lyrics script can connect to this
        void fetchLyrics( const QString&, const QString&, const QString& url, Meta::TrackPtr );
        void lyricsScriptStarted();

        /**
         * Emitted when a script is added, removed or updated
         */
        void scriptsChanged();

    private Q_SLOTS:
        bool slotRunScript( const QString &name, bool silent = false );
        void handleException( const QJSValue &value );

        void updaterFinished( const QString &scriptPath );
        void slotConfigChanged();

    private:
        explicit ScriptManager( QObject* parent );
        ~ScriptManager() override;

        /// \return false if loadScript failed.
        bool loadScript( const QString& path );

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

#endif /* AMAROK_SCRIPTMANAGER_H */
