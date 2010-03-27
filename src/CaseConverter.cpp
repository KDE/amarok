/****************************************************************************************
 * Copyright (c) 2008 Nicos Gollan <gtdev@spearhead.de>                                 *
 * Copyright (c) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#define DEBUG_PREFIX "CaseConverter"

#include "CaseConverter.h"

#include "core/support/Debug.h"

#include <QObject>
#include <QString>
#include <QRegExp>

namespace Amarok
{
const QString CaseConverter::s_MATCH_A_WORD( "\\b([\\w']+)\\b" );
const QString CaseConverter::s_LITTLE_WORDS( "\\b(a|an|and|as|at|by|for|if|in|of|on|or|to|the)\\b" );

QString
CaseConverter::toTitleCase( const QString &s )
{
    QString result = s;
    debug() << "Original string: " << s;

    QRegExp wordRegExp( CaseConverter::s_MATCH_A_WORD );
    int i = wordRegExp.indexIn( result );
    QString match = wordRegExp.cap( 1 );
    bool first = true;

    QRegExp littleWordRegExp( CaseConverter::s_LITTLE_WORDS );
    while ( i > -1 )
    {
        debug() << "  Title case i=" << i << "; remaining: \"" << result.mid( i ) << "\"";

        // uppercase if:
        //  * no uppercase letters in partial AND first partial
        // OR
        //  * no uppercase letters in partial AND not a "little" word
        if ( match == match.toLower() && ( first || !littleWordRegExp.exactMatch( match ) ) )
        {
            result[i] = result[i].toUpper();
        }
        else
        {
            debug() << "  partial will not be capitalized: \"" << match << "\"";
        }

        i = wordRegExp.indexIn( result, i + match.length() );
        match = wordRegExp.cap( 1 );
        first = false;
    }

    debug() << "  Title case of \"" << s << "\" = \"" << result << "\"";
    return result;
}

QString
CaseConverter::toCapitalizedCase( const QString &s )
{
    QString result = s;
    QRegExp wordRegExp( CaseConverter::s_MATCH_A_WORD );
    int i = wordRegExp.indexIn( result );
    int ml = wordRegExp.cap( 1 ).length();
    while ( i > -1 )
    {
        result[i] = result[i].toUpper();
        i = wordRegExp.indexIn( result, i + ml );
        ml = wordRegExp.cap( 1 ).length();
    }
    return result;
}
}
