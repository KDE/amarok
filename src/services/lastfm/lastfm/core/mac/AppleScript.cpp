/***************************************************************************
 *   Copyright 2008 Last.fm Ltd.                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "AppleScript.h"
#include <QDebug>

ComponentInstance AppleScript::s_component = NULL;


AppleScript::AppleScript( const QString& code )
           : m_compiled_script( kOSANullScript ),
             m_code( code )
{
    if (!s_component)
        s_component = OpenDefaultComponent( kOSAComponentType, typeAppleScript );
}


AppleScript::~AppleScript()
{
    OSADispose( s_component, m_compiled_script );
}


bool //static
AppleScript::isAppleScriptAvailable()
{
    long r;
    if (Gestalt( gestaltAppleScriptAttr, &r ) != noErr)
        r = 0;
    return (r & (1 << gestaltAppleScriptPresent)) != 0;
}


static inline QString
qStringFromAEDesc( AEDesc d )
{
    const int N = AEGetDescDataSize( &d ) + 1;
    QByteArray r( N, '\0' );
    AEGetDescData( &d, r.data(), r.size() );
    // sometimes iTunes gives us "bonus" nulls for each returned applescript
    // string component
    return QString::fromUtf8( r.data(), r.size() ).remove( QChar::Null );
}


void
AppleScript::logError()
{
    AEDesc d;
    OSAScriptError( s_component, kOSAErrorMessage, typeUTF8Text, &d );
    qWarning() << "ERROR: AppleScript:"
               << m_compiled_script << '-'
               << qStringFromAEDesc( d );
}


void
AppleScript::compile()
{
    QByteArray const utf8 = m_code.toUtf8();
    void const* text = utf8.data();
    long const textLength = utf8.length();
    
    AEDesc d;
    AECreateDesc( typeUTF8Text, text, textLength, &d );

    OSStatus err = OSACompile( s_component, &d, kOSAModeNull, &m_compiled_script );

    qDebug() << "Compiled:" << m_compiled_script << '\n' << m_code.trimmed();
    
    if (err != noErr)
        logError();
}


/** NOTE old, unedited text may not all apply:-
  *
  * LowRunAppleScript compiles and runs an AppleScript
  * provided as text in the buffer pointed to by text.  textLength
  * bytes will be compiled from this buffer and run as an AppleScript
  * using all of the default environment and execution settings.  If
  * resultData is not NULL, then the result returned by the execution
  * command will be returned as typeChar in this descriptor record
  * (or typeNull if there is no result information).  If the function
  * returns errOSAScriptError, then resultData will be set to a
  * descriptive error message describing the error (if one is
  * available). */
QString
AppleScript::exec()
{
    if (m_compiled_script == kOSANullScript)
        compile();

    OSAID id = kOSANullScript;
    OSStatus err = OSAExecute( s_component,
                               m_compiled_script,
                               kOSANullScript,
                               kOSAModeAlwaysInteract,
                               &id );

    if (err != noErr)
    {
        logError();
        return "";
    }

    if (id == kOSANullScript) // means script succeeded with no output
        return "";

    AEDesc r;
    OSADisplay( s_component, id, typeUTF8Text, kOSAModeNull, &r );
    QString s = qStringFromAEDesc( r );
    OSADispose( s_component, id );

    // strip surrounding quotes if any
    if (s.startsWith( '"' ) && s.endsWith( '"' ))
    {
        s = s.mid( 1, s.length() - 2 );
    }

    // iTunes also likes to escape quotes
    s.replace( "\\\"", "\"" );

    qDebug() << "Output from script:" << m_compiled_script << s;

    return s;
}


/** Due to the fact Apple Script can't work with Unicode characters directly 
  * written by Qt into the script, we convert the Unicode data from a QString 
  * into an AppleScript function + the hex values of the QString data (for 
  * example: "<<data utxt00AB00BB>> as unicode text") 
  * NOTE AppleScript 2 supports full unicode without this crap, but this works
  * with all versions
  */
QString //static
AppleScript::asUnicodeText( const QString& string )
{	
    if (string.isEmpty())
        return string;

    QString const OPEN = QChar(0x00AB) + QString("data utxt");
    QString const CLOSE = QChar(0x00BB) + QString(" as unicode text");
    QString r;

    for (int i = 0, j = 0; i < string.length(); i++)
    {
        QString hexadecimal = QString::number( string[i].unicode(), 16 );
        r += hexadecimal.rightJustified( 4, '0' );

        // AppleScript is only able to work with 252 / 4 unicode chars at once
        // (haha, yeah not 64 - the "utxt" prefix counts also and is 4 chars
        // long, so 64 - 1 = 63), everything else would cause a compilation
        // error. Therefore we split the string every 63 bytes and begin a new
        // "<<data utxt..." function.
        if (!(++j &= 63))
        {
            r += OPEN + " & " + CLOSE;
        }
    }

    return OPEN + r + CLOSE;
}
