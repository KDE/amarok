/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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


#ifndef TAGSFROMFILENAMEGUESSER_H
#define TAGSFROMFILENAMEGUESSER_H

#ifndef UTILITIES_BUILD
    #include "amarok_export.h"
#else
    #define AMAROK_EXPORT
#endif

#include "MetaValues.h"

namespace Meta
{
    namespace Tag
    {
        namespace TagGuesser
        {
            /**
            * Try to guess metadata from file name, using common filename
            * templates, can't work with full file path.
            * @arg fileName file name, if fileName contains full path, It will be
            * truckated.
            * @returns guessed metadata.
            */
            AMAROK_EXPORT Meta::FieldHash guessTags( const QString &fileName );

            /**
            * Try to guess metadata from fil name,using specified scheme.
            * @arg fileName file path
            * @arg scheme is a regular exprassion with tokens.
            * Available Tokens: %album%, %albumartist%, %artist%, %title%, %track%.
            */
            AMAROK_EXPORT Meta::FieldHash guessTagsByScheme( const QString &fileName,
                                                             const QString &scheme,
                                                             bool cutTrailingSpaces = true,
                                                             bool convertUnderscores = true );
        }
    }
}

#endif // TAGSFROMFILENAMEGUESSER_H
