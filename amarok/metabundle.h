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
#include <taglib/audioproperties.h>


class MetaBundle
{
   public:

  //ordered same way as playlist columns
   MetaBundle( const QString &t1, const QString &t2, const QString &t3, const QString &t4, const QString &t5,
               const QString &t6, const QString &t7, const TagLib::AudioProperties *ap )
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

#endif
