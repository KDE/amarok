/****************************************************************************************
 * Copyright (c) 2004 David Hammerton <david@crazney.net>                               *
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

#ifndef _HASHER_H
#define _HASHER_H

#ifdef __cplusplus
   extern "C" {
#endif

void GenerateHash(short version_major,
                  const unsigned char *url, unsigned char hashSelect,
                  unsigned char *outhash,
                  int request_id);

#ifdef __cplusplus
   }
#endif

#endif /* _HASHER_H */

