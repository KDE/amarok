/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef PLAYLISTBREADCRUMBLEVEL_H
#define PLAYLISTBREADCRUMBLEVEL_H

#include <KIcon>

#include <QMap>
#include <QPair>
#include <QString>

namespace Playlist
{

/**
 *  A level of a hierarchical structure which can be used in a breadcrumb interface.
 *  @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class BreadcrumbLevel
{
public:
    /**
     * Constructor.
     */
    BreadcrumbLevel( QString internalColumnName );

    /**
     * Destructor.
     */
    ~BreadcrumbLevel();

    const QString & name();

    const QString & prettyName();

    const KIcon& icon();

    const QMap< QString, QPair< KIcon, QString > > siblings();

protected:
    QString m_name;         //! the name of this item.
    QString m_prettyName;
    KIcon m_icon;
    QMap< QString, QPair< KIcon, QString > > m_siblings;    //! internalColumnName, icon, prettyName
};

}   //namespace Playlist

#endif  //PLAYLISTBREADCRUMBLEVEL_H
