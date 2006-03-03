/***************************************************************************
 *   Copyright (C) 2004-2006 by Mark Kretschmann <markey@web.de>           *
 *                      2005 by Seb Ruiz <me@sebruiz.net>                  *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
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
class KArchiveDirectory;
class KProcess;
class KProcIO;


/**
 * @class ScriptManager
 * @short Script management widget and backend
 * @author Mark Kretschmann <markey@web.de>
 *
 * Script notifications, sent to stdin:
 *   configure
 *   engineStateChange: {empty|idle|paused|playing}
 *   trackChange
 *   volumeChange: newVolume (range: 0-100)
 *   fetchLyrics: artist title
 *   fetchLyricsByUrl: url
 *
 * @see http://amarok.kde.org/amarokwiki/index.php/Script-Writing_HowTo
 */

class ScriptManager : public KDialogBase, public EngineObserver
{
    Q_OBJECT

    friend class AmarokScriptNewStuff;

    public:
        ScriptManager( QWidget *parent = 0, const char *name = 0 );
        virtual ~ScriptManager();

        static ScriptManager* instance() { return s_instance ? s_instance : new ScriptManager( PlaylistWindow::self() ); }

        /**
         * Runs the script with the given name. Used by the DCOP handler.
         * @param name The name of the script.
         * @return True if successful.
         */
        bool runScript( const QString& name );

        /**
         * Stops the script with the given name. Used by the DCOP handler.
         * @param name The name of the script.
         * @return True if successful.
         */
        bool stopScript( const QString& name );

        /** Returns a list of all currently running scripts. Used by the DCOP handler. */
        QStringList listRunningScripts();

       /** Custom Menu Click */
       void customMenuClicked( const QString& message );

       /** Returns the path of the spec file of the given script */
       QString specForScript( const QString& name );

       /** Return name of the lyrics script currently running, or QString::null if none */
       QString lyricsScriptRunning() const;

       /** Returns a list of all lyrics scripts */
       QStringList lyricsScripts() const;

       /** Sends a fetchLyrics notification to all scripts */
       void notifyFetchLyrics( const QString& artist, const QString& title );

       /** Sends a fetchLyrics notification to retrieve lyrics from a specific page */
       void notifyFetchLyricsByUrl( const QString& url );

       /** Return name of the transcode script currently running, or QString::null if none */
       QString transcodeScriptRunning() const;

       /** Sends a transcode notification to all scripts */
       void notifyTranscode( const QString& srcUrl, const QString& filetype );

    signals:
        /** Emitted when the lyrics script changes, so that a lyrics retry can be made */
        void lyricsScriptChanged();

    private slots:
        /** Finds all installed scripts and adds them to the listview */
        void findScripts();

        /** Enables/disables the buttons */
        void slotCurrentChanged( QListViewItem* );

        bool slotInstallScript( const QString& path = QString::null );
        void slotRetrieveScript();
        void slotUninstallScript();
        bool slotRunScript();
        void slotStopScript();
        void slotConfigureScript();
        void slotAboutScript();
        void slotShowContextMenu( QListViewItem*, const QPoint& );

        void slotReceivedStdout( KProcess*, char*, int );
        void slotReceivedStderr( KProcess*, char*, int );
        void scriptFinished( KProcess* process );

    private:
        /** Terminates a process with SIGTERM and deletes the KProcIO object */
        void terminateProcess( KProcIO** proc );

        /** Sends a string message to all running scripts */
        void notifyScripts( const QString& message );

        /** Adds a script to the listview */
        void loadScript( const QString& path );

        /** Copies the file permissions from the tarball and loads the script */
        void recurseInstall( const KArchiveDirectory* archiveDir, const QString& destination );

        /** EngineObserver reimplementations **/
        void engineStateChanged( Engine::State state, Engine::State oldState = Engine::Empty );
        void engineNewMetaData( const MetaBundle& /*bundle*/, bool /*trackChanged*/ );
        void engineVolumeChanged( int newVolume );

        /////////////////////////////////////////////////////////////////////////////////////
        // DATA MEMBERS
        /////////////////////////////////////////////////////////////////////////////////////
        static ScriptManager* s_instance;
        ScriptManagerBase*    m_gui;
        bool                  m_installSuccess;

        struct ScriptItem {
            KURL           url;
            QString        type;
            KProcIO*       process;
            QListViewItem* li;
            QString        log;
        };

        typedef QMap<QString, ScriptItem> ScriptMap;

        ScriptMap m_scripts;
};


#endif /* AMAROK_SCRIPTMANAGER_H */


