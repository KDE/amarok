/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
 * Copyright (c) 2006 Mattias Fliesberg <mattias.fliesberg@gmail.com>                   *
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

#ifndef XSPFPLAYLIST_H
#define XSPFPLAYLIST_H

#include "core-impl/playlists/types/file/PlaylistFile.h"

#include <QDomDocument>
#include <QTextStream>

namespace Playlists
{
/* convenience struct for internal use */
struct XSPFTrack {
    // initialize primitive types, don't give stochasticity a chance!
    XSPFTrack() : trackNum( 0 ), duration( 0 ) {}

    QUrl location;
    QString identifier;
    QString title;
    QString creator;
    QString annotation;
    QUrl info;
    QUrl image;
    QString album;
    uint trackNum;
    uint duration;
    QUrl link;
};

typedef QList<XSPFTrack> XSPFTrackList;

/**
 * @author Bart Cerneels <bart.cerneels@kde.org>
 */
class AMAROK_EXPORT XSPFPlaylist : public PlaylistFile, public QDomDocument
{
public:
    enum OnLoadAction {
        NoAction, // do nothing on playlist load
        AppendToPlaylist, // apped this playlist to play queue on load
    };

    /**
     * Creates a new XSPFPlaylist
     *
     * @param url The Url of the xspf file to load.
     * @param onLoad Should this playlist automatically append itself to the playlist when loaded (useful when loading a remote url as it
     * allows the caller to do it in a "one shot" way and not have to worry about waiting untill download and parsing is completed.
     */
    explicit XSPFPlaylist( const QUrl &url, PlaylistProvider *provider = 0, OnLoadAction onLoad = NoAction );

    ~XSPFPlaylist();

    virtual QString name() const;

    /* convenience functions */
    QString title() const;
    QString creator() const;
    QString annotation() const;
    QUrl info() const;
    QUrl location() const;
    QString identifier() const;
    QUrl image() const;
    QDateTime date() const;
    QUrl license() const;
    QList<QUrl> attribution() const;
    QUrl link() const;

    /* Extra XSPF setter methods: */
    void setTitle( const QString &title );
    void setCreator( const QString &creator );
    void setAnnotation( const QString &annotation );
    void setInfo( const QUrl &info );
    void setLocation( const QUrl &location );
    void setIdentifier( const QString &identifier );
    void setImage( const QUrl &image );
    void setDate( const QDateTime &date );
    void setLicense( const QUrl &license );
    void setAttribution( const QUrl &attribution, bool append = true );
    void setLink( const QUrl &link );
    void setTrackList( Meta::TrackList trackList, bool append = false );

    /* PlaylistFile methods */
    virtual bool load( QTextStream &stream ) { return loadXSPF( stream ); }
    virtual bool load( QByteArray &content ) { return loadXSPF( content ); }
    /* Overrides filename and title */
    void setName(const QString &name);
    virtual QString extension() const { return "xspf"; }
    virtual QString mimetype() const { return "application/xspf+xml"; }

    virtual bool save( bool relative ) { return PlaylistFile::save( relative ); }

    void setQueue( const QList<int> &queue );
    QList<int> queue();

protected:
    virtual void savePlaylist( QFile &file );

private:
    XSPFTrackList trackList();

    /**
     * Load file after content was set
     */
    void load();

    /**
     * Sets content in terms of xml document
     * @return true is xml-document is correct, false overwise
     */
    bool processContent( QByteArray &content );

    bool loadXSPF( QTextStream &stream );
    bool loadXSPF( QByteArray &content );

    bool m_autoAppendAfterLoad;
};
}

#endif
