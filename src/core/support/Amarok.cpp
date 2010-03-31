/****************************************************************************************
 * Copyright (c) 2002 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "core/support/Amarok.h"

#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"


#include <QDateTime>
#include <QTextDocument>

#include <KCalendarSystem>
#include <KConfigGroup>
#include <KDirLister>
#include <KStandardDirs>
#include <KUniqueApplication>

QPointer<KActionCollection> Amarok::actionCollectionObject;
QMutex Amarok::globalDirsMutex;

namespace Amarok
{
    /*
    * Transform to be usable within HTML/XHTML attributes
    */
    QString escapeHTMLAttr( const QString &s )
    {
        return QString(s).replace( '%', "%25" ).replace( '\'', "%27" ).replace( '"', "%22" ).
                replace( '#', "%23" ).replace( '?', "%3F" );
    }
    QString unescapeHTMLAttr( const QString &s )
    {
        return QString(s).replace( "%3F", "?" ).replace( "%23", "#" ).replace( "%22", "\"" ).
                replace( "%27", "'" ).replace( "%25", "%" );
    }

    QString verboseTimeSince( const QDateTime &datetime )
    {
        const QDateTime now = QDateTime::currentDateTime();
        const int datediff = datetime.daysTo( now );

        // HACK: Fix 203522. Arithmetic overflow?
        // Getting weird values from Plasma::DataEngine (LAST_PLAYED field).
        if( datediff < 0 )
            return i18nc( "When this track was last played", "Unknown" );

        if( datediff >= 6*7 /*six weeks*/ ) {  // return absolute month/year
            const KCalendarSystem *cal = KGlobal::locale()->calendar();
            const QDate date = datetime.date();
            return i18nc( "monthname year", "%1 %2", cal->monthName(date),
                          cal->yearString(date, KCalendarSystem::LongFormat) );
        }

        //TODO "last week" = maybe within 7 days, but prolly before last Sunday

        if( datediff >= 7 )  // return difference in weeks
            return i18np( "One week ago", "%1 weeks ago", (datediff+3)/7 );

        const int timediff = datetime.secsTo( now );

        if( timediff >= 24*60*60 /*24 hours*/ )  // return difference in days
            return datediff == 1 ?
                    i18n( "Yesterday" ) :
                    i18np( "One day ago", "%1 days ago", (timediff+12*60*60)/(24*60*60) );

        if( timediff >= 90*60 /*90 minutes*/ )  // return difference in hours
            return i18np( "One hour ago", "%1 hours ago", (timediff+30*60)/(60*60) );

        //TODO are we too specific here? Be more fuzzy? ie, use units of 5 minutes, or "Recently"

        if( timediff >= 0 )  // return difference in minutes
            return timediff/60 ?
                    i18np( "One minute ago", "%1 minutes ago", (timediff+30)/60 ) :
                    i18n( "Within the last minute" );

        return i18n( "The future" );
    }

    QString verboseTimeSince( uint time_t )
    {
        if( !time_t )
            return i18nc( "The amount of time since last played", "Never" );

        QDateTime dt;
        dt.setTime_t( time_t );
        return verboseTimeSince( dt );
    }

    QString conciseTimeSince( uint time_t )
    {
        if( !time_t )
            return i18nc( "The amount of time since last played", "0" );

        QDateTime datetime;
        datetime.setTime_t( time_t );

        const QDateTime now = QDateTime::currentDateTime();
        const int datediff = datetime.daysTo( now );

        if( datediff >= 6*7 /*six weeks*/ ) {  // return difference in months
            return i18nc( "number of months ago", "%1M", datediff/7/4 );
        }

        if( datediff >= 7 )  // return difference in weeks
            return i18nc( "w for weeks", "%1w", (datediff+3)/7 );

        if( datediff == -1 )
            return i18nc( "When this track was last played", "Tomorrow" );

        const int timediff = datetime.secsTo( now );

        if( timediff >= 24*60*60 /*24 hours*/ )  // return difference in days
            // xgettext: no-c-format
            return i18nc( "d for days", "%1d", (timediff+12*60*60)/(24*60*60) );

        if( timediff >= 90*60 /*90 minutes*/ )  // return difference in hours
            return i18nc( "h for hours", "%1h", (timediff+30*60)/(60*60) );

        //TODO are we too specific here? Be more fuzzy? ie, use units of 5 minutes, or "Recently"

        if( timediff >= 60 )  // return difference in minutes
            return QString("%1'").arg( ( timediff + 30 )/60 );
        if( timediff >= 0 )  // return difference in seconds
            return QString("%1\"").arg( ( timediff + 1 )/60 );

        return i18n( "0" );
    }

    void manipulateThe( QString &str, bool reverse )
    {
        if( reverse )
        {
            if( !str.startsWith( "the ", Qt::CaseInsensitive ) )
                return;

            QString begin = str.left( 3 );
            str = str.append( ", %1" ).arg( begin );
            str = str.mid( 4 );
            return;
        }

        if( !str.endsWith( ", the", Qt::CaseInsensitive ) )
            return;

        QString end = str.right( 3 );
        str = str.prepend( "%1 " ).arg( end );

        uint newLen = str.length() - end.length() - 2;

        str.truncate( newLen );
    }

   KActionCollection* actionCollection()  // TODO: constify?
    {
        if( !actionCollectionObject )
        {
            actionCollectionObject = new KActionCollection( kapp );
            actionCollectionObject->setObjectName( "Amarok-KActionCollection" );
        }

        return actionCollectionObject;
    }

    KConfigGroup config( const QString &group )
    {
        //Slightly more useful config() that allows setting the group simultaneously
        return KGlobal::config()->group( group );
    }

    namespace ColorScheme
    {
        QColor Base;
        QColor Text;
        QColor Background;
        QColor Foreground;
        QColor AltBase;
    }

    OverrideCursor::OverrideCursor( Qt::CursorShape cursor )
    {
        QApplication::setOverrideCursor( cursor == Qt::WaitCursor ?
                                        Qt::WaitCursor :
                                        Qt::BusyCursor );
    }

    OverrideCursor::~OverrideCursor()
    {
        QApplication::restoreOverrideCursor();
    }

    QString saveLocation( const QString &directory )
    {
        globalDirsMutex.lock();
        QString result = KGlobal::dirs()->saveLocation( "data", QString("amarok/") + directory, true );
        globalDirsMutex.unlock();
        return result;
    }

    QString cleanPath( const QString &path )
    {
        /* Unicode uses combining characters to form accented versions of other characters.
         * (Exception: Latin-1 table for compatibility with ASCII.)
         * Those can be found in the Unicode tables listed at:
         * http://en.wikipedia.org/w/index.php?title=Combining_character&oldid=255990982
         * Removing those characters removes accents. :)                                   */
        QString result = path;

        // German umlauts
        result.replace( QChar(0x00e4), "ae" ).replace( QChar(0x00c4), "Ae" );
        result.replace( QChar(0x00f6), "oe" ).replace( QChar(0x00d6), "Oe" );
        result.replace( QChar(0x00fc), "ue" ).replace( QChar(0x00dc), "Ue" );
        result.replace( QChar(0x00df), "ss" );

        // other special cases
        result.replace( QChar(0x00C6), "AE" );
        result.replace( QChar(0x00E6), "ae" );

        result.replace( QChar(0x00D8), "OE" );
        result.replace( QChar(0x00F8), "oe" );

        // normalize in a form where accents are separate characters
        result = result.normalized( QString::NormalizationForm_D );

        // remove accents from table "Combining Diacritical Marks"
        for( int i = 0x0300; i <= 0x036F; i++ )
        {
            result.remove( QChar( i ) );
        }

        return result;
    }

    QString asciiPath( const QString &path )
    {
        QString result = path;
        for( int i = 0; i < result.length(); i++ )
        {
            QChar c = result[ i ];
            if( c > QChar(0x7f) || c == QChar(0) )
            {
                c = '_';
            }
            result[ i ] = c;
        }
        return result;
    }

    QString vfatPath( const QString &path )
    {
        QString s = path;

        if( QDir::separator() == '/' ) // we are on *nix, \ is a valid character in file or directory names, NOT the dir separator
            s.replace( '\\', '_' );
        else
            s.replace( '/', '_' ); // on windows we have to replace / instead

            for( int i = 0; i < s.length(); i++ )
            {
                QChar c = s[ i ];
                if( c < QChar(0x20) || c == QChar(0x7F) // 0x7F = 127 = DEL control character
                    || c=='*' || c=='?' || c=='<' || c=='>'
                    || c=='|' || c=='"' || c==':' )
                    c = '_';
                else if( c == '[' )
                    c = '(';
                else if ( c == ']' )
                    c = ')';
                s[ i ] = c;
            }

        /* beware of reserved device names */
        uint len = s.length();
        if( len == 3 || (len > 3 && s[3] == '.') )
        {
            QString l = s.left(3).toLower();
            if( l=="aux" || l=="con" || l=="nul" || l=="prn" )
                s = '_' + s;
        }
        else if( len == 4 || (len > 4 && s[4] == '.') )
        {
            QString l = s.left(3).toLower();
            QString d = s.mid(3,1);
            if( (l=="com" || l=="lpt") &&
                    (d=="0" || d=="1" || d=="2" || d=="3" || d=="4" ||
                     d=="5" || d=="6" || d=="7" || d=="8" || d=="9") )
                s = '_' + s;
        }

        // "clock$" is only allowed WITH extension, according to:
        // http://en.wikipedia.org/w/index.php?title=Filename&oldid=303934888#Comparison_of_file_name_limitations
        if( QString::compare( s, "clock$", Qt::CaseInsensitive ) == 0 )
            s = '_' + s;

        /* max path length of Windows API */
        s = s.left(255);

        /* whitespace at the end of folder/file names or extensions are bad */
        len = s.length();
        if( s[len-1] == ' ' )
            s[len-1] = '_';

        int extensionIndex = s.lastIndexOf( '.' ); // correct trailing spaces in file name itself
        if( ( s.length() > 1 ) &&  ( extensionIndex > 0 ) )
            if( s.at( extensionIndex - 1 ) == ' ' )
                s[extensionIndex - 1] = '_';

        for( int i = 1; i < s.length(); i++ ) // correct trailing whitespace in folder names
        {
            if( ( s.at( i ) == QDir::separator() ) && ( s.at( i - 1 ) == ' ' ) )
                s[i - 1] = '_';
        }

        return s;
    }

    /* Strip the common prefix of two strings from the first one and trim
     * whitespace from the beginning of the resultant string.
     * Case-insensitive.
     *
     * @param input the string being processed
     * @param ref the string used to determine prefix
     */
    QString decapitateString( const QString &input, const QString &ref )
    {
        int len;    //the length of common prefix calculated so far
        for ( len = 0; len < input.length() && len < ref.length(); len++ )
        {
            if ( input.at( len ).toUpper() != ref.at( len ).toUpper() )
                break;
        }

        return input.right( input.length() - len ).trimmed();
    }

    //this function (C) Copyright 2003-4 Max Howell, (C) Copyright 2004 Mark Kretschmann
    KUrl::List
    recursiveUrlExpand ( const KUrl &url )
    {
        typedef QMap<QString, KUrl> FileMap;

        KDirLister lister ( false );
        lister.setAutoUpdate ( false );
        lister.setAutoErrorHandlingEnabled ( false, 0 );
        lister.openUrl ( url );

        while ( !lister.isFinished() )
            kapp->processEvents ( QEventLoop::ExcludeUserInputEvents );

        KFileItemList items = lister.items();
        KUrl::List urls;
        FileMap files;
        foreach ( const KFileItem& it, items )
        {
            if ( it.isFile() ) { files[it.name() ] = it.url(); continue; }
            if ( it.isDir() ) urls += recursiveUrlExpand( it.url() );
        }

        oldForeachType ( FileMap, files )
        // users often have playlist files that reflect directories
        // higher up, or stuff in this directory. Don't add them as
        // it produces double entries
        if ( !Playlists::isPlaylist( ( *it ).fileName() ) )
            urls += *it;
        return urls;
    }

    KUrl::List
    recursiveUrlExpand ( const KUrl::List &list )
    {
        KUrl::List urls;
        oldForeachType ( KUrl::List, list )
        {
            urls += recursiveUrlExpand ( *it );
        }

        return urls;
    }
} // End namespace Amarok
