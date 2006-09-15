// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution

#ifndef MAGNATUNETYPES_H
#define MAGNATUNETYPES_H


#include <qstring.h>
#include <qdatetime.h>
#include <qvaluelist.h>


class MagnatuneArtist {
    
protected:
    int m_id;
    QString m_name;
    QString m_genre;
    QString m_description;
    QString m_photoURL;
    QString m_homeURL;

public:

    MagnatuneArtist();
    MagnatuneArtist(const MagnatuneArtist& artist);
    ~MagnatuneArtist();

    void setId(int id);
    int getId() const;

    void setName(QString name);
    QString getName() const;

    void setDescription(QString description);
    QString getDescription() const;

    void setPhotoURL(QString photoURL);
    QString getPhotoURL() const;
  
    void setHomeURL(QString homeURL);
    QString getHomeURL() const;

};


class MagnatuneAlbum {

protected:
    int m_id;
    QString m_name;
    QString m_coverURL;
    QDate m_launchDate;
    QString m_albumCode;
    QString m_mp3Genre;
    QString m_magnatuneGenres;
    int m_artistId;

public:

    MagnatuneAlbum();
    MagnatuneAlbum(const MagnatuneAlbum& album);
    ~MagnatuneAlbum();

    void setId(int id);
    int getId() const;

    void setName(QString name);
    QString getName() const;

    void setCoverURL(QString coverURL);
    QString getCoverURL() const;

    void setLaunchDate(QDate launchDate);
    QDate getLaunchDate() const;

    void setAlbumCode(QString albumCode);
    QString getAlbumCode() const;

    void setMp3Genre(QString mp3Genre);
    QString getMp3Genre() const;

    void setMagnatuneGenres(QString magnatuneGenres);
    QString getMagnatuneGenres() const;

    void setArtistId(int artistId);
    int getArtistId() const;


};

class MagnatuneTrack {

protected:

    int m_id;
    QString m_name;
    int m_trackNumber;
    int m_duration;
    QString m_hifiURL;
    QString m_lofiURL;
    int m_albumId;
    int m_artistId;

public:

    MagnatuneTrack();
    ~MagnatuneTrack();
    MagnatuneTrack(const MagnatuneTrack& track);

    void setId(int id);
    int getId() const;
 
    void setName(QString name);
    QString getName() const;

    void setTrackNumber(int trackNumber);
    int getTrackNumber() const;

    void setDuration(int duration);
    int getDuration() const;

    void setHifiURL(QString hifiURL);
    QString getHifiURL() const;

    void setLofiURL(QString lofiURL);
    QString getLofiURL() const;

    void setAlbumId(int albumId);
    int getAlbumId() const;

    void setArtistId(int artistId);
    int getArtistId() const;

   
};

typedef QValueList<MagnatuneArtist> MagnatuneArtistList;
typedef QValueList<MagnatuneAlbum> MagnatuneAlbumList;
typedef QValueList<MagnatuneTrack> MagnatuneTrackList;

#endif
