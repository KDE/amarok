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

#ifndef TRANSCODING_OPTIONSSTACKEDWIDGET_H
#define TRANSCODING_OPTIONSSTACKEDWIDGET_H

#include "core/transcoding/TranscodingFormat.h"
#include "core/transcoding/TranscodingConfiguration.h"
#include "TranscodingPropertyWidget.h"

#include <QMap>
#include <QStackedWidget>

namespace Transcoding
{

class OptionsStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    explicit OptionsStackedWidget( QWidget *parent = 0 );

    const Configuration configuration( const Configuration::TrackSelection trackSelection ) const;

Q_SIGNALS:
    void formatChanged( Encoder encoder );

public Q_SLOTS:
    void switchPage( Encoder encoder );

private:
    void initWelcomePage();

    /**
     * Initializes a transcoding format configuration page on the stacked widget.
     * @param encoder the encoder enum value.
     * @return the index of the page where the configuration widget was initialized.
     */
    int initCodecPage( Format *format );

    QMap< Encoder, int > m_pagesMap;
    QMap< Encoder, QList< PropertyWidget * > > m_propertyWidgetsMap;
};

} //namespace Transcoding

#endif //TRANSCODING_OPTIONSSTACKEDWIDGET_H
