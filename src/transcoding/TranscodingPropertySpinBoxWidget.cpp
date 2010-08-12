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

#include "TranscodingPropertySpinBoxWidget.h"

#include <QHBoxLayout>

namespace Transcoding
{

PropertySpinBoxWidget::PropertySpinBoxWidget( Property property, QWidget * parent )
    : QWidget( parent )
    , m_property( property )
{
    m_name = property.name();

    QBoxLayout *mainLayout;
    m_mainLabel = new QLabel( m_property.prettyName(), this );
    m_mainLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

    mainLayout = new QHBoxLayout( this );
    mainLayout->addSpacing( 5 );
    mainLayout->addWidget( m_mainLabel, 1 );

    m_mainEdit = new QSpinBox( this );
    m_mainEdit->setRange( m_property.min(), m_property.max() );
    m_mainEdit->setValue( m_property.defaultValue() );

    mainLayout->addWidget( m_mainEdit );
    mainLayout->addSpacing( 5 );

    m_mainEdit->setToolTip( m_property.description() );
    m_mainLabel->setBuddy( m_mainEdit );
}

QVariant
PropertySpinBoxWidget::value() const
{
    return m_mainEdit->value();
}

} //namespace Transcoding
