/****************************************************************************************
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef AMAROK_LASTFM_SHARED_EXPORT_H
#define AMAROK_LASTFM_SHARED_EXPORT_H

/* needed for Q_DECL_EXPORT and Q_DECL_IMPORT macros */
#include <QtGlobal>

#ifndef AMAROK_LASTFM_SHARED_EXPORT
# ifdef MAKE_AMAROK_SERVICE_LASTFM_SHARED_LIB
   /* We are building this library */
#  define AMAROK_LASTFM_SHARED_EXPORT Q_DECL_EXPORT

# else
   /* We are using this library */
#  define AMAROK_LASTFM_SHARED_EXPORT Q_DECL_IMPORT

# endif // MAKE_AMAROK_SERVICE_LASTFM_SHARED_LIB
#endif // AMAROK_LASTFM_SHARED_EXPORT

#endif //AMAROK_LASTFM_SHARED_EXPORT_H
