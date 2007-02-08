// Author: Mattias Fliesberg (C) Copyright 2006
// Copyright: See COPYING file that comes with this distribution
//

#ifndef XSPFPlaylist_H
#define XSPFPlaylist_H

class AtomicString;

#include <QString>
#include <q3textstream.h>
#include <qdom.h>
#include <QDateTime>
#include <QFile>
#include <q3valuelist.h>

#include <kurl.h>

/**
 * @class XSPFPlaylist
 * @author Mattias Fliesberg
 */

typedef struct {
    KUrl location;
    QString identifier;
    QString title;
    AtomicString creator;
    QString annotation;
    KUrl info;
    KUrl image;
    AtomicString album;
    uint trackNum;
    uint duration;
    KUrl link;
//     meta,
//     extension
} XSPFtrack;

typedef Q3ValueList < XSPFtrack > XSPFtrackList;

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
    KUrl info();
    KUrl location();
    QString identifier();
    KUrl image();
    QDateTime date();
    KUrl license();
    KUrl::List attribution();
    KUrl link();
//    meta();
//    extension();

    void setTitle( QString title );
    void setCreator( QString creator );
    void setAnnotation( QString annotation );
    void setInfo( KUrl info );
    void setLocation( KUrl location );
    void setIdentifier( QString identifier );
    void setImage( KUrl image );
    void setDate( QDateTime date );
    void setLicense( KUrl license );
    void setAttribution( KUrl attribution, bool append = true );
    void setLink( KUrl link );
    void setTrackList( XSPFtrackList trackList, bool append = false );
//    meta();
//    extension();

    XSPFtrackList trackList();


private:
    bool loadXSPF( QTextStream& );
};

#endif // XSPFPlaylist_H




