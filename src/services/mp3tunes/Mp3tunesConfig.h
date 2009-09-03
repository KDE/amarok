/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef MP3TUNESCONFIG_H
#define MP3TUNESCONFIG_H

#include <QString>

/**
A wrapper class for Mp3tunes service configuration

	@author
*/
class Mp3tunesConfig{
public:

    Mp3tunesConfig();

    ~Mp3tunesConfig();

    void load();
    void save();

    QString email();
    QString password();
    QString identifier();
    QString partnerToken();
    QString pin();
    QString harmonyEmail();
    bool harmonyEnabled();

    void setEmail( const QString &email );
    void setPassword( const QString &password );
    void setIdentifier( const QString &ident );
    void setHarmonyEnabled( bool enabled );
    void setPartnerToken( const QString &token );
    void setPin( const QString &pin );
    void setHarmonyEmail( const QString &harmonyEmail );

private:

    bool m_hasChanged;
    bool m_harmonyEnabled;
    QString m_email;
    QString m_password;
    QString m_identifier;
    QString m_partnerToken;
    QString m_pin;
    QString m_harmonyEmail;

};

#endif
