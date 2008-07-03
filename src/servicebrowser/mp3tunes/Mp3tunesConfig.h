/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
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
    QString hardwareAddress();
    bool harmonyEnabled();

    void setEmail( const QString &email );
    void setPassword( const QString &password );
    void setHardwareAddress( const QString &address );
    void setHarmonyEnabled( bool enabled );

private:

    bool m_hasChanged;
    bool m_harmonyEnabled;
    QString m_email;
    QString m_password;
    QString m_hardwareAddress;

};

#endif
