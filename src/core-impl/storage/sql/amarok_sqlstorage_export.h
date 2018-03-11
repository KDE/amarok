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

#ifndef AMAROK_SQLSTORAGE_EXPORT_H
#define AMAROK_SQLSTORAGE_EXPORT_H

/* needed for Q_DECL_EXPORT and Q_DECL_IMPORT macros */
#include <QtGlobal>

#ifndef AMAROK_SQLSTORAGE_EXPORT
# if defined(MAKE_AMAROK_SQLSTORAGE_LIB)
   /* We are building this library */
#   define AMAROK_SQLSTORAGE_EXPORT Q_DECL_EXPORT
# else
   /* We are using this library */
#   define AMAROK_SQLSTORAGE_EXPORT Q_DECL_IMPORT
# endif
#endif

#ifndef AMAROK_SQLSTORAGE_MYSQLE_EXPORT
# if defined(MAKE_AMAROK_STORAGE_MYSQLESTORAGE_LIB)
   /* We are building this library */
#   define AMAROK_SQLSTORAGE_MYSQLE_EXPORT Q_DECL_EXPORT
# else
   /* We are using this library */
#   define AMAROK_SQLSTORAGE_MYSQLE_EXPORT Q_DECL_IMPORT
# endif
#endif

# ifndef AMAROK_SQLSTORAGE_EXPORT_DEPRECATED
#  define AMAROK_SQLSTORAGE_EXPORT_DEPRECATED QT_DEPRECATED AMAROK_SQLSTORAGE_EXPORT
# endif

#endif
