/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007-2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "TextualQueryFilter"

#include "TextualQueryFilter.h"
#include "Expression.h"

#include "FileType.h"
#include "core/support/Debug.h"

#include <KLocalizedString>

using namespace Meta;


#define ADD_OR_EXCLUDE_FILTER( VALUE, FILTER, MATCHBEGIN, MATCHEND ) \
            { if( elem.negate ) \
                qm->excludeFilter( VALUE, FILTER, MATCHBEGIN, MATCHEND ); \
            else \
                qm->addFilter( VALUE, FILTER, MATCHBEGIN, MATCHEND ); }
#define ADD_OR_EXCLUDE_NUMBER_FILTER( VALUE, FILTER, COMPARE ) \
            { if( elem.negate ) \
                qm->excludeNumberFilter( VALUE, FILTER, COMPARE ); \
            else \
                qm->addNumberFilter( VALUE, FILTER, COMPARE ); }

void
Collections::addTextualFilter( Collections::QueryMaker *qm, const QString &filter )
{
    const int validFilters = qm->validFilterMask();

    ParsedExpression parsed = ExpressionParser::parse( filter );
    for( const or_list &orList : parsed )
    {
        qm->beginOr();

        for( const expression_element &elem : orList )
        {
            if( elem.negate )
                qm->beginAnd();
            else
                qm->beginOr();

            if ( elem.field.isEmpty() )
            {
                qm->beginOr();

                if( ( validFilters & Collections::QueryMaker::TitleFilter ) )
                    ADD_OR_EXCLUDE_FILTER( Meta::valTitle, elem.text, false, false );
                if( ( validFilters & Collections::QueryMaker::UrlFilter ) )
                    ADD_OR_EXCLUDE_FILTER( Meta::valUrl, elem.text, false, false );
                if( ( validFilters & Collections::QueryMaker::AlbumFilter ) )
                    ADD_OR_EXCLUDE_FILTER( Meta::valAlbum, elem.text, false, false );
                if( ( validFilters & Collections::QueryMaker::ArtistFilter ) )
                    ADD_OR_EXCLUDE_FILTER( Meta::valArtist, elem.text, false, false );
                if( ( validFilters & Collections::QueryMaker::AlbumArtistFilter ) )
                    ADD_OR_EXCLUDE_FILTER( Meta::valAlbumArtist, elem.text, false, false );
                if( ( validFilters & Collections::QueryMaker::ComposerFilter ) )
                    ADD_OR_EXCLUDE_FILTER( Meta::valComposer, elem.text, false, false );
                if( ( validFilters & Collections::QueryMaker::GenreFilter ) )
                    ADD_OR_EXCLUDE_FILTER( Meta::valGenre, elem.text, false, false );
                if( ( validFilters & Collections::QueryMaker::YearFilter ) )
                    ADD_OR_EXCLUDE_FILTER( Meta::valYear, elem.text, false, false );

                ADD_OR_EXCLUDE_FILTER( Meta::valLabel, elem.text, false, false );

                qm->endAndOr();
            }
            else
            {
                //get field values based on name
                const qint64 field = Meta::fieldForName( elem.field );
                Collections::QueryMaker::NumberComparison compare = Collections::QueryMaker::Equals;
                switch( elem.match )
                {
                    case expression_element::More:
                        compare = Collections::QueryMaker::GreaterThan;
                        break;
                    case expression_element::Less:
                        compare = Collections::QueryMaker::LessThan;
                        break;
                    case expression_element::Equals:
                    case expression_element::Contains:
                        compare = Collections::QueryMaker::Equals;
                        break;
                }

                const bool matchEqual = ( elem.match == expression_element::Equals );

                switch( field )
                {
                    case Meta::valAlbum:
                        if( ( validFilters & Collections::QueryMaker::AlbumFilter ) == 0 ) break;
                        ADD_OR_EXCLUDE_FILTER( field, elem.text, matchEqual, matchEqual );
                        break;
                    case Meta::valArtist:
                        if( ( validFilters & Collections::QueryMaker::ArtistFilter ) == 0 ) break;
                        ADD_OR_EXCLUDE_FILTER( field, elem.text, matchEqual, matchEqual );
                        break;
                    case Meta::valAlbumArtist:
                        if( ( validFilters & Collections::QueryMaker::AlbumArtistFilter ) == 0 ) break;
                        ADD_OR_EXCLUDE_FILTER( field, elem.text, matchEqual, matchEqual );
                        break;
                    case Meta::valGenre:
                        if( ( validFilters & Collections::QueryMaker::GenreFilter ) == 0 ) break;
                        ADD_OR_EXCLUDE_FILTER( field, elem.text, matchEqual, matchEqual );
                        break;
                    case Meta::valTitle:
                        if( ( validFilters & Collections::QueryMaker::TitleFilter ) == 0 ) break;
                        ADD_OR_EXCLUDE_FILTER( field, elem.text, matchEqual, matchEqual );
                        break;
                    case Meta::valComposer:
                        if( ( validFilters & Collections::QueryMaker::ComposerFilter ) == 0 ) break;
                        ADD_OR_EXCLUDE_FILTER( field, elem.text, matchEqual, matchEqual );
                        break;
                    case Meta::valYear:
                        if( ( validFilters & Collections::QueryMaker::YearFilter ) == 0 ) break;
                        ADD_OR_EXCLUDE_NUMBER_FILTER( field, elem.text.toInt(), compare );
                        break;
                    case Meta::valLabel:
                    case Meta::valComment:
                        ADD_OR_EXCLUDE_FILTER( field, elem.text, matchEqual, matchEqual );
                        break;
                    case Meta::valUrl:
                        ADD_OR_EXCLUDE_FILTER( field, elem.text, false, false );
                        break;
                    case Meta::valBpm:
                    case Meta::valBitrate:
                    case Meta::valScore:
                    case Meta::valPlaycount:
                    case Meta::valSamplerate:
                    case Meta::valDiscNr:
                    case Meta::valTrackNr:
                        ADD_OR_EXCLUDE_NUMBER_FILTER( field, elem.text.toInt(), compare );
                        break;
                    case Meta::valRating:
                        ADD_OR_EXCLUDE_NUMBER_FILTER( field, elem.text.toFloat() * 2, compare );
                        break;
                    case Meta::valLength:
                        ADD_OR_EXCLUDE_NUMBER_FILTER( field, elem.text.toInt() * 1000, compare );
                        break;
                    case Meta::valLastPlayed:
                    case Meta::valFirstPlayed:
                    case Meta::valCreateDate:
                    case Meta::valModified:
                        addDateFilter( field, compare, elem.negate, elem.text, qm );
                        break;
                    case Meta::valFilesize:
                    {
                        bool doubleOk( false );
                        const double mbytes = elem.text.toDouble( &doubleOk ); // input in MBs
                        if( !doubleOk )
                        {
                            qm->endAndOr();
                            return;
                        }
                        /*
                        * A special case is made for Equals (e.g. filesize:100), which actually filters
                        * for anything between 100 and 101MBs. Megabytes are used because for audio files
                        * they are the most reasonable units for the user to deal with.
                        */
                        const qreal bytes = mbytes * 1024.0 * 1024.0;
                        const qint64 mbFloor = qint64( qAbs(mbytes) );
                        switch( compare )
                        {
                        case Collections::QueryMaker::Equals:
                            qm->endAndOr();
                            qm->beginAnd();
                            ADD_OR_EXCLUDE_NUMBER_FILTER( field, mbFloor * 1024 * 1024, Collections::QueryMaker::GreaterThan );
                            ADD_OR_EXCLUDE_NUMBER_FILTER( field, (mbFloor + 1) * 1024 * 1024, Collections::QueryMaker::LessThan );
                            break;
                        case Collections::QueryMaker::GreaterThan:
                        case Collections::QueryMaker::LessThan:
                            ADD_OR_EXCLUDE_NUMBER_FILTER( field, bytes, compare );
                            break;
                        }
                        break;
                    }
                    case Meta::valFormat:
                    {
                        const QString &ftStr = elem.text;
                        Amarok::FileType ft = Amarok::FileTypeSupport::fileType(ftStr);
                        ADD_OR_EXCLUDE_NUMBER_FILTER( field, int(ft), compare );
                        break;
                    }
                }
            }
            qm->endAndOr();
        }
        qm->endAndOr();
    }
}

void
Collections::addDateFilter( qint64 field, Collections::QueryMaker::NumberComparison compare,
                            bool negate, const QString &text, Collections::QueryMaker *qm )
{
    bool absolute = false;
    const uint date = semanticDateTimeParser( text, &absolute ).toSecsSinceEpoch();
    if( date == 0 )
        return;

    if( compare == Collections::QueryMaker::Equals )
    {
        // equal means, on the same day
        uint day = 24 * 60 * 60;

        qm->endAndOr();
        qm->beginAnd();

        if( negate )
        {
            qm->excludeNumberFilter( field, date - day, Collections::QueryMaker::GreaterThan );
            qm->excludeNumberFilter( field, date + day, Collections::QueryMaker::LessThan );
        }
        else
        {
            qm->addNumberFilter( field, date - day, Collections::QueryMaker::GreaterThan );
            qm->addNumberFilter( field, date + day, Collections::QueryMaker::LessThan );
        }
    }
    // note: if the date is a relative time difference, invert the condition
    else if( ( compare == Collections::QueryMaker::LessThan && !absolute ) || ( compare == Collections::QueryMaker::GreaterThan && absolute ) )
    {
        if( negate )
            qm->excludeNumberFilter( field, date, Collections::QueryMaker::GreaterThan );
        else
            qm->addNumberFilter( field, date, Collections::QueryMaker::GreaterThan );
    }
    else if( ( compare == Collections::QueryMaker::GreaterThan && !absolute ) || ( compare == Collections::QueryMaker::LessThan && absolute ) )
    {
        if( negate )
            qm->excludeNumberFilter( field, date, Collections::QueryMaker::LessThan );
        else
            qm->addNumberFilter( field, date, Collections::QueryMaker::LessThan );
    }
}

QDateTime
Collections::semanticDateTimeParser( const QString &text, bool *absolute )
{
    /* TODO: semanticDateTimeParser: has potential to extend and form a class of its own */
    // some code duplication, see EditFilterDialog::parseTextFilter

    const QString lowerText = text.toLower();
    const QDateTime curTime = QDateTime::currentDateTime();

    if( absolute )
        *absolute = false;

    // parse date using local settings
    QDateTime result = QLocale().toDate( text, QLocale::ShortFormat ).startOfDay();

    // parse date using a backup standard independent from local settings
    QRegularExpression shortDateReg("(\\d{1,2})[-.](\\d{1,2})");
    QRegularExpression longDateReg("(\\d{1,2})[-.](\\d{1,2})[-.](\\d{4})");

    if( text.at(0).isLetter() )
    {
        if( ( lowerText.compare( QLatin1String("today") ) == 0 ) || ( lowerText.compare( i18n( "today" ) ) == 0 ) )
            result = curTime.addDays( -1 );
        else if( ( lowerText.compare( QLatin1String("last week") ) == 0 ) || ( lowerText.compare( i18n( "last week" ) ) == 0 ) )
            result = curTime.addDays( -7 );
        else if( ( lowerText.compare( QLatin1String("last month") ) == 0 ) || ( lowerText.compare( i18n( "last month" ) ) == 0 ) )
            result = curTime.addMonths( -1 );
        else if( ( lowerText.compare( QLatin1String("two months ago") ) == 0 ) || ( lowerText.compare( i18n( "two months ago" ) ) == 0 ) )
            result = curTime.addMonths( -2 );
        else if( ( lowerText.compare( QLatin1String("three months ago") ) == 0 ) || ( lowerText.compare( i18n( "three months ago" ) ) == 0 ) )
            result = curTime.addMonths( -3 );
    }
    else if( result.isValid() )
    {
        if( absolute )
            *absolute = true;
    }
    else if( text.contains(longDateReg) )
    {
        QRegularExpressionMatch rmatch = longDateReg.match( text );
        result = QDate( rmatch.captured(3).toInt(), rmatch.captured(2).toInt(), rmatch.captured(1).toInt() ).startOfDay();
        if( absolute )
            *absolute = true;
    }
    else if( text.contains(shortDateReg) )
    {
        QRegularExpressionMatch rmatch = shortDateReg.match( text );
        result = QDate( QDate::currentDate().year(), rmatch.captured(2).toInt(), rmatch.captured(1).toInt() ).startOfDay();
        if( absolute )
            *absolute = true;
    }
    else // first character is a number
    {
        // parse a "#m#d" (discoverability == 0, but without a GUI, how to do it?)
        int years = 0, months = 0, days = 0, secs = 0;
        QString tmp;
        for( int i = 0; i < text.length(); i++ )
        {
            QChar c = text.at( i );
            if( c.isNumber() )
            {
                tmp += c;
            }
            else if( c == 'y' )
            {
                years += -tmp.toInt();
                tmp.clear();
            }
            else if( c == 'm' )
            {
                months += -tmp.toInt();
                tmp.clear();
            }
            else if( c == 'w' )
            {
                days += -tmp.toInt() * 7;
                tmp.clear();
            }
            else if( c == 'd' )
            {
                days += -tmp.toInt();
                tmp.clear();
            }
            else if( c == 'h' )
            {
                secs += -tmp.toInt() * 60 * 60;
                tmp.clear();
            }
            else if( c == 'M' )
            {
                secs += -tmp.toInt() * 60;
                tmp.clear();
            }
            else if( c == 's' )
            {
                secs += -tmp.toInt();
                tmp.clear();
            }
        }
        result = QDateTime::currentDateTime().addYears( years ).addMonths( months ).addDays( days ).addSecs( secs );
    }
    return result;
}

