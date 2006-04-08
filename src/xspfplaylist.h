// Author: Mattias Fliesberg (C) Copyright 2006
// Copyright: See COPYING file that comes with this distribution
//

#ifndef XSPFPlaylist_H
#define XSPFPlaylist_H

#include <qstring.h>
#include <qtextstream.h>
#include <qdom.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qvaluelist.h>

#include <kurl.h>

/**
 * @class XSPFPlaylist
 * @author Mattias Fliesberg
 */

typedef struct {
    KURL location;
    QString identifier;
    QString title;
    QString creator;
    QString annotation;
    KURL info;
    KURL image;
    QString album;
    uint trackNum;
    uint duration;
    KURL link;
//     meta,
//     extension
} XSPFtrack;

typedef QValueList < XSPFtrack > XSPFtrackList;

class XSPFPlaylist : public QDomDocument
{
public:
    XSPFPlaylist();
    XSPFPlaylist( QTextStream &stream );

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

    void title( QString title );
    void creator( QString creator );
    void annotation( QString annotation );
    void info( KURL info );
    void location( KURL location );
    void identifier( QString identifier );
    void image( KURL image );
    void date( QDateTime date );
    void license( KURL license );
    void attribution( KURL attribution, bool append = true );
    void link( KURL link );
//    meta();
//    extension();

    XSPFtrackList trackList();

    void trackList( XSPFtrackList trackList, bool append = false );

private:
    bool loadXSPF( QTextStream& );
};

#endif // XSPFPlaylist_H




