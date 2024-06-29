/****************************************************************************************
 * Copyright (c) 2005 Gav Wood <gav@kde.org>                                            *
 * Copyright (c) 2006 Joseph Rabinoff <rabinoff@post.harvard.edu>                       *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#define DEBUG_PREFIX "MoodbarManager"

/*
The mood file loading and rendering code is based on the Amarok 1.4 moodbar implementation
by Gav Wood and Joseph Rabinoff, ported to Qt 4 with only a few modifications by me.

The moodbar generator seems to be running just fine on modern systems if gstreamer is
installed, but it could none the less do with a major update, perhaps to use Phonon or
even porting to qtscript so it could be run, as needed, by Amarok.

- Nikolaj
*/

#include "MoodbarManager.h"

#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"

#include <QFile>
#include <QFileInfo>
#include <QPainter>

#define NUM_HUES 12

namespace The
{
    static MoodbarManager* s_MoodbarManager_instance = nullptr;

    MoodbarManager* moodbarManager()
    {
        if( !s_MoodbarManager_instance )
            s_MoodbarManager_instance = new MoodbarManager();

        return s_MoodbarManager_instance;
    }
}

MoodbarManager::MoodbarManager()
    : m_cache( new KImageCache( QStringLiteral("Amarok-moodbars"), 10 * 1024 ) )
    , m_lastPaintMode( 0 )
{
    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &MoodbarManager::paletteChanged );
}

MoodbarManager::~MoodbarManager()
{}

bool MoodbarManager::hasMoodbar( Meta::TrackPtr track )
{

    //check if we already checked this track:
    if ( m_hasMoodMap.contains( track ) )
    {
        //debug() << "Cached value, returning: " << m_hasMoodMap.value( track );
        return m_hasMoodMap.value( track );
    }
        
 
    QUrl trackUrl = track->playableUrl();
    //only supports local files for now.
    if ( !trackUrl.isLocalFile() )
    {
        debug() << "non local file, no moodbar...";
        m_hasMoodMap.insert( track, false );
        return false;
    }

    //do we already have a moodFile path for this track?
    QString moodFilePath;
    if ( m_moodFileMap.contains( track ) )
        moodFilePath = m_moodFileMap.value( track );
    else
    {
        //Now, lets see if there is a mood file that matches the track filename
        moodFilePath = moodPath( trackUrl.path() );

    }

    debug() << "file path: " << trackUrl.path();
    debug() << "mood file path: " << moodFilePath;

    if( !QFile::exists( moodFilePath ) )
    {
        debug() << "no such file";
        //for fun, try without the leading '.'

        QFileInfo fInfo( moodFilePath );
        QString testName = fInfo.fileName(); 
        testName.remove( 0, 1 );

        moodFilePath.replace( fInfo.fileName(), testName );

        debug() << "trying : " << moodFilePath;
        if( !QFile::exists( moodFilePath ) )
        {
            debug() << "no luck removing the leading '.' either...";
            m_hasMoodMap.insert( track, false );
            return false;
        }

        debug() << "whoops, missing leading '.', so mood file path: " << moodFilePath;
    }

    //it is a local file with a matching .mood file. Good enough for now!
    
    m_moodFileMap.insert( track, moodFilePath );
    m_hasMoodMap.insert( track, true );
    
    return true;
}

QPixmap MoodbarManager::getMoodbar( Meta::TrackPtr track, int width, int height, bool rtl )
{
    //if we have already marked this track as
    //not having a moodbar, don't even bother...
    if ( m_hasMoodMap.contains( track ) )
        if( !m_hasMoodMap.value( track ) )
            return QPixmap();
        

    //first of all... Check if rendering settings have changed. If
    //so, clear data and pixmap caches.

    if( m_lastPaintMode != AmarokConfig::moodbarPaintStyle() )
    {
        m_lastPaintMode = AmarokConfig::moodbarPaintStyle();
        m_cache->clear();
        m_moodDataMap.clear();
        Q_EMIT moodbarStyleChanged();
    }


    //Do we already have this pixmap cached?
    const QString pixmapKey = QStringLiteral( "mood:%1-%2x%3%4" ).arg( track->uidUrl(), QString::number( width ),
                                                                QString::number( height ), QString( rtl?QStringLiteral("r"):QStringLiteral("") ) );
    QPixmap moodbar;

    if( m_cache->findPixmap( pixmapKey, &moodbar ) )
        return moodbar;
        
    //No? Ok, then create it reusing as much info as possible

    MoodbarColorList data;

    if ( m_moodDataMap.contains( track ) )
        data = m_moodDataMap.value( track );
    else
    {

        QString moodFilePath;
        if ( m_moodFileMap.contains( track ) )
            moodFilePath = m_moodFileMap.value( track );
        else
            moodFilePath = moodPath( track->playableUrl().path() );

        data = readMoodFile( QUrl::fromUserInput(moodFilePath) );

        if ( data.size() > 10 )
            m_moodDataMap.insert( track, data );
        else
        {
            //likely a corrupt file, so mark this track as not having a moodbar
             m_hasMoodMap.insert( track, false );
        }
    }

    //assume that the readMoodFile function emits the proper error...
    if ( data.size() < 10 )
        return moodbar;

    moodbar = drawMoodbar( data, width, height, rtl );
    m_cache->insertPixmap( pixmapKey, moodbar );
    
    return moodbar;
}

MoodbarColorList MoodbarManager::readMoodFile( const QUrl &moodFileUrl )
{
    DEBUG_BLOCK

    MoodbarColorList data;

    const QString path = moodFileUrl.path();
    if( path.isEmpty() )
        return data;

    debug() << "Trying to read " << path;

    QFile moodFile( path );

    if( !moodFile.open( QIODevice::ReadOnly ) )
        return data;

    int r, g, b, samples = moodFile.size() / 3;
    debug() << "File" << path << "opened. Proceeding to read contents... s=" << samples;

    // This would be bad.
    if( samples == 0 )
    {
        debug() << "Filex " << moodFile.fileName() << "is corrupted, removing";
        //TODO: notify the user somehow
        //moodFile.remove();
        return data;
    }

    int huedist[360];         // For alterMood
    int modalHue[NUM_HUES];   // For m_hueSort
    int h, s, v;

    memset( modalHue, 0, sizeof( modalHue ) );
    memset( huedist, 0, sizeof( huedist ) );

    // Read the file, keeping track of some histograms
    for( int i = 0; i < samples; ++i )
    {

        char rChar, gChar, bChar;
        moodFile.getChar( &rChar );
        moodFile.getChar( &gChar );
        moodFile.getChar( &bChar );

        r = qAbs( (int) rChar );
        g = qAbs( (int) gChar );
        b = qAbs( (int) bChar );
  
        data.append( QColor( qBound( 0, r, 255 ),
                             qBound( 0, g, 255 ),
                             qBound( 0, b, 255 ) ) );

        // Make a histogram of hues
        data.last().getHsv( &h, &s, &v );
        modalHue[qBound( 0, h * NUM_HUES / 360, NUM_HUES - 1 )] += v;

        if( h < 0 ) h = 0;  else h = h % 360;
        huedist[h]++;
    }

    // Make moodier -- copied straight from Gav Wood's code
    // Here's an explanation of the algorithm:
    //
    // The "input" hue for each bar is mapped to a hue between
    // rangeStart and (rangeStart + rangeDelta).  The mapping is
    // determined by the hue histogram, huedist[], which is calculated
    // above by putting each sample into one of 360 hue bins.  The
    // mapping is such that if your histogram is concentrated on a few
    // hues that are close together, then these hues are separated,
    // and the space between spikes in the hue histogram is
    // compressed.  Here we consider a hue value to be a "spike" in
    // the hue histogram if the number of samples in that bin is
    // greater than the threshold variable.
    //
    // As an example, suppose we have 100 samples, and that
    //    threshold = 10  rangeStart = 0  rangeDelta = 288
    // Suppose that we have 10 samples at each of 99,100,101, and 200.
    // Suppose that there are 20 samples < 99, 20 between 102 and 199,
    // and 20 above 201, with no spikes.  There will be five hues in
    // the output, at hues 0, 72, 144, 216, and 288, containing the
    // following number of samples:
    //     0:   20 + 10 = 30   (range 0   - 99 )
    //     72:            10   (range 100 - 100)
    //     144:           10   (range 101 - 101)
    //     216: 10 + 20 = 30   (range 102 - 200)
    //     288:           20   (range 201 - 359)
    // The hues are now much more evenly distributed.
    //
    // After the hue redistribution is calculated, the saturation and
    // value are scaled by sat and val, respectively, which are percentage
    // values.
    moodFile.close();

    const int paintStyle = AmarokConfig::moodbarPaintStyle();

    {
        MoodbarColorList modifiedData;
        // Explanation of the parameters:
        //
        //   threshold: A hue value is considered to be a "spike" in the
        //     histogram if it's above this value.  Setting this value
        //     higher will tend to make the hue distribution more uniform
        //
        //   rangeStart, rangeDelta: output hues will be more or less
        //     evenly spaced between rangeStart and (rangeStart + rangeDelta)
        //
        //   sat, val: the saturation and value are scaled by these integral
        //     percentage values

        int threshold, rangeStart, rangeDelta, sat, val;
        int total = 0;
        memset( modalHue, 0, sizeof( modalHue ) );  // Recalculate this

        switch( paintStyle )
        {
        case Angry: // Angry
            threshold  = samples / 360 * 9;
            rangeStart = 45;
            rangeDelta = -45;
            sat        = 200;
            val        = 100;
            break;

        case Frozen: // Frozen
            threshold  = samples / 360 * 1;
            rangeStart = 140;
            rangeDelta = 160;
            sat        = 50;
            val        = 100;
            break;

        case Happy: // Happy
            threshold  = samples / 360 * 2;
            rangeStart = 0;
            rangeDelta = 359;
            sat        = 150;
            val        = 250;
            break;

        case Normal: // old "normal" mode, don't change moodfile's RGB values
            threshold  = samples / 360 * 3;
            rangeStart = 0;
            rangeDelta = 359;
            sat        = 100;
            val        = 100;
            break;

        case SystemColours:
        default: // Default (system colours)
            threshold  = samples / 360 * 3;
            rangeStart = The::paletteHandler()->highlightColor().hsvHue();
            rangeStart = (rangeStart - 20 + 360) % 360;
            rangeDelta = 20;
            sat        = The::paletteHandler()->highlightColor().hsvSaturation();
            val        = The::paletteHandler()->highlightColor().value() / 2;
        }

        //debug() << "ReadMood: Applying filter t=" << threshold
        //        << ", rS=" << rangeStart << ", rD=" << rangeDelta
        //        << ", s=" << sat << "%, v=" << val << "%" << Qt::endl;

        // On average, huedist[i] = samples / 360.  This counts the
        // number of samples over the threshold, which is usually
        // 1, 2, 9, etc. times the average samples in each bin.
        // The total determines how many output hues there are,
        // evenly spaced between rangeStart and rangeStart + rangeDelta.
        for( int i = 0; i < 360; i++ )
            if( huedist[i] > threshold )
                total++;

        if( total < 360 && total > 0 )
        {
            // Remap the hue values to be between rangeStart and
            // rangeStart + rangeDelta.  Every time we see an input hue
            // above the threshold, increment the output hue by
            // (1/total) * rangeDelta.
            for( int i = 0, n = 0; i < 360; i++ )
                huedist[i] = ( ( huedist[i] > threshold ? n++ : n )
                               * rangeDelta / total + rangeStart ) % 360;

            // Now huedist is a hue mapper: huedist[h] is the new hue value
            // for a bar with hue h
            for( QColor color : data )
            {
                color.getHsv( &h, &s, &v );
                h = h < 0 ? 0 : h % 360;

                color.setHsv( qBound( 0, huedist[h], 359 ),
                              qBound( 0, s * sat / 100, 255 ),
                              qBound( 0, v * val / 100, 255 ) );

                modalHue[qBound( 0, huedist[h] * NUM_HUES / 360, NUM_HUES - 1 )] += ( v * val / 100 );
                modifiedData.append( color );
            }
        }
        return modifiedData;
    }

    // Calculate m_hueSort.  This is a 3-digit number in base NUM_HUES,
    // where the most significant digit is the first strongest hue, the
    // second digit is the second strongest hue, and the third digit
    // is the third strongest.  This code was written by Gav Wood.

    /*
    m_hueSort = 0;
    mx = 0;
    for( int i = 1; i < NUM_HUES; i++ )
      if( modalHue[i] > modalHue[mx] )
        mx = i;
    m_hueSort = mx * NUM_HUES * NUM_HUES;
    modalHue[mx] = 0;

    mx = 0;
    for( int i = 1; i < NUM_HUES; i++ )
      if( modalHue[i] > modalHue[mx] )
        mx = i;
    m_hueSort += mx * NUM_HUES;
    modalHue[mx] = 0;

    mx = 0;
    for( int i = 1; i < NUM_HUES; i++ )
      if( modalHue[i] > modalHue[mx] )
        mx = i;
    m_hueSort += mx;
*/
    //debug() << "All done.";
    return data;
}

QPixmap MoodbarManager::drawMoodbar( const MoodbarColorList &data, int width, int height, bool rtl )
{

    // First average the moodbar samples that will go into each
    // vertical bar on the screen.

    if( data.isEmpty() ) // Play it safe -- see below
      return QPixmap();

    MoodbarColorList screenColors;
    QColor bar;
    float r, g, b;
    int h, s, v;

    for( int i = 0; i < width; i++ )
    {
        r = 0.f;  g = 0.f;  b = 0.f;

        // data.size() needs to be at least 1 for this not to crash!
        uint start = i * data.size() / width;
        uint end   = (i + 1) * data.size() / width;

        if( start == end )
            end = start + 1;

        for( uint j = start; j < end; j++ )
        {
            r += data[j].red();
            g += data[j].green();
            b += data[j].blue();
        }

        uint n = end - start;
        bar =  QColor( int( r / float( n ) ),
                       int( g / float( n ) ),
                       int( b / float( n ) ) );

        // Snap to the HSV values for later
        bar.getHsv(&h, &s, &v);
        bar.setHsv(h, s, v);

        screenColors.append( bar );
    }

    // Paint the bars.  This is Gav's painting code -- it breaks up the
    // monotony of solid-color vertical bars by playing with the saturation
    // and value.

    QPixmap pixmap = QPixmap( width, height );
    QPainter paint( &pixmap );
    
    for( int x = 0; x < width; x++ )
    {
        screenColors[x].getHsv( &h, &s, &v );

        for( int y = 0; y <= height / 2; y++ )
        {
            float coeff = float( y ) / float( height / 2 );
            float coeff2 = 1.f - ( ( 1.f - coeff ) * ( 1.f - coeff ) );
            coeff = 1.f - ( 1.f - coeff ) / 2.f;
            coeff2 = 1.f - ( 1.f - coeff2 ) / 2.f;

            QColor hsvColor;
            hsvColor.setHsv( h,
            qBound( 0, int( float( s ) * coeff ), 255 ),
            qBound( 0, int( 255.f - (255.f - float( v ) ) * coeff2 ), 255 ) ) ;
            paint.setPen( hsvColor );
            paint.drawPoint( x, y );
            paint.drawPoint( x, height - 1 - y );
        }
    }
    paint.end();

    if ( rtl )
        pixmap = QPixmap::fromImage( pixmap.toImage().mirrored( true, false ) );

    return pixmap;
}

QString MoodbarManager::moodPath( const QString &trackPath ) const
{
    QStringList parts = trackPath.split( QLatin1Char('.') );
    parts.takeLast();
    parts.append( QStringLiteral("mood") );
    QString moodPath = parts.join( QStringLiteral(".") );
    
    //now prepend the filename with .
    const QFileInfo fileInfo( moodPath );
    const QString fileName = fileInfo.fileName();

    return moodPath.replace( fileName, QLatin1Char('.') + fileName );
}

void MoodbarManager::paletteChanged( const QPalette &palette )
{
    Q_UNUSED( palette )
    const int paintStyle = AmarokConfig::moodbarPaintStyle();
    if( paintStyle == 0 ) // system default colour
    {
        m_cache->clear();
        m_moodDataMap.clear();
    }
}
