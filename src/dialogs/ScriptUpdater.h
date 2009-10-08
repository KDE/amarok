/****************************************************************************************
 * Copyright (c) 2009 Jakob Kummerow <jakob.kummerow@gmail.com>                         *
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

#ifndef AMAROK_SCRIPTUPDATER_H
#define AMAROK_SCRIPTUPDATER_H

#include <string.h>

#include <KIO/Job>

#include <QThread>
#include <QSemaphore>
#include <QTemporaryFile>

// static configuration
// TODO: remember to change the base URL before release!
static const QString updateBaseUrl     = "http://home.in.tum.de/~kummeroj/update/"; // must end with '/'
static const QString archiveFilename   = "main.tar.bz2";
static const QString versionFilename   = "version";
static const QString signatureFilename = "signature";
// TODO: remember to change the public key before release!
static const QString publicKey = "-----BEGIN PUBLIC KEY-----\n"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAqSVx2dsSkfNniS/bK81q\n"
"JqyWsBiOaTFcvKn3SsQ8hWlPiyYgJUc0BFThbpOLw0et2cxvgCCryudWigCW5iNq\n"
"DeOYU2rC+fWjqMJMV/pSMQKIDtvlZRKpR6pmqcWSlpfLXxTVHPKBk4LKcb62O4Vi\n"
"TUQ6YYDQuMeDmpvdNJLRJtHs3ZAT5nLxLGP5TqLgcBtnte43uNgdJ1FSDROSwQcS\n"
"JpwhhEWsMnHB8wC6kr2oS721DJscMdGkqPvDZqqUqCfybzyFy20kFZ6ws5Ae4LgQ\n"
"c6vqkUUfaeiFx2Cx2htEgU4A1tze58h7Om3q3YXX1Rpl+iEMCMLAKRVHwYkLkUSe\n"
"sQIDAQAB\n"
"-----END PUBLIC KEY-----";


class ScriptUpdater : public QThread
{

    Q_OBJECT

    public:
        explicit ScriptUpdater();
        virtual ~ScriptUpdater();
        void setScriptPath( const QString& scriptPath );

    public slots:
        void updateScript();

    signals:
        void finished( QString scriptPath );

    protected:
        virtual void run();

    private slots:
        void phase2( KJob * job );
        void phase3( KJob * job );
        void phase4( KJob * job );

    private:

        bool isNewer(const QString & update, const QString & installed);
        
        QString m_scriptPath;

        // dynamically collected information about the script
        QString m_scriptname, m_scriptversion, m_fileName;
        QTemporaryFile m_archiveFile, m_sigFile, m_versionFile;

};

#endif // AMAROK_SCRIPTUPDATER_H
