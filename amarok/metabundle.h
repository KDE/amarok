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

#include "playlistitem.h" //FIXME dependency

#include <qstring.h>
#include <kurl.h>
#include <klocale.h>

//taglib includes
#include <tag.h>
#include <tstring.h>
#include <audioproperties.h>


/*
 * This class is not very complete, it fits our needs as they stand currently
 * If it doesn't work for you in some way, extend it sensibly :)
 */

//TODO cache prettyLength for bundles
//TODO is it feasible to just use a TagLib::AudioProperties structure?


class MetaBundle
{
public:

    //for the audioproperties
    static const int Undetermined = -2;
    static const int Irrelevant   = -1;
    static const int Unavailable  =  0;

    MetaBundle()
    {}
    
    MetaBundle( const QString &title, const QString &genre, int bitrate = -2 )
      : m_title     ( title )
      , m_genre     ( genre )
      , m_bitrate   ( bitrate )
      , m_length    ( -2 )
      , m_sampleRate( -2 )
    {}

    //TODO one without audioProps please
    //And have ability to determine bitrate etc from the strings, slow but infrequently called so ok
    MetaBundle( const PlaylistItem *item, TagLib::AudioProperties *ap )
      : m_url(     item->url() )
      , m_title(   item->title() ) //because you override text()
      , m_artist(  item->text( 2 ) )
      , m_album(   item->text( 3 ) )
      , m_year(    item->text( 4 ) )
      , m_comment( item->text( 5 ) )
      , m_genre(   item->text( 6 ) )
      , m_track(   item->text( 7 ) )
      , m_bitrate( -2 )
      , m_length( -2 )
      , m_sampleRate( -2 )
    {
        init( ap );
    }

    MetaBundle( const KURL &url, TagLib::Tag *tag, TagLib::AudioProperties *ap = 0 )
      : m_url( url )
      , m_title(   TStringToQString( tag->title() ).stripWhiteSpace() )
      , m_artist(  TStringToQString( tag->artist() ).stripWhiteSpace() )
      , m_album(   TStringToQString( tag->album() ).stripWhiteSpace() )
      , m_year(    tag->year() ? QString::number( tag->year() ) : QString() )
      , m_comment( TStringToQString( tag->comment() ).stripWhiteSpace() )
      , m_genre(   TStringToQString( tag->genre() ).stripWhiteSpace() )
      , m_track(   tag->track() ? QString::number( tag->track() ) : QString() )
      , m_bitrate( -2 )

      , m_length( -2 )
      , m_sampleRate( -2 )

    {
        init( ap );
    }

// ATTRIBUTES ------
    int length()     const { return m_length > 0 ? m_length : 0; }
    int bitrate()    const { return m_bitrate; }
    int sampleRate() const { return m_sampleRate; }

    QString prettyTitle() const;
    QString prettyURL()     const { return m_url.prettyURL(); }
    QString prettyBitrate() const { return prettyBitrate( m_bitrate ); }
    QString prettyLength()  const { return prettyLength( m_length ); }
    QString prettySampleRate() const { return prettyGeneric( i18n( "SampleRate", "%1 Hz" ), m_sampleRate ); }

    static QString prettyBitrate( int i ) { return prettyGeneric( i18n( "Bitrate", "%1 kpbs" ), i ); }
    static QString prettyLength( int );

    const KURL    m_url;
    const QString m_title;
    const QString m_artist;
    const QString m_album;
    const QString m_year;
    const QString m_comment;
    const QString m_genre;
    const QString m_track;

private:
    int m_bitrate;
    int m_length;
    int m_sampleRate;

    static QString prettyGeneric( const QString&, int );

    void init( TagLib::AudioProperties *ap )
    {
        if( ap )
        {
            m_bitrate    = ap->bitrate();
            m_length     = ap->length();
            m_sampleRate = ap->sampleRate();
        }
    }
};


inline QString
MetaBundle::prettyTitle() const
{
    QString s = m_artist;
    if( !s.isEmpty() ) s += " - ";
    s += m_title;

    return s.isEmpty() ? m_url.fileName().section( '.', 0, 0 ) : s;
}

inline QString
MetaBundle::prettyLength( int length ) //static
{
    //TODO don't inline! (code bloat)

    QString s;

    if( length > 0 )
    {
        //we don't do hours, people aren't interested in them
        int min = length / 60;
        int sec = length % 60;

        //don't zeroPad the minutes
        s.setNum( min ).append( ':' );
        if( sec < 10 ) s += '0';
        s += QString::number( sec );
    }
    else if( length ==  0 ) s = '?';
    else if( length == -1 ) s = '-';

    return s;
}

inline QString
MetaBundle::prettyGeneric( const QString &s, int i )
{
    return ( i > 0 ) ? s.arg( i ) : ( i == -2 ) ? QString() : "?";
}

#endif
