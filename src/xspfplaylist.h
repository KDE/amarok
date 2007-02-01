// Author: Mattias Fliesberg (C) Copyright 2006
// Copyright: See COPYING file that comes with this distribution
//

#ifndef XSPFPlaylist_H
#define XSPFPlaylist_H

class AtomicString;

#include <qstring.h>
#include <q3textstream.h>
#include <qdom.h>
#include <qdatetime.h>
#include <qfile.h>
#include <q3valuelist.h>

#include <kurl.h>

/**
 * @class XSPFPlaylist
 * @author Mattias Fliesberg
 */

typedef struct {
    KURL location;
    QString identifier;
    QString title;
    AtomicString creator;
    QString annotation;
    KURL info;
    KURL image;
    AtomicString album;
    uint trackNum;
    uint duration;
    KURL link;
//     meta,
//     extension
} XSPFtrack;

typedef Q3ValueList < XSPFtrack > XSPFtrackList;

class XSPFPlaylist : public QDomDocument
{
public:
    XSPFPlaylist();
    XSPFPlaylist( Q3TextStream &stream );

public:
    /* convenience functions */
    QString title();
    QString creator();
    QString annotation();
    KURL info();
    KURL location();
    QString identifier();
    KURL image();
    QDateTime date();
    KURL license();
    KURL::List attribution();
    KURL link();
//    meta();
//    extension();

    void setTitle( QString title );
    void setCreator( QString creator );
    void setAnnotation( QString annotation );
    void setInfo( KURL info );
    void setLocation( KURL location );
    void setIdentifier( QString identifier );
    void setImage( KURL image );
    void setDate( QDateTime date );
    void setLicense( KURL license );
    void setAttribution( KURL attribution, bool append = true );
    void setLink( KURL link );
    void setTrackList( XSPFtrackList trackList, bool append = false );
//    meta();
//    extension();

    XSPFtrackList trackList();


private:
    bool loadXSPF( Q3TextStream& );
};

#endif // XSPFPlaylist_H




