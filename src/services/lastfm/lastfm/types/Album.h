/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef LASTFM_ALBUM_H
#define LASTFM_ALBUM_H

#include <lastfm/DllExportMacro.h>
#include <lastfm/types/Artist.h>
#include <lastfm/types/Mbid.h>
#include <QString>
#include <QUrl>
class WsReply;


class LASTFM_TYPES_DLLEXPORT Album
{
    Mbid m_mbid;
    Artist m_artist;
    QString m_title;

public:
	Album()
	{}

    explicit Album( Mbid mbid ) : m_mbid( mbid )
    {}

    Album( Artist artist, QString title ) : m_artist( artist ), m_title( title )
    {}

	bool operator==( const Album& that ) const { return m_title == that.m_title && m_artist == that.m_artist; }
	bool operator!=( const Album& that ) const { return m_title != that.m_title || m_artist != that.m_artist; }
	
    operator QString() const { return m_title; }
    QString title() const { return m_title; }
    Artist artist() const { return m_artist; }
    Mbid mbid() const { return m_mbid; }

	/** artist may have been set, since we allow that in the ctor, but should we handle untitled albums? */
	bool isNull() const { return m_title.isEmpty() && m_mbid.isNull(); }
	
    /** Album.getInfo WebService */
    WsReply* getInfo() const;
    WsReply* share( const class User& recipient, const QString& message = "" );

    /** use Tag::list to get the tag list out of the finished reply */
    WsReply* getTags() const;
    WsReply* addTags( const QStringList& ) const;
    
    /** the Last.fm website url for this album */
	QUrl www() const;
	
	enum ImageSize
	{
		Small = 0,
		Medium = 1,
		Large = 2, /** seemingly 174x174 */
        ExtraLarge = 3
	};
};


/** fetches the album art for an album, via album.getInfo */
class LASTFM_TYPES_DLLEXPORT AlbumImageFetcher : public QObject
{
	Q_OBJECT

	int m_size;
	class WsAccessManager* m_manager;
    bool m_nocover;
    
public:
	AlbumImageFetcher( const Album&, Album::ImageSize = Album::Small );
	
    /** if Last.fm doesn't know the album, or has no cover, this @returns false */
    bool isValid() const;
    
signals:
	/** you can init a QPixmap or QImage with this 
      * if the image download fails, you get our default cover image 
      * so this is guarenteed to provide an image */
	void finished( const QByteArray& );
	
private slots:
	void onGetInfoFinished( WsReply* );
	void onImageDataDownloaded();
    void fail();
	
private:
	QString size() const
	{
		switch ((Album::ImageSize) m_size)
		{
			default:
			case Album::Small:return"small";
			case Album::Medium:return"medium";
			case Album::Large:return"large";
            case Album::ExtraLarge:return"extralarge";
		}
	}
};

#endif
