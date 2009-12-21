/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef MP3TUNESSERVICE_H
#define MP3TUNESSERVICE_H

#include "../ServiceBase.h"
#include "Mp3tunesServiceCollection.h"
#include "Mp3tunesLocker.h"
#include "Mp3tunesWorkers.h"
#include "harmonydaemon/Mp3tunesHarmonyDownload.h"
#include "Mp3tunesHarmonyHandler.h"
#include <QVariantMap>

class Mp3tunesServiceFactory: public ServiceFactory
{
    Q_OBJECT

    public:
        explicit Mp3tunesServiceFactory() {}
        virtual ~Mp3tunesServiceFactory() {}

        virtual bool possiblyContainsTrack( const KUrl &url ) const;

        virtual void init();
        virtual QString name();
        virtual KPluginInfo info();
        virtual KConfigGroup config();
};


/**
    A service for displaying, previewing and downloading music from Mp3tunes.com
	@author
*/
class Mp3tunesService : public ServiceBase
{

Q_OBJECT

public:
    explicit Mp3tunesService( Mp3tunesServiceFactory* parent,
                              const QString &name,
                              const QString &partnerToken,
                              const QString &email = QString(),
                              const QString &password = QString(),
                              bool harmonyEnabled = false );

    ~Mp3tunesService();

    /**
     * Helper function to redraw the service's ui elements
     */
    void polish();

    virtual Amarok::Collection * collection() { return m_collection; }

private slots:
    /**
     * Enables harmony
     */
    void enableHarmony();

    /**
     * Disables harmony
     */
    void disableHarmony();

    /**
     * Logs the user into the locker, prompts them for a user/pass if not supplied
     */
    void authenticate( const QString & uname = "", const QString & passwd = "" );

    /**
     * Handles authentication reply.
     */
    void authenticationComplete(  const QString & sessionId );

    /**
     * the daemon received the PIN. now that pin has to be presented to the user,
     * so he/she (comments must be gender neutral) can add it to his/her mp3tunes
     * account.
     */
    void harmonyWaitingForEmail( const QString &pin );
    void harmonyWaitingForPin();
    void harmonyConnected();
    void harmonyDisconnected();
    void harmonyError( const QString &error );
    void harmonyDownloadReady( const QVariantMap &download );
    void harmonyDownloadPending( const QVariantMap &download );

private:
    /**
     * Helper function that draws the menu bar above the tree view
     */
    void initTopPanel();

    /**
     * Helper function that draws the ui elements below the tree view
     */
    void initBottomPanel();

    QString m_email;
    QString m_password;
    bool m_harmonyEnabled; // if the user has enabled harmony
    QString m_partnerToken;

    bool m_authenticated; // true if mp3tunes has authenticated successfully
    bool m_authenticationFailed;
    QString m_sessionId; // the mp3tunes sid

    Mp3tunesServiceCollection *  m_collection;
    Mp3tunesLoginWorker * m_loginWorker; // used to see if logging in has completed
    Mp3tunesLocker * m_locker; // the actual locker
    Mp3tunesHarmonyHandler * m_harmony;
};

#endif
