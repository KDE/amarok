/*
  Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef MAGNATUNEXMLPARSER_H
#define MAGNATUNEXMLPARSER_H


#include "magnatunetypes.h"
#include "threadweaver.h"

#include <qdom.h>
#include <qstring.h>
#include <qxml.h>


/**
@author Nikolaj Hald Nielsen

Parser for the XML file from http://magnatune.com/info/album_info.xml
*/
class MagnatuneXmlParser : public ThreadWeaver::Job
{
    Q_OBJECT

public:
    MagnatuneXmlParser( QString fileName );

    bool doJob();
    void completeJob();

    ~MagnatuneXmlParser();

    void readConfigFile( QString filename );

signals:

    void doneParsing();

private:

    QString m_currentArtist;
    QString m_currentArtistGenre;

    void parseElement( QDomElement e );
    void parseChildren( QDomElement e );
    void parseAlbum( QDomElement e );
    void parseTrack( QDomElement e );

    //MagnatuneListViewArtistItem * m_currentArtistItem;
    //MagnatuneListViewAlbumItem * m_currentAlbumItem;

    MagnatuneAlbum      *m_pCurrentAlbum;
    MagnatuneArtist     *m_pCurrentArtist;
    MagnatuneTrackList   m_currentAlbumTracksList;

    QString m_sFileName;

    int m_nNumberOfTracks;
    int m_nNumberOfAlbums;
    int m_nNumberOfArtists;

    //KListView * m_targetList;

};

#endif
