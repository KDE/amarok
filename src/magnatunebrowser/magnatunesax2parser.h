// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
#ifndef MAGNATUNESAX2PARSER_H
#define MAGNATUNESAX2PARSER_H


#include "magnatunetypes.h"
#include <klistview.h>
#include "magnatunelistviewtrackitem.h"

#include <qxml.h>

/**
A SAX2 parser for the magnatune xml

@author Nikolaj Hald Nielsen
*/


class QString;

class MagnatuneSAX2Parser : public QXmlDefaultHandler
{
public:

    bool startDocument();
    bool startElement( const QString&, const QString&, const QString&, const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );
    bool characters ( const QString & ch );
    void setTargetListView(KListView * target);

private:
    

    // following 3 are only for my prototype
    // will be removed when integrated
    KListView * m_targetList;


    bool m_inAlbum;
    bool m_inTrack;
    bool m_inArtist;

    QString indent;
    QString m_currentElementName;

//    MagnatuneListViewArtistItem * m_currentArtist;
//    MagnatuneListViewAlbumItem * m_currentAlbum;
//    MagnatuneListViewTrackItem * m_currentTrack;
    
    void albumStart();
    void handleAlbumElement(QString contents);
    void albumEnd();

    void trackStart();
    void handleTrackElement(QString contents);
    void trackEnd();


};


#endif
