/*
    Copyright (c) 2008 Soren Harward <stharward@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Fingerprint.h"

#include <QStringList>
#ifdef HAVE_FINGERPRINT
#include <gtacfeat/fingerprint.h>
#endif

#include "Debug.h"


Fingerprint::Fingerprint::Fingerprint( const QString& fpstr )
{
    v = parseString( fpstr );
}

Fingerprint::Fingerprint::~Fingerprint() { }

bool
Fingerprint::Fingerprint::exists() const
{
    return ( bool )( v.size() > 0 );
}

// returns a value between 0.0 (not similar at all) and 1.0 (exactly the same)
// or returns Fingerprint::INVALID_SIMILARITY if the similarity can't be calculated
Fingerprint::Similarity
Fingerprint::Fingerprint::calcSimilarity( const FingerprintPtr other ) const
{
#ifdef HAVE_FINGERPRINT
    return ( Similarity )fprint_similarity( v.constData(), other->v.constData(), GTACFEAT_FINGERPRINT_FIELDS );
#else
    Q_UNUSED( other )
    return INVALID_SIMILARITY;
#endif
}

const QString
Fingerprint::Fingerprint::toString() const
{
    QStringList vstr;

    for ( int i = 0; i < v.size(); i++ ) {
        vstr.append( QString::number( v[i] ) );
    }
    return vstr.join( "," );
}

// PRIVATE FUNCTIONS
Fingerprint::Vector
Fingerprint::Fingerprint::parseString( const QString& str ) const
{
    Vector x;

    QStringList strl = str.split( ":" );
    if ( strl.size() != 2 ) {
        x.clear();
        return x;
    }

    QString versionstr = strl[0];
    //version = versionstr.toInt();

    QString datastr = strl[1];
    QStringList data = datastr.split( "," );
    bool ok = true;

    for ( int i = 0; i < data.size(); i++ ) {
        x.append( data[i].toDouble( &ok ) );
        if ( !ok ) {
            x.clear();
            return x;
        }
    }
    return x;
}
