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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef MP3TUNESSERVICE_H
#define MP3TUNESSERVICE_H



#include "../servicebase.h"
#include "databasedrivencontentmodel.h"

#include "mp3tunesdatafetcher.h"






/**
A service for displaying, previewing and downloading music from Mp3tunes.com

	@author 
*/
class Mp3tunesService : public ServiceBase
{

Q_OBJECT
public:
    Mp3tunesService( const QString &name );

    ~Mp3tunesService();

    void polish();

private:

    Mp3tunesDataFetcher * m_dataFetcher;

};

#endif
