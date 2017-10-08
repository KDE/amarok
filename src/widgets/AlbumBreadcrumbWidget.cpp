/****************************************************************************************
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

#include "AlbumBreadcrumbWidget.h"

#include "core/meta/Meta.h"
#include "widgets/BreadcrumbItemButton.h"

#include <QIcon>
#include <KLocale>

AlbumBreadcrumbWidget::AlbumBreadcrumbWidget( const Meta::AlbumPtr album, QWidget *parent )
    : KHBox( parent )
    , m_album( album )
{
    const QIcon artistIcon = QIcon::fromTheme( "filename-artist-amarok" );
    const QIcon albumIcon = QIcon::fromTheme( "filename-album-amarok" );
    new BreadcrumbItemMenuButton( this );
    m_artistButton = new BreadcrumbItemButton( artistIcon, QString(), this );
    new BreadcrumbItemMenuButton( this );
    m_albumButton = new BreadcrumbItemButton( albumIcon, QString(), this );

    QWidget *spacer = new QWidget( this );

    setStretchFactor( m_artistButton, 1 );
    setStretchFactor( m_albumButton, 1 );
    setStretchFactor( spacer, 1 );

    connect( m_artistButton, &BreadcrumbItemButton::clicked, this, &AlbumBreadcrumbWidget::slotArtistClicked );
    connect( m_albumButton, &BreadcrumbItemButton::clicked, this, &AlbumBreadcrumbWidget::slotAlbumClicked );

    updateBreadcrumbs();
}

AlbumBreadcrumbWidget::~AlbumBreadcrumbWidget()
{
}

void AlbumBreadcrumbWidget::setAlbum( const Meta::AlbumPtr album )
{
    m_album = album;
    updateBreadcrumbs();
}

void AlbumBreadcrumbWidget::updateBreadcrumbs()
{
    const QString &album  = m_album->prettyName();
    const QString &artist = m_album->hasAlbumArtist() ? m_album->albumArtist()->prettyName()
                                                      : i18n( "Various Artists" );
    m_artistButton->setText( artist );
    m_albumButton->setText( album );
}

void AlbumBreadcrumbWidget::slotArtistClicked()
{
    if( m_album->hasAlbumArtist() )
        emit artistClicked( m_album->albumArtist()->name() );
}

void AlbumBreadcrumbWidget::slotAlbumClicked()
{
    emit albumClicked( m_album->name() );
}

