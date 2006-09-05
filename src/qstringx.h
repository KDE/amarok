// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
// Copyright (C) 2006 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#ifndef AMAROK_QSTRINGX_H
#define AMAROK_QSTRINGX_H

#include <qglobal.h>
#include <qregexp.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>

namespace Amarok
{

class QStringx : public QString
{
public:
    QStringx() {};
    QStringx( QChar ch ) : QString( ch ) {};
    QStringx( const QString& s ) : QString( s ) {};
    QStringx( const QByteArray& ba ) : QString( ba ) {};
    QStringx( const QChar* unicode, uint length ) : QString( unicode, length ) {};
    QStringx( const char* str ) : QString( str ) {};
    virtual ~QStringx() {};

    // the numbers following % obviously are not taken into account
    QString args( const QStringList& args ) const
    {
        const QStringList text = QStringList::split( QRegExp( "%\\d+" ), *this, true );

        QValueListConstIterator<QString> itrText = text.begin();
        QValueListConstIterator<QString> itrArgs = args.begin();
        QString merged = (*itrText);
        ++itrText;
        while ( itrText != text.end() && itrArgs != args.end() )
        {
            merged += (*itrArgs) + (*itrText);
            ++itrText;
            ++itrArgs;
        }

        Q_ASSERT( itrText == text.end() && itrArgs == args.end() );

        return merged;
    }

    // %something gets replaced by the value corresponding to key "something" in args
    QString namedArgs( const QMap<QString, QString> args, bool opt=false ) const
    {
        QRegExp rxArg( "%[a-zA-Z0-9]+" );

        QString result;
        int start = 0;
        for( int pos = rxArg.search( *this );
                pos != -1;
                pos = rxArg.search( *this, start ) )
        {
            int len = rxArg.matchedLength();
            QString p = rxArg.capturedTexts()[0].mid(1, len-1);

            result += mid( start, pos-start );
            if( args[p] != QString::null )
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
    QString namedOptArgs( const QMap<QString, QString> args ) const
    {
        QRegExp rxOptArg( "\\{.*%[a-zA-Z0-9_]+.*\\}" );
        rxOptArg.setMinimal( true );

        QString result;
        int start = 0;
        for( int pos = rxOptArg.search( *this );
                pos != -1;
                pos = rxOptArg.search( *this, start ) )
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
