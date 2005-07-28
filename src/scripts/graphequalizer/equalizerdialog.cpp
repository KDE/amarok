/***************************************************************************
 *   Copyright (C) 2005 by Ian Monroe   *
 *   ian@monroe.nu   *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "equalizerdialog.h"
#include "equalizercanvasview.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qslider.h>

EqualizerDialog::EqualizerDialog()
{
    QBoxLayout* overallLayout = new QHBoxLayout(this, 5, -1, "overallLayout");
    QBoxLayout* sliderLayout  = new QVBoxLayout(this, 5, -1, "sliderLayout");

    QSlider*    preampSlider  = new QSlider(-100,100,1,0,QSlider::Vertical,this, "preampSlider");
    QLabel*     preampLabel   = new QLabel("Pre-amp",this,"preampLabel");

    sliderLayout->addWidget(preampSlider);
    sliderLayout->addWidget(preampLabel);

    QCanvas* canvas = new QCanvas();
    canvas->resize(400, 200);

    EqualizerCanvasView* canvasView = new EqualizerCanvasView(canvas, this, "canvasView");

    overallLayout->addLayout(sliderLayout);
    overallLayout->addWidget(canvasView);
}


EqualizerDialog::~EqualizerDialog()
{
}


#include "equalizerdialog.moc"
