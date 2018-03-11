/****************************************************************************************
 * Copyright (c) 2007 David Faure <faure@kde.org>                                       *
 * Copyright (c) 2010 Patrick von Reth <patrick.vonreth@gmail.com>                      *
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef AMAROKCORE_EXPORT_H
#define AMAROKCORE_EXPORT_H

/* needed for Q_DECL_EXPORT and Q_DECL_IMPORT macros */
#include <QtGlobal>

#ifndef AMAROK_CORE_EXPORT
# ifdef MAKE_AMAROKCORE_LIB
   /* We are building this library */
#  define AMAROK_CORE_EXPORT Q_DECL_EXPORT
# else
   /* We are using this library */
#  define AMAROK_CORE_EXPORT Q_DECL_IMPORT
# endif // MAKE_AMAROKCORE_LIB
#endif // AMAROK_CORE_EXPORT

#endif // AMAROKCORE_EXPORT_H
