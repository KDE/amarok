// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
// See COPYING file for licensing information

#include <qglobal.h>
#include <qregexp.h>
#include <qstring.h>
#include <qstringlist.h>

#ifndef AMAROK_QSTRINGX_H
#define AMAROK_QSTRINGX_H

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
  ~QStringx() {};

  QString args( const QStringList& l );
};

QString QStringx::args( const QStringList& l )
{
  QStringList k = QStringList::split( QRegExp( "%\\d+" ), *this, TRUE );
  Q_ASSERT( k.count()-1 == l.count() );
  QString r;
  for ( unsigned int i=0 ; i < k.count()-1 ; ++i )
    r += k[i] + l[i];
  r += k[k.count()-1];
  return r;
}

} // namespace amaroK


#endif // AMAROK_QSTRINGX_H
