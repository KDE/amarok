/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef MAGNATUNECONFIG_H
#define MAGNATUNECONFIG_H

#include <QString>

/**
Wrapper class for configuration options for the MagnatuneStore plugin

	@author 
*/
class MagnatuneConfig{
public:
    MagnatuneConfig();

    ~MagnatuneConfig();

    void load();
    void save();

    bool isMember();
    void setIsMember( bool isMember );

    QString membershipType();
    void setMembershipType( const QString &membershipType );

    QString email();
    void setEmail( const QString &email );

    QString username();
    QString password();

    void setUsername( const QString &username );
    void setPassword( const QString &password );

    void setStreamType( int theValue );
    int streamType() const;

    qulonglong lastUpdateTimestamp();
    void setLastUpdateTimestamp( qulonglong timestamp );
    

private:

    bool m_hasChanged;
    QString m_username;
    QString m_password;
    QString m_membershipType;
    bool m_isMember;
    int m_streamType;
    QString m_email;
    qulonglong m_lastUpdateTimestamp;

};

#endif
