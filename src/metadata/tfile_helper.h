/***************************************************************************
    copyright            : (C) 2007 by Shane King
    email                : kde@dontletsstart.com
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#ifndef TFILE_HELPER_H
#define TFILE_HELPER_H

#include "config-amarok.h"
#include <tfile.h>

// need to make everything compile against old versions of taglib
// where TagLib::FileName wasn't typedef'd to const char *
#ifdef HAVE_TAGLIB_FILENAME
#define TagLibFileName TagLib::FileName
#else
#define TagLibFileName const char *
#endif

#endif // TFILE_HELPER_H
