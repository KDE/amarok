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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef MAGNATUNEINFOPARSER_H
#define MAGNATUNEINFOPARSER_H

#include "../InfoParserBase.h"

#include "MagnatuneMeta.h"

#include <kio/job.h>
#include <kio/jobclasses.h>



/**
Handles the fetching and processing of Jamendo specific information for meta items

	@author
*/
class MagnatuneInfoParser : public InfoParserBase
{
Q_OBJECT

public:
    MagnatuneInfoParser() {}

    ~MagnatuneInfoParser() {}


    
    virtual void getInfo( Meta::ArtistPtr artist );
    virtual void getInfo( Meta::AlbumPtr album );
    virtual void getInfo( Meta::TrackPtr track );

    void getFrontPage();

private:

    KJob * m_infoDownloadJob;
    KJob * m_frontPageDownloadJob;
    
    QString extractArtistInfo( const QString &artistPage );
private slots:

    void artistInfoDownloadComplete( KJob *downLoadJob );
    void frontPageDownloadComplete( KJob *downLoadJob );

signals:

    void info( QString );
};

#endif

