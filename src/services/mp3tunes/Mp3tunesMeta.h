/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2007,2008 Casey Link <unnamedrambler@gmail.com>                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MP3TUNESMETA_H
#define MP3TUNESMETA_H


#include "../ServiceMetaBase.h"
#include "../ServiceAlbumCoverDownloader.h"

#include <KStandardDirs>

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QList>


namespace Meta
{

class Mp3TunesTrack  : public ServiceTrack
{

    public:

        Mp3TunesTrack( const QString& title );

        virtual QString sourceName();
        virtual QString sourceDescription();
        virtual QPixmap emblem();
        virtual QString type() const;
        void setType( const QString &type );
    private:
        QString m_filetype;
};


class Mp3TunesAlbum  : public ServiceAlbumWithCover
{
private:
    QString m_coverURL;


public:
    Mp3TunesAlbum( const QString &name );
    Mp3TunesAlbum( const QStringList &resultRow );

    ~Mp3TunesAlbum();

    virtual QString downloadPrefix() const { return "mp3tunes"; }

    virtual void setCoverUrl( const QString &coverURL );
    virtual QString coverUrl() const;

    virtual QList< QAction *> customActions();


};

}

#endif
