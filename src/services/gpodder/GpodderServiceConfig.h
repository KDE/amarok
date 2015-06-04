/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2010 Christian Wagner <christian.wagner86@gmx.at>                      *
 * Copyright (c) 2010 Felix Winter <ixos01@gmail.com>                                   * 
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

#ifndef GPODDERSERVICECONFIG_H
#define GPODDERSERVICECONFIG_H

#include <QObject>
#include <QString>

namespace KWallet { class Wallet; }

class KDialog;

class GpodderServiceConfig : public QObject
{
    Q_OBJECT

public:
    static const char *configSectionName() { return "Service_gpodder"; }

    GpodderServiceConfig();
    ~GpodderServiceConfig();

    void load();
    void save();
    void reset();

    const QString &username() { return m_username; }
    void setUsername( const QString &username ) { m_username = username; }

    const QString &password() { return m_password; }
    void setPassword( const QString &password ) { m_password = password; }

    bool enableProvider() { return m_enableProvider; }
    void setEnableProvider( bool enableProvider ) { m_enableProvider = enableProvider; }

    bool ignoreWallet() { return m_ignoreWallet; }
    void setIgnoreWallet( bool ignoreWallet ) { m_ignoreWallet = ignoreWallet; }

    bool isDataLoaded() { return m_isDataLoaded; }

private Q_SLOTS:
    void textDialogYes();
    void textDialogNo();

private:
    void askAboutMissingKWallet();
    void tryToOpenWallet();

    QString m_username;
    QString m_password;
    bool m_enableProvider; //Enables PodcastProvider if correct LoginData given
    bool m_ignoreWallet;
    bool m_isDataLoaded;

    KDialog *m_askDiag;
    KWallet::Wallet *m_wallet;
};

#endif // GPODDERSERVICECONFIG_H
