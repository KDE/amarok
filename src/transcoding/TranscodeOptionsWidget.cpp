/****************************************************************************************
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "TranscodeOptionsWidget.h"

#include "core/support/Debug.h"

TranscodeOptionsWidget::TranscodeOptionsWidget( TranscodeFormat::Encoder encoder, QWidget *parent )
    : QWidget(parent)
    , m_encoder( encoder )
{
    DEBUG_BLOCK
    switch( encoder )
    {
    case NULL_CODEC:
        break;
    case AAC:
        initQuality();
        break;
    case ALAC:
        //NO IDEA WHAT TO DO WITH THIS!
        break;
    case FLAC:
        initLevel();
        break;
    case MP3:
        initVRating();
        break;
    case VORBIS:
        initQuality();
        break;
    case WMA2:
        initQuality();
        break;
    default:
        debug() << "Bad encoder.";
    }

}
