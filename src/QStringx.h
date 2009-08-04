/****************************************************************************************
 * Copyright (c) 2004 Shintaro Matsuoka <shin@shoegazed.org>                            *
 * Copyright (c) 2006 Martin Aumueller <aumuell@reserv.at>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef AMAROK_QSTRINGX_H
#define AMAROK_QSTRINGX_H

#include <qglobal.h>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <qmap.h>

namespace Amarok
{

class QStringx : public QString
{
public:
    QStringx() {}
    QStringx( QChar ch ) : QString( ch ) {}
    QStringx( const QString& s ) : QString( s ) {}
    QStringx( const QByteArray& ba ) : QString( ba ) {}
    QStringx( const QChar* unicode, uint length ) : QString( unicode, length ) {}
    QStringx( const char* str ) : QString( str ) {}
    virtual ~QStringx() {}

    // the numbers following % obviously are not taken into account
    QString args( const QStringList& args ) const
    {
        const QStringList text = (*this).split( QRegExp( "%\\d+" ), QString::KeepEmptyParts );

        QList<QString>::ConstIterator itrText = text.constBegin();
        QList<QString>::ConstIterator itrArgs = args.constBegin();
        QList<QString>::ConstIterator endText = text.constEnd();
        QList<QString>::ConstIterator endArgs = args.constEnd();
        QString merged = (*itrText);
        ++itrText;
        while( itrText != endText && itrArgs != endArgs )
        {
            merged += (*itrArgs) + (*itrText);
            ++itrText;
            ++itrArgs;
        }

        Q_ASSERT( itrText == text.end() || itrArgs == args.end() );

        return merged;
    }

    // %something gets replaced by the value corresponding to key "something" in args
    QString namedArgs( const QMap<QString, QString> &args, bool opt=false ) const
    {
        QRegExp rxArg( "%[a-zA-Z0-9]+" );

        QString result;
        int start = 0;
        for( int pos = rxArg.indexIn( *this );
                pos != -1;
                pos = rxArg.indexIn( *this, start ) )
        {
            int len = rxArg.matchedLength();
            QString p = rxArg.capturedTexts()[0].mid(1, len-1);

            result += mid( start, pos-start );
            if( !args[p].isEmpty() )
                result += args[p];
            else if( opt )
                return QString();

            start = pos + len;
        }
        result += mid( start );

        return result;
    }

    // %something gets replaced by the value corresponding to key "something" in args,
    // however, if key "something" is not available,
    // then replace everything within surrounding { } by an empty string
    QString namedOptArgs( const QMap<QString, QString> &args ) const
    {
        QRegExp rxOptArg( "\\{.*%[a-zA-Z0-9_]+.*\\}" );
        rxOptArg.setMinimal( true );

        QString result;
        int start = 0;
        for( int pos = rxOptArg.indexIn( *this );
                pos != -1;
                pos = rxOptArg.indexIn( *this, start ) )
        {
            int len = rxOptArg.matchedLength();
            QStringx opt = rxOptArg.capturedTexts()[0].mid(1, len-2);

            result += QStringx(mid( start, pos-start )).namedArgs( args );
            result += opt.namedArgs( args, true );

            start = pos + len;
        }
        result += QStringx( mid( start ) ).namedArgs( args );

        return result;
    }
};

} // namespace Amarok

#endif // AMAROK_QSTRINGX_H
