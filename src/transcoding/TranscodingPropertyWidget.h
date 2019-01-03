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

#ifndef TRANSCODING_PROPERTYWIDGET_H
#define TRANSCODING_PROPERTYWIDGET_H

#include "core/transcoding/TranscodingProperty.h"

#include <QWidget>

namespace Transcoding
{

/**
 * Provides the interface for a generic configuration editing widget for a given
 * Transcoding::Property.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class PropertyWidget
{
public:
    static PropertyWidget * create( Property &property, QWidget * parent = nullptr );

    virtual ~PropertyWidget(){}

    virtual QByteArray name() const { return m_name; }

    virtual QVariant value() const = 0;

    virtual QWidget *widget() = 0;

protected:
    QByteArray m_name;
};

} //namespace Transcoding

#endif //TRANSCODING_PROPERTYWIDGET_H
