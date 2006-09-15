// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution

#ifndef MAGNATUNEXMLPARSER_H
#define MAGNATUNEXMLPARSER_H

#include <qstring.h>
#include <qxml.h>
#include <qdom.h>
//#include <klistview.h>
//#include "magnatunelistviewtrackitem.h"
#include "magnatunetypes.h"
#include "threadweaver.h"




/**
@author Nikolaj Hald Nielsen
*/
class MagnatuneXmlParser : public ThreadWeaver::Job {
Q_OBJECT
public:

    MagnatuneXmlParser(QString fileName);

    bool doJob();
    void completeJob();

    ~MagnatuneXmlParser();

    void ReadConfigFile( QString filename );

signals:

    void DoneParsing();

private:
  	
	QString m_currentArtist;
	QString m_currentArtistGenre;

	void ParseElement( QDomElement e );
	void ParseChildren( QDomElement e );	
	void ParseAlbum( QDomElement e );
	void ParseTrack( QDomElement e );

	//MagnatuneListViewArtistItem * m_currentArtistItem;
	//MagnatuneListViewAlbumItem * m_currentAlbumItem;

        MagnatuneAlbum * m_pCurrentAlbum;
        MagnatuneArtist * m_pCurrentArtist;
        MagnatuneTrackList m_currentAlbumTracksList;

	QString m_sFileName;

	int m_nNumberOfTracks;
	int m_nNumberOfAlbums;
	int m_nNumberOfArtists;

	//KListView * m_targetList;

};

#endif
