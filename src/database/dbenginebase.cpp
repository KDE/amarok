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

#include <dbenginebase.h>


DbConnection::DbConnection( DbConfig* config )
    : m_config( config )
{}


DbConnection::~DbConnection()
{}

