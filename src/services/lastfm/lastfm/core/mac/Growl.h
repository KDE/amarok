/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef CORE_MAC_GROWL_H
#define CORE_MAC_GROWL_H
#ifdef __APPLE__

#include <QString>


class Growl
{
    QString const m_name;
    QString m_title;
    QString m_description;

public:
    /** name is the notification name */
    Growl( const QString& name );
    
    void setTitle( const QString& s ) { m_title = s; }
    void setDescription( const QString& s ) { m_description = s; }
    
    /** shows the notification */
    void notify();
    
    static bool isGrowlAvailable();
};

#endif
#endif
