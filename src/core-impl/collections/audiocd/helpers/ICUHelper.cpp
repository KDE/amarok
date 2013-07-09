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
#include "ICUHelper.h"

#include "unicode/ucsdet.h"

bool ICUHelper::detectAllEncodings( QVector<QString> &encodings ) const
{
        /** this code is a part of Chromium project */
        const char* text = m_samples.data();
        int length = m_samples.length();

        UErrorCode status = U_ZERO_ERROR;
        UCharsetDetector* detector = ucsdet_open( &status );
        ucsdet_setText( detector, text, length, &status );
        int matches_count = 0;
        const UCharsetMatch** matches = ucsdet_detectAll( detector,
                                                         &matches_count,
                                                         &status );
        if ( U_FAILURE( status ) )
        {
            ucsdet_close(detector);
            return false;
        }

        // ICU has some heuristics for encoding detection, such that the more likely
        // encodings should be returned first. However, it doesn't always return
        // all encodings that properly decode |text|, so we'll append more encodings
        // later. To make that efficient, keep track of encodings sniffed in this
        // first phase.
        QSet<QString> sniffed_encodings;

        encodings.clear();
        for ( int i = 0; i < matches_count; i++ )
        {
            UErrorCode get_name_status = U_ZERO_ERROR;
            const char* encoding_name = ucsdet_getName( matches[i], &get_name_status );

            // If we failed to get the encoding's name, ignore the error.
            if ( U_FAILURE( get_name_status ) )
                continue;

            int32_t confidence = ucsdet_getConfidence( matches[i], &get_name_status );

            // We also treat this error as non-fatal.
            if ( U_FAILURE( get_name_status ) )
                continue;

            // A confidence level >= 10 means that the encoding is expected to properly
            // decode the text. Drop all encodings with lower confidence level.
            if ( confidence < 10 )
                continue;
            encodings.append( encoding_name );
            sniffed_encodings.insert( encoding_name );
        }

        // Append all encodings not included earlier, in arbitrary order.
        // TODO(jshin): This shouldn't be necessary, possible ICU bug.
        // See also http://crbug.com/65917.
        UEnumeration* detectable_encodings = ucsdet_getAllDetectableCharsets( detector,
                                                                              &status );
        int detectable_count = uenum_count( detectable_encodings, &status );
        for ( int i = 0; i < detectable_count; i++ )
        {
            int name_length;
            const char* name_raw = uenum_next( detectable_encodings,
                                               &name_length,
                                               &status );
            QString name( name_raw );
            if ( sniffed_encodings.find( name ) == sniffed_encodings.end() )
                encodings.append( name );
        }
        uenum_close( detectable_encodings );

        ucsdet_close( detector );
        return !encodings.empty();
}
