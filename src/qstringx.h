// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
// See COPYING file for licensing information

#ifndef AMAROK_QSTRINGX_H
#define AMAROK_QSTRINGX_H

#include <qglobal.h>
#include <qregexp.h>
#include <qstring.h>
#include <qstringlist.h>

namespace amaroK
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
    
    QString args( const QStringList& args ) const
    {
        const QStringList text = QStringList::split( QRegExp( "%\\d+" ), *this, TRUE );
        
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
};

} // namespace amaroK

#endif // AMAROK_QSTRINGX_H
