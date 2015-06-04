/****************************************************************************************
 * Copyright (c) 2012 Sven Krohlas <sven@asbest-online.de>                              *
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

#ifndef AMAZONINFOPARSER_H
#define AMAZONINFOPARSER_H

#include "../InfoParserBase.h"

#include "AmazonMeta.h"

#include <kio/job.h>
#include <kio/jobclasses.h>

class AmazonInfoParser : public InfoParserBase
{
    Q_OBJECT

public:
    AmazonInfoParser()
       : InfoParserBase() {}

    ~AmazonInfoParser() {}
    
    virtual void getInfo( Meta::ArtistPtr artist );
    virtual void getInfo( Meta::AlbumPtr album );
    virtual void getInfo( Meta::TrackPtr track );

    void showFrontPage();

private Q_SLOTS:
    void albumInfoDownloadComplete( KJob *requestJob );
};

#endif // AMAZONINFOPARSER_H
