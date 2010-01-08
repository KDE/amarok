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

#ifndef INFOPARSERBASE_H
#define INFOPARSERBASE_H

#include "amarok_export.h"
#include "Meta.h"

#include <QObject>

/**
Abstract base class for info parsers

    @author  Nikolaj Hald Nielsen <nhn@kde.org>
*/
class AMAROK_EXPORT InfoParserBase  : public QObject
{
    Q_OBJECT

public:

    InfoParserBase();

     /**
     * Fetches info about artist and emits info( Qstring )
     * with a ready to show html page when the info is ready
     * @param artist The artist to get info about
     */
    virtual void getInfo( Meta::ArtistPtr artist ) = 0;

    /**
     * Overloaded function
     * Fetches info about album and emits info( Qstring )
     * with a ready to show html page when the info is ready
     * @param url The album to get info about
     */
    virtual void getInfo( Meta::AlbumPtr album ) = 0;

    /**
     * Overloaded function
     * Fetches info about track and emits info( Qstring )
     * with a ready to show html page when the info is ready
     * @param url The track to get info about
     */
    virtual void getInfo( Meta::TrackPtr track ) = 0;


    void showLoading( const QString &message );

signals:
    /**
     * Signal emmited when new html info is ready to be shown
     * @param info The string containing the html formatted information
     */
    void info( QString info );

private:

    static QString s_loadingBaseHtml;
};

#endif
