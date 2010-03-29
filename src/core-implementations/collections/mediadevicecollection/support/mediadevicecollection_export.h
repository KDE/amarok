/****************************************************************************************
 * Copyright (c) 2009 Patrick Spendrin <ps_ml@gmx.de>                                   *
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

#ifndef MEDIADEVICECOLLECTION_EXPORT_H
#define MEDIADEVICECOLLECTION_EXPORT_H

/* needed for KDE_EXPORT and KDE_IMPORT macros */
#include <kdemacros.h>

#ifndef MEDIADEVICECOLLECTION_EXPORT
# if defined(MAKE_MEDIADEVICELIB_LIB) || defined(MAKE_AMAROKLIB_LIB)
   /* We are building this library */ 
#  define MEDIADEVICECOLLECTION_EXPORT KDE_EXPORT
# else
   /* We are using this library */ 
#  define MEDIADEVICECOLLECTION_EXPORT KDE_IMPORT
# endif
#endif

#endif // MEDIADEVICECOLLECTION_EXPORT_H
