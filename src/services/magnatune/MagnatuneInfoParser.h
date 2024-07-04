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

#ifndef MAGNATUNEINFOPARSER_H
#define MAGNATUNEINFOPARSER_H

#include "../InfoParserBase.h"

#include "MagnatuneMeta.h"

#include <kio/job.h>


/**
Handles the fetching and processing of Jamendo specific information for meta items

	@author
*/
class MagnatuneInfoParser : public InfoParserBase
{
Q_OBJECT

public:
    MagnatuneInfoParser()
       : InfoParserBase() {}

    ~MagnatuneInfoParser() override {}


    
    void getInfo( const Meta::ArtistPtr &artist ) override;
    void getInfo( const Meta::AlbumPtr &album ) override;
    void getInfo( const Meta::TrackPtr &track ) override;

    void getFrontPage();
    void getFavoritesPage();
    void getRecommendationsPage();

private Q_SLOTS:

    void artistInfoDownloadComplete( KJob *downLoadJob );
    void frontpageDownloadComplete( KJob *downLoadJob );
    void userPageDownloadComplete( KJob *downLoadJob );

private:

    QByteArray extractArtistInfo( const QByteArray &artistPage );
    QByteArray generateMemberMenu();
    QByteArray generateHomeLink();
    QByteArray createArtistLinks( const QByteArray &page );
    
    KJob * m_infoDownloadJob;
    KJob * m_pageDownloadJob;

    QString m_cachedFrontpage;


};

#endif

