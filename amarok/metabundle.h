/***************************************************************************
                          metabundle.h  -  description
                             -------------------
    copyright            : (C) 2003 by Max Howell
    email                : max.howell@methylblue.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef METABUNDLE_H
#define METABUNDLE_H

#include <qstring.h>
#include <klocale.h>
#include <taglib/audioproperties.h>


class MetaBundle
{
   public:

  //ordered same way as playlist columns
   MetaBundle( const QString &t1, const QString &t2, const QString &t3, const QString &t4,
               const QString &t5, const QString &t6, const QString &t7,
               const TagLib::AudioProperties *ap )
     : m_title( t1 )
     , m_artist( t2 )
     , m_album( t3 )
     , m_year( t4 )
     , m_comment( t5 )
     , m_genre( t6 )
     , m_track( t7 )
     , m_bitrate( 0 ), m_length( 0 ), m_sampleRate( 0 )
   {
      if( ap )
      {
         m_bitrate    = ap->bitrate();
         m_length     = ap->length();
         m_sampleRate = ap->sampleRate();
      }
   }
   MetaBundle( const QString &title, uint length ) : m_title( title ), m_bitrate( 0 ), m_length( length ), m_sampleRate( 0 ) {}
   MetaBundle() : m_bitrate( 0 ), m_length( 0 ), m_sampleRate( 0 ) {}

   QString prettyLength() const;
   QString prettyBitRate() const;

//private:
   const QString m_title;
   const QString m_artist;
   const QString m_album;
   const QString m_year;
   const QString m_comment;
   const QString m_genre;
   const QString m_track;

   uint m_bitrate;
   int  m_length; //-1 no established length, eg streams
   uint m_sampleRate;
};


inline QString
MetaBundle::prettyLength() const
{
    //TODO don't inline! (code bloat)

    QString s;

    if( m_length == -1 ) s = "-";
    else if( m_length > 0 )
    {
        //we don't do hours, people aren't interested in them
        int min = m_length / 60 % 60;
        int sec = m_length % 60;

        //don't zeroPad the minutes
        s.setNum( min ).append( ':' );
        if( sec < 10 ) s += '0';
        s += QString::number( sec );
    }

    return s;
}

inline QString
MetaBundle::prettyBitRate() const
{
    return ( m_bitrate == 0 ) ? QString() : i18n( "Bitrate = %1", "%1 kbps" ).arg( m_bitrate );
}

#endif
