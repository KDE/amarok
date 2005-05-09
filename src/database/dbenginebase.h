/***************************************************************************
 *   Copyright (C)  2004-2005 Mark Kretschmann <markey@web.de>             *
 *                  2004 Christian Muehlhaeuser <chris@chris.de>           *
 *                  2004 Sami Nieminen <sami.nieminen@iki.fi>              *
 *                  2005 Ian Monroe <ian@monroe.nu>                        *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef AMAROK_DBENGINEBASE_H
#define AMAROK_DBENGINEBASE_H

#include "plugin/plugin.h" //baseclass
#include <qobject.h>       //baseclass


class DbConnection  : public QObject, public amaroK::Plugin
{
    public:
        enum DbConnectionType { sqlite = 0, mysql = 1, postgresql = 2 };

        DbConnection( DbConfig* /* config */ );
        virtual ~DbConnection() = 0;

        virtual QStringList query( const QString& /* statement */ ) = 0;
        virtual int insert( const QString& /* statement */, const QString& /* table */ ) = 0;
        const bool isInitialized() const { return m_initialized; }
        virtual bool isConnected() const = 0;
        virtual const QString lastError() const { return "None"; }

    protected:
        bool m_initialized;
        DbConfig *m_config;
};


#endif /*AMAROK_DBENGINEBASE_H*/
