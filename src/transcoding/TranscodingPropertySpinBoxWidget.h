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

#ifndef TRANSCODING_PROPERTYSPINBOXWIDGET_H
#define TRANSCODING_PROPERTYSPINBOXWIDGET_H

#include "transcoding/TranscodingPropertyWidget.h"

#include <QLabel>
#include <QSpinBox>

namespace Transcoding
{

/**
 * Provides a single QSpinBox-based widget for configuring a given Transcoding::Property.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class PropertySpinBoxWidget : public QWidget, public PropertyWidget
{
    Q_OBJECT
public:
    explicit PropertySpinBoxWidget( Property property, QWidget * parent = 0 );

    QVariant value() const;

    QWidget *widget() { return qobject_cast< QWidget *>( this ); }

private:
    QLabel *m_mainLabel;
    QSpinBox *m_mainEdit;
    Property m_property;
};

} //namespace Transcoding

#endif //TRANSCODING_PROPERTYSPINBOXWIDGET_H
