/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef WATERMARK_WIDGET_H
#define WATERMARK_WIDGET_H

#include "UnicornDllExportMacro.h"

#include <QWidget>
#include <QPixmap>

class UNICORN_DLLEXPORT WatermarkWidget : public QWidget
{
    public:
        WatermarkWidget( QWidget* parent = 0 ) : QWidget( parent ) {}
        void setWatermark( QString fileName );

    protected:
        virtual void paintEvent( QPaintEvent* event );

    private:
        QPixmap m_watermark;
};

#endif
