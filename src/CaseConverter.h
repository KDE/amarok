/****************************************************************************************
 * Copyright (c) 2008 Nicos Gollan <gtdev@spearhead.de>                                 *
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef CASECONVERTER_H
#define CASECONVERTER_H

#include <QString>

namespace Amarok
{
/** Case converter for tag formatting.
  *
  * Provides helper functions to achieve sane capitalization of tag
  * information.
  */
class CaseConverter
{
public:
    /** Convert to "title case".
    *
    * Title case tries to conform to the common capitalization of titles,
    * i.e. first letter of each word is capitalized, except for "small"
    * words like "in", "of", etc.
    *
    * This implementation will also leave alone words that already have
    * some kind of capitalization, assuming that those are properly
    * formatted.
    *
    * @param s A string to be converted
    * @return The converted string
    */
    static QString toTitleCase( const QString &s );
    /** Convert to "capitalized case"
    *
    * Capitalizes the initial letter of each word.
    *
    * @param s A string to be converted
    * @return The converted string
    */
    static QString toCapitalizedCase( const QString &s );
private:
    /// regular expression for a word.
    static const QString s_MATCH_A_WORD;
    /** "small words" that ought not be capitalized.
    *
    * This is mostly English only.
    */
    static const QString s_LITTLE_WORDS;
};
}

#endif //CASECONVERTER_H
