/* PhotoFlow - animated image viewer for mobile devices
 *
 * Copyright (C) 2008 Ariya Hidayat (ariya.hidayat@gmail.com)
 * Copyright (C) 2007 Ariya Hidayat (ariya.hidayat@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA
 *
 */

#include "PhotoBrowser.h"

#include "ImageLoader.h"
#include "pictureflow.h"

#include <QDir>
#include <QImage>
#include <QImageReader>
#include <QTimer>
#include "meta/Meta.h"

#define ImageLoader PlainImageLoader

class PhotoBrowser::Private
{
public:
    QString imagePath;
    QTimer* updateTimer;
    ImageLoader* worker;
};

PhotoBrowser::PhotoBrowser( QWidget* parent ): PictureFlow( parent )
{
    d = new Private;

    d->updateTimer = new QTimer;
    connect( d->updateTimer, SIGNAL( timeout() ), this, SLOT( updateImageData() ) );
    d->worker = new ImageLoader;
    connect( this, SIGNAL( centerIndexChanged( int ) ), this, SLOT( preload() ) );
}

PhotoBrowser::~PhotoBrowser()
{
    delete d->worker;
    delete d->updateTimer;
    delete d;
}
void PhotoBrowser::fillAlbums( Meta::AlbumList albums )
{
    foreach( Meta::AlbumPtr album, albums )
    {
        addAlbum( album );
        addSlide( QImage() );
    }
    setCenterIndex( 0 );
    preload();
}
void PhotoBrowser::preload()
{
    d->updateTimer->start( 100 );
}

void PhotoBrowser::updateImageData()
{
    // can't do anything, wait for the next possibility
    if ( d->worker->busy() )
        return;

    // set image of last one
    if ( d->worker->index() >= 0 )
        setSlide( d->worker->index(), d->worker->result() );

    // try to load only few images on the left and right side
    // i.e. all visible ones plus some extra
#define COUNT 10
    int indexes[2*COUNT+1];
    int center = centerIndex();
    indexes[0] = center;
    for ( int j = 0; j < COUNT; j++ )
    {
        indexes[j*2+1] = center + j + 1;
        indexes[j*2+2] = center - j - 1;
    }

    for ( int c = 0; c < 2*COUNT + 1; c++ )
    {
        int i = indexes[c];
        if (( i >= 0 ) && ( i < slideCount() ) )
            if ( slide( i ).isNull() )
            {
                // schedule thumbnail generation
                Meta::AlbumPtr iAlbum = m_album_list[i];
                d->worker->generate( i, iAlbum, slideSize() );
                return;
            }
    }

    // no need to generate anything? stop polling...
    d->updateTimer->stop();
}
void PhotoBrowser::fastForward()
{
    int nbslides = slideCount();
    int current = centerIndex();
    showSlide( current + nbslides / 10 );
}
void PhotoBrowser::fastBackward()
{
    int nbslides = slideCount();
    int current = centerIndex();
    showSlide( current - nbslides / 10 );
}
void PhotoBrowser::skipToSlide( int iSlide )
{
    setCenterIndex( iSlide );
    preload();
}
