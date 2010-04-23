/****************************************************************************************
 * Copyright (c) 2007 David Faure <faure@kde.org>                                       *
 * Copyright (c) 2010 Patrick von Reth <patrick.vonreth@gmail.com>                      *
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

#ifndef AMAROK_EXPORT_H
#define AMAROK_EXPORT_H

/* needed for KDE_EXPORT and KDE_IMPORT macros */
#include <kdemacros.h>

#ifndef AMAROK_EXPORT
# ifdef MAKE_AMAROKLIB_LIB 
   /* We are building this library */ 
#  define AMAROK_EXPORT KDE_EXPORT

#  if defined(DEBUG)
#    define AMAROK_EXPORT_TESTS KDE_EXPORT
#  else
#    define AMAROK_EXPORT_TESTS
#  endif

# else
   /* We are using this library */ 
#  define AMAROK_EXPORT KDE_IMPORT

#  if defined(DEBUG)
#    define AMAROK_EXPORT_TESTS KDE_IMPORT
#  else
#    define AMAROK_EXPORT_TESTS
#  endif

# endif//MAKE_AMAROKLIB_LIB 
#endif// AMAROK_EXPORT

# ifndef AMAROK_EXPORT_DEPRECATED
#  define AMAROK_EXPORT_DEPRECATED KDE_DEPRECATED AMAROK_EXPORT
# endif


#ifndef AMAROK_CORE_EXPORT
# ifdef MAKE_AMAROKCORE_LIB 
   /* We are building this library */ 
#  define AMAROK_CORE_EXPORT KDE_EXPORT

#  if defined(DEBUG)
#    define AMAROK_CORE_EXPORT_TESTS KDE_EXPORT
#  else
#    define AMAROK_CORE_EXPORT_TESTS
#  endif

# else
   /* We are using this library */ 
#  define AMAROK_CORE_EXPORT KDE_IMPORT

#  if defined(DEBUG)
#    define AMAROK_CORE_EXPORT_TESTS KDE_IMPORT
#  else
#    define AMAROK_CORE_EXPORT_TESTS
#  endif

# endif
#endif//AMAROK_CORE_EXPORT

# ifndef AMAROK_CORE_EXPORT_DEPRECATED
#  define AMAROK_CORE_EXPORT_DEPRECATED KDE_DEPRECATED AMAROK_CORE_EXPORT
# endif

#endif

