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
static const QString updateBaseUrl     = "http://amarok.kde.org/scriptupdates/"; // must end with '/'
static const QString archiveFilename   = "main.tar.bz2";
static const QString versionFilename   = "version";
static const QString signatureFilename = "signature";
static const QString publicKey = "-----BEGIN PUBLIC KEY-----\n"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuiTmOX5inpOpSIHDB5Je\n"
"W2R+YKINMdWW35rL0NKt7tCm1bl3Xdd9k7AdSHSCkJo4xnpXwLeisAhLEpNNCsUZ\n"
"n1GNJ1AouCfSlHOyES9uIc9ecLx3ByjfQ4XKBu0Jf1QmoAhzRgpvdoYtkR/gul8X\n"
"yfA1n6keL3ZQ+5YYqD/vU5rgYKaOloZlUhXVVohfJxCV9jvKRvfVsVlt5DQmYt1k\n"
"GfWjJAaJ6/XS+BlvxV8pgEYvnH4aVtspoD3GMIJLV8q+xK9FeQUNJZxlOoj5CyMc\n"
"BZmCyrPU1o4S4nvCSOFuAkEYtlnsSs4U/LmW3uKkVJET22wG2c/CPR8J9+X/pHZA\n"
"4QIDAQAB\n"
"-----END PUBLIC KEY-----\n";


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
