/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef AMPACHECONFIG_H
#define AMPACHECONFIG_H

#include <QMap>
#include <QStringList>

class AmpacheServerEntry {

public:
    QString name;
    QString url;
    QString username;
    QString password;
    bool addToCollection;
};

typedef QList< AmpacheServerEntry > AmpacheServerList;

/**
A class for accessing the Ampache plugin configuration

	@author 
*/
class AmpacheConfig{
public:
    
    AmpacheConfig();
    void load();
    void save();

    int serverCount();
    AmpacheServerList servers();

    void addServer( const AmpacheServerEntry &server );
    void removeServer( int index);
    void updateServer( int index, const AmpacheServerEntry &server );

private:

    bool m_hasChanged;
    AmpacheServerList m_servers;

    // Disable copy constructor and assignment
    AmpacheConfig( const AmpacheConfig& );
    AmpacheConfig& operator=( const AmpacheConfig& );

};

#endif
