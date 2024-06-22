/***************************************************************************
 *   Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>            *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#define DEBUG_PREFIX "TestDebug"

#include "core/support/Debug.h"
#include "config-amarok-test.h"

#include <QStack>
#include <QTest>

class TestDebug : public QObject
{
Q_OBJECT

public:
    TestDebug() {}

private slots:
    void benchDebugBlock();
    void benchDebugBlock_data();

private:
    void work( bool debugEnabled, bool colorEnabled );
    void work2( bool debugEnabled, bool colorEnabled );

    enum BeginOrEnd {
        Begin,
        End
    };
    void expectMessage( const QString &message, bool debugEnabled );
    void expectBeginEnd( BeginOrEnd type, const QString &message, bool debugEnabled,
                         bool colorEnabled );
    QString colorize( const QString &string, int colorIndex, bool colorEnabled );

    static QString m_indent;
};

QString TestDebug::m_indent;

void TestDebug::benchDebugBlock_data()
{
    QTest::addColumn<bool>("debugEnabled");
    QTest::addColumn<bool>("colorEnabled");

    QTest::newRow("debug with color")   << true  << true;
    QTest::newRow("debug nocolor")      << true  << false;
    QTest::newRow("nodebug with color") << false << true;
    QTest::newRow("nodebug nocolor")    << false << false;
}

void TestDebug::benchDebugBlock()
{
    QFETCH(bool, debugEnabled);
    QFETCH(bool, colorEnabled);

    Debug::setDebugEnabled( debugEnabled );
    Debug::setColoredDebug( colorEnabled );

    QVERIFY( Debug::debugEnabled() == debugEnabled );
    QVERIFY( Debug::debugColorEnabled() == colorEnabled );

    QBENCHMARK_ONCE {
        work( debugEnabled, colorEnabled );
    }
}

void TestDebug::work( bool debugEnabled, bool colorEnabled )
{
    expectBeginEnd( Begin, __PRETTY_FUNCTION__, debugEnabled, colorEnabled );
    DEBUG_BLOCK
    expectMessage( "level 1", debugEnabled );
    debug() << "level 1";

    for( int i = 0; i < 100; ++i )
    {
        expectBeginEnd( Begin, __PRETTY_FUNCTION__, debugEnabled, colorEnabled );
        DEBUG_BLOCK
        expectMessage( "level 2", debugEnabled );
        debug() << "level 2";
        work2( debugEnabled, colorEnabled );
        expectBeginEnd( End, __PRETTY_FUNCTION__, debugEnabled, colorEnabled );
    }
    expectBeginEnd( End, __PRETTY_FUNCTION__, debugEnabled, colorEnabled );
}

void TestDebug::work2( bool debugEnabled, bool colorEnabled )
{
    expectBeginEnd( Begin, __PRETTY_FUNCTION__, debugEnabled, colorEnabled );
    DEBUG_BLOCK
    for( int j = 0; j < 10; ++j )
    {
        expectMessage( "limbo", debugEnabled );
        debug() << "limbo";
    }
    expectBeginEnd( End, __PRETTY_FUNCTION__, debugEnabled, colorEnabled );
}

void
TestDebug::expectMessage( const QString &message, bool debugEnabled )
{
    if( !debugEnabled )
        return;
    QString exp = QString( "%1:%2 [%3] %4 " ).arg( "amarok", m_indent, DEBUG_PREFIX, message );
    QTest::ignoreMessage( QtDebugMsg, exp.toLocal8Bit() );
}

void
TestDebug::expectBeginEnd( TestDebug::BeginOrEnd type, const QString &message,
                           bool debugEnabled, bool colorEnabled )
{
    static int colorIndex = 0;
    static QStack<int> colorStack;
    if( !debugEnabled )
        return;

    QString beginEnd;
    QString took;
    if( type == Begin )
    {
        beginEnd = "BEGIN:";
        colorStack.push( colorIndex );
        colorIndex = (colorIndex + 1) % 5;
    }
    else
    {
        beginEnd = "END__:";
        double duration = DEBUG_OVERRIDE_ELAPSED_TIME;
        took = QLatin1Char( ' ' ) + colorize( QString( "[Took: %1s]" ).arg( duration, 0, 'g', 2 ),
            colorStack.top(), colorEnabled );
        m_indent.truncate( m_indent.length() - 2 );
    }
    QString exp = QString( "%1:%2 %3 %4%5 " ).arg( "amarok", m_indent, colorize( beginEnd,
        colorStack.top(), colorEnabled ), message, took );
    QTest::ignoreMessage( QtDebugMsg, exp.toLocal8Bit() );
    if( type == Begin )
        m_indent.append( "  " );
    else
        colorStack.pop();
}

QString
TestDebug::colorize( const QString &string, int colorIndex, bool colorEnabled )
{
    static int colors[] = { 1, 2, 4, 5, 6 }; // from Debug.cpp
    if( !colorEnabled )
        return string;
    return QString( "\x1b[00;3%1m%2\x1b[00;39m" ).arg( QString::number(colors[colorIndex]), string );
}


QTEST_GUILESS_MAIN( TestDebug )

#include "TestDebug.moc"
