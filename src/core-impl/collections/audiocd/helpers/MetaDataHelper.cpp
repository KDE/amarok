/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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

#include "MetaDataHelper.h"

#include <KEncodingProber>
#include <QTextCodec>

MetaDataHelper::MetaDataHelper( const QString& encodingPreferences )
               : m_encodingPreferences( encodingPreferences )
{
};

MetaDataHelper::~MetaDataHelper()
{
};

QString
MetaDataHelper::encode( const char* field ) const
{
    // collects samples for ecoding guessing
    m_icu.addSample( field );
    if ( m_encodingPreferences.isEmpty() )
    {
        KEncodingProber prober;
        KEncodingProber::ProberState result = prober.feed( field );
        if( result != KEncodingProber::NotMe && prober.confidence() > 0.6 )
        {
            QTextCodec* codec = QTextCodec::codecForName( prober.encoding() );
            if ( codec )
                return codec->toUnicode( field );
        }
    }
    else
    {
        QTextCodec* codec = QTextCodec::codecForName( m_encodingPreferences.toUtf8() );
        if ( codec )
            return codec->toUnicode( field );
    }
    return QString( field );
}

void
MetaDataHelper::getEncodings( QVector<QString> &encodings ) const
{
    m_icu.detectAllEncodings( encodings );
}
