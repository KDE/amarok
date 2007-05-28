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

#ifndef INFOPARSERBASE_H
#define INFOPARSERBASE_H

#include "meta.h"

#include <QObject>

using namespace Meta;


/**
Abstract base class for info parsers

	@author  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class InfoParserBase  : public QObject{
Q_OBJECT
public:

     /**
     * Fetches info about artist and emits infoReady( Qstring ) 
     * with a ready to show html page when the info is ready
     * @param artist The artist to get info about
     */
    virtual void getInfo( ArtistPtr artist ) = 0;

    /**
     * Overloaded function
     * Fetches info about album and emits infoReady( Qstring ) 
     * with a ready to show html page when the info is ready
     * @param url The album to get info about
     */
    virtual void getInfo( AlbumPtr album ) = 0;


    /**
     * Overloaded function
     * Fetches info about track and emits infoReady( Qstring ) 
     * with a ready to show html page when the info is ready
     * @param url The track to get info about
     */
    virtual void getInfo( TrackPtr track ) = 0;

signals:

    void info( QString );


};

#endif
