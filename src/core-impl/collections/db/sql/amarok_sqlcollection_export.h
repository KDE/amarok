/****************************************************************************************
 * Copyright (c) 2007 David Faure <faure@kde.org>                                       *
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

#ifndef AMAROK_SQLCOLLECTION_EXPORT_H
#define AMAROK_SQLCOLLECTION_EXPORT_H

/* needed for KDE_EXPORT and KDE_IMPORT macros */
#include <kdemacros.h>

#ifndef AMAROK_SQLCOLLECTION_EXPORT
# if defined(MAKE_AMAROK_SQLCOLLECTION_LIB)
   /* We are building this library */
#   define AMAROK_SQLCOLLECTION_EXPORT KDE_EXPORT

#   if defined(DEBUG)
#       define AMAROK_SQLCOLLECTION_EXPORT_TESTS KDE_EXPORT
#   else
#       define AMAROK_SQLCOLLECTION_EXPORT_TESTS
#   endif


# else
   /* We are using this library */
#   define AMAROK_SQLCOLLECTION_EXPORT KDE_IMPORT

#   if defined(DEBUG)
#       define AMAROK_SQLCOLLECTION_EXPORT_TESTS KDE_IMPORT
#   else
#       define AMAROK_SQLCOLLECTION_EXPORT_TESTS
#   endif

# endif
#endif

# ifndef AMAROK_SQLCOLLECTION_EXPORT_DEPRECATED
#  define AMAROK_SQLCOLLECTION_EXPORT_DEPRECATED KDE_DEPRECATED AMAROK_SQLCOLLECTION_EXPORT
# endif

#endif

