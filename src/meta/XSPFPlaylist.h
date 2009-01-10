/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>
   Copyright (C) 2006 Mattias Fliesberg <mattias.fliesberg@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef XSPFPLAYLIST_H
#define XSPFPLAYLIST_H

#include "Playlist.h"
#include "capabilities/EditablePlaylistCapability.h"

#include <QDomDocument>
#include <QTextStream>

class QTextStream;
class KUrl;

namespace Meta
{
class XSPFPlaylist;

typedef KSharedPtr<XSPFPlaylist> XSPFPlaylistPtr;
typedef QList<XSPFPlaylistPtr> XSPFPlaylistList;

/* convenience struct for internal use */
typedef struct {
    KUrl location;
    QString identifier;
    QString title;
    QString creator;
    QString annotation;
    KUrl info;
    KUrl image;
    QString album;
    uint trackNum;
    uint duration;
    KUrl link;
} XSPFTrack;

typedef QList<XSPFTrack> XSPFTrackList;

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class AMAROK_EXPORT XSPFPlaylist : public Playlist, public QDomDocument, public EditablePlaylistCapability
{
public:
    XSPFPlaylist();
    XSPFPlaylist( const KUrl &url );
    XSPFPlaylist( Meta::TrackList list );

    ~XSPFPlaylist();

    virtual QString name() const { return title(); }
    virtual QString prettyName() const { return name(); }

    bool load( QTextStream &stream ) { return loadXSPF( stream ); };
    bool save( const QString &location, bool relative );

    /** returns all tracks in this playlist */
    TrackList tracks();

    /* convenience functions */
    QString title() const;
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

    /* EditablePlaylistCapability virtual functions */
    void setTitle( const QString &title );
    void setCreator( const QString &creator );
    void setAnnotation( const QString &annotation );
    void setInfo( const KUrl &info );
    void setLocation( const KUrl &location );
    void setIdentifier( const QString &identifier );
    void setImage( const KUrl &image );
    void setDate( const QDateTime &date );
    void setLicense( const KUrl &license );
    void setAttribution( const KUrl &attribution, bool append = true );
    void setLink( const KUrl &link );
    void setTrackList( TrackList trackList, bool append = false );

    //TODO: implement these
    void beginMetaDataUpdate() {};
    void endMetaDataUpdate() {};
    void abortMetaDataUpdate() {};

    bool isEditable() const { return true; };

    /* Meta::Playlist virtual functions */
    bool hasCapabilityInterface( Capability::Type type ) const;

    KUrl retrievableUrl() { return m_url; };

    Capability* asCapabilityInterface( Capability::Type type );

private:
    XSPFTrackList trackList();
    bool loadXSPF( QTextStream& );

    KUrl m_url;
};

}

Q_DECLARE_METATYPE( Meta::XSPFPlaylistPtr )
Q_DECLARE_METATYPE( Meta::XSPFPlaylistList )

#endif
