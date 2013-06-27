/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef BROWSERDEFINES_H
#define BROWSERDEFINES_H

#include <QMetaType>

namespace CategoryId
{
    /**
     * Categories for collection browser levels.
     *
     * Beware, the numeric values get written to config files, change them only if
     * you know what you're doing.
     */
    enum CatMenuId {
        None = 0,
        Album = 1,
        Artist = 8, // used to be 2, transitioned to 8 to allow for transition pre Amarok 2.8
        AlbumArtist = 3,
        Composer = 4,
        Genre = 5,
        Year = 6,
        Label = 7
    };
}

Q_DECLARE_METATYPE( CategoryId::CatMenuId )

#endif // BROWSERDEFINES_H
