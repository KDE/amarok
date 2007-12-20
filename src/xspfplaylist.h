// Author: Mattias Fliesberg (C) Copyright 2006
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//

#ifndef XSPFPlaylist_H
#define XSPFPlaylist_H

#include "atomicstring.h"
class QString;
class QDateTime;
class KUrl;

#include <QDomDocument>

#include "Meta.h"

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
} XSPFtrack;

typedef QList<XSPFtrack> XSPFtrackList;

/**
 * @class XSPFPlaylist
 * @author Mattias Fliesberg
 * @author Ian Monroe
 */
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
    void setTrackList( Meta::TrackList trackList, bool append = false );
//    meta();
//    extension();

    XSPFtrackList trackList();


private:
    bool loadXSPF( QTextStream& );
};

#endif // XSPFPlaylist_H




