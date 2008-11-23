/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "EncodingDetector.h"

#include "include/chardet.h"

#include <iostream>

EncodingDetector::EncodingDetector( const char* str )
:m_str( str )
{
    m_gotResult = false;
}

EncodingDetector::~EncodingDetector()
{
}

void EncodingDetector::startTableDetector()
{
    size_t len = strlen( m_str );
    int res = 0;
    chardet_t det = NULL;
    char encoding[CHARDET_MAX_ENCODING_NAME];
    chardet_create( &det );
    res = chardet_handle_data( det, m_str, len );
    chardet_data_end( det );
    res = chardet_get_charset( det, encoding, CHARDET_MAX_ENCODING_NAME );
    chardet_destroy( det );

    QString track_encoding = encoding;
    std::cout << "encoding detector debug: encoding = " << encoding << std::endl;
    if ( res == CHARDET_RESULT_OK )
    {
        //http://doc.trolltech.com/4.4/qtextcodec.html
        //http://www.mozilla.org/projects/intl/chardet.html
        m_gotResult = true;
        if ( track_encoding == "IBM866" ) track_encoding = "IBM 866";
        //Qt cannot handle the following encodings
        if ( ( track_encoding == "x-euc-tw" )
            || ( track_encoding == "HZ-GB2312" )
            || ( track_encoding == "ISO-2022-CN" )
            || ( track_encoding == "ISO-2022-KR" )
            || ( track_encoding == "ISO-2022-JP" )
            || ( track_encoding == "x-mac-cyrillic" )
            || ( track_encoding == "IBM855" )
            || ( track_encoding == "TIS-620" )
            || track_encoding.isEmpty() )
            m_gotResult = false;
    }
    //FIXME: should return the confidence as well.
    m_encoding = track_encoding;
}

void EncodingDetector::startFileNameDetector()
{
    //TODO: detect the encoding by comparing with the filename
}

void EncodingDetector::startLocaleDetector()
{
    //TODO: detect the system locale
}

QString EncodingDetector::encoding() const
{
    return m_encoding;
}

bool EncodingDetector::gotResult() const
{
    return m_gotResult;
}

#include "EncodingDetector.moc"
