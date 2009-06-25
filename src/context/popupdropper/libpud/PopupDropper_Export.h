/****************************************************************************************
 * Copyright (c) 2008 Jeff Mitchell <mitchell@kde.org>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef POPUPDROPPER_EXPORT_H
#define POPUPDROPPER_EXPORT_H

#include <qglobal.h>

#ifdef Q_WS_WIN
# if defined(MAKE_POPUPDROPPER_LIB)
#  define POPUPDROPPER_EXPORT Q_DECL_EXPORT
# else
#  define POPUPDROPPER_EXPORT Q_DECL_IMPORT
# endif
#else
# define POPUPDROPPER_EXPORT Q_DECL_EXPORT
#endif 

#endif
