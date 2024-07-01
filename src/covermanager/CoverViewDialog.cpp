/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "CoverViewDialog.h"

#include "core/meta/Meta.h"
#include "widgets/PixmapViewer.h"

#include <KConfigGroup>
#include <KLocalizedString>

#include <QApplication>
#include <QDialog>
#include <QHBoxLayout>

CoverViewDialog::CoverViewDialog( Meta::AlbumPtr album, QWidget *parent )
    : QDialog( parent )
    , m_title( i18n( "Cover View" ) )
    , m_size( album->image().size() )
    , m_zoom( 100 )
{
    setAttribute( Qt::WA_DeleteOnClose );
    updateCaption();
    createViewer( album->image(), parent );
}

CoverViewDialog::CoverViewDialog( const QImage &image, QWidget *parent )
    : QDialog( parent )
    , m_title( i18n( "Cover View" ) )
    , m_size( image.size() )
    , m_zoom( 100 )
{
    setAttribute( Qt::WA_DeleteOnClose );
    updateCaption();
    createViewer( image, parent );
}

void
CoverViewDialog::updateCaption()
{
    QString width   = QString::number( m_size.width() );
    QString height  = QString::number( m_size.height() );
    QString zoom    = QString::number( m_zoom );
    QString size    = QStringLiteral( "%1x%2" ).arg( width, height );
    QString caption = QStringLiteral( "%1 - %2 - %3%" ).arg( m_title, size, zoom );
    setWindowTitle( caption );
}

void
CoverViewDialog::zoomFactorChanged( qreal value )
{
    m_zoom = 100 * value;
    updateCaption();
}

void
CoverViewDialog::createViewer( const QImage &image, const QWidget *widget )
{
    int screenNumber = QApplication::screens().indexOf( widget->screen() );
    PixmapViewer *pixmapViewer = new PixmapViewer( this, QPixmap::fromImage(image), screenNumber );
    QHBoxLayout *layout = new QHBoxLayout( this );
    layout->addWidget( pixmapViewer );
    layout->setSizeConstraint( QLayout::SetFixedSize );
    layout->setContentsMargins( 0, 0, 0, 0 );
    connect( pixmapViewer, &PixmapViewer::zoomFactorChanged, this, &CoverViewDialog::zoomFactorChanged );

    qreal zoom = pixmapViewer->zoomFactor();
    zoomFactorChanged( zoom );
    QPoint topLeft = mapFromParent( widget->geometry().center() );
    topLeft -= QPoint( image.width() * zoom / 2, image.height() * zoom / 2 );
    move( topLeft );
    activateWindow();
    raise();
}

