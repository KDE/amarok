/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_COVERVIEWDIALOG_H
#define AMAROK_COVERVIEWDIALOG_H

#include "meta/Meta.h"
#include "widgets/PixmapViewer.h"

#include <KApplication>
#include <KDialog> //baseclass
#include <KLocale>
#include <KWindowSystem>

#include <QHBoxLayout>
#include <QDesktopWidget>

class AMAROK_EXPORT CoverViewDialog : public QDialog
{
    public:
        CoverViewDialog( Meta::AlbumPtr album, QWidget *parent )
            : QDialog( parent )
        {
            setAttribute( Qt::WA_DeleteOnClose );

            #ifdef Q_WS_X11
            KWindowSystem::setType( winId(), NET::Utility );
            #endif

            kapp->setTopWidget( this );
            setWindowTitle( KDialog::makeStandardCaption( i18n("%1 - %2",
                            album->albumArtist()? album->albumArtist()->prettyName() : i18n( "Various Artists" ),
                            album->prettyName() ) ) );

            int screenNumber = KApplication::desktop()->screenNumber( parent );

            PixmapViewer *pixmapViewer = new PixmapViewer( this, album->image( 0 ) /* full sized image */, screenNumber );
            QHBoxLayout *layout = new QHBoxLayout( this );
            layout->addWidget( pixmapViewer );
            layout->setSizeConstraint( QLayout::SetFixedSize );
        }

        CoverViewDialog( QPixmap pixmap, QWidget *parent )
            : QDialog( parent )
        {
            setAttribute( Qt::WA_DeleteOnClose );

            #ifdef Q_WS_X11
            KWindowSystem::setType( winId(), NET::Utility );
            #endif

            kapp->setTopWidget( this );
            setWindowTitle( KDialog::makeStandardCaption( i18n( "Cover View" ) ) );

            int screenNumber = KApplication::desktop()->screenNumber( parent );

            PixmapViewer *pixmapViewer = new PixmapViewer( this, pixmap /* full sized image */, screenNumber );
            QHBoxLayout *layout = new QHBoxLayout( this );
            layout->addWidget( pixmapViewer );
            layout->setSizeConstraint( QLayout::SetFixedSize );
        }
};

#endif
