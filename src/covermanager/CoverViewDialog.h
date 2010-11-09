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

#include "core/meta/Meta.h"

#include <QDialog>

class AMAROK_EXPORT CoverViewDialog : public QDialog
{
    Q_OBJECT

    public:
        CoverViewDialog( Meta::AlbumPtr album, QWidget *parent );
        CoverViewDialog( const QPixmap &pixmap, QWidget *parent );

    private slots:
        void updateCaption();
        void zoomFactorChanged( float value );

    private:
        void init();
        void createViewer( const QPixmap &pixmap, const QWidget *widget );

        QString m_title;
        QSize m_size;
        int m_zoom;
};

#endif
