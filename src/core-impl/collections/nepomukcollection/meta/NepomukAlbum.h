/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
 * Copyright (c) 2013 Edward Toroshchin <amarok@hades.name>                             *
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

#ifndef NEPOMUKALBUM_H
#define NEPOMUKALBUM_H

#include "core/meta/Meta.h"

#include <QUrl>

namespace Meta
{
/**
 * Represents a unit album resource in Amarok
 */
class NepomukAlbum : public Album
{
public:
    explicit NepomukAlbum( const QUrl &resourceUri );

    /* Nepomuk does not support album artists and compilations */
    /* TODO: fix this once it actually does */
    virtual bool isCompilation() const { return false; }
    virtual bool hasAlbumArtist() const { return false; }
    virtual ArtistPtr albumArtist() const { return ArtistPtr(); }

    virtual TrackList tracks();
    virtual QString name() const;

    bool isFilled() const { return !m_name.isEmpty(); }

    void fill( const QString &name ) { m_name = name; }

    QUrl resourceUri() const { return m_resource; }

private:
    QUrl m_resource;
    QString m_name;
};

}
#endif /*NEPOMUKALBUM_H*/
