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

#ifndef TRANSCODEOPTIONSWIDGET_H
#define TRANSCODEOPTIONSWIDGET_H

#include "core/transcoding/TranscodeFormat.h"

#include <QMap>
#include <QStackedWidget>

class TranscodeOptionsWidget : public QStackedWidget
{
    Q_OBJECT
public:
    explicit TranscodeOptionsWidget( QWidget *parent = 0 );

signals:
    void formatChanged( TranscodeFormat::Encoder encoder );

public slots:
    void switchPage( TranscodeFormat::Encoder encoder );

private:
    void initWelcomePage();

    /**
     * Initializes a transcoding format configuration page on the stacked widget.
     * @param encoder the encoder enum value.
     * @return the index of the page where the configuration widget was initialized.
     */
    int initCodecPage( TranscodeFormat::Encoder encoder );

    QMap< TranscodeFormat::Encoder, int > m_pagesMap;
};

#endif // TRANSCODEOPTIONSWIDGET_H
