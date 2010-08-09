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

#include "TranscodingPropertyWidget.h"

#include "core/support/Debug.h"

#include <QHBoxLayout>
#include <QSlider>
#include <QLineEdit>
#include <QComboBox>

namespace Transcoding
{

PropertyWidget::PropertyWidget( Property property, QWidget * parent )
    : QWidget( parent )
{
    m_name = property.name();
    QHBoxLayout *mainLayout = new QHBoxLayout( this );
    m_mainLabel = new QLabel( property.prettyName(), this );
    m_mainLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    mainLayout->addSpacing( 5 );
    mainLayout->addWidget( m_mainLabel, 1 );

    switch( property.type() )
    {
    case Property::NUMERIC:
        m_mainEdit = new QSlider( this );
        qobject_cast< QSlider * >( m_mainEdit )->setOrientation( Qt::Horizontal );
        qobject_cast< QSlider * >( m_mainEdit )->setRange( property.min(), property.max() );
        qobject_cast< QSlider * >( m_mainEdit )->setValue( property.defaultValue() );
        qobject_cast< QSlider * >( m_mainEdit )->setTickPosition( QSlider::TicksBelow );
        break;
    case Property::TEXT:
        m_mainEdit = new QLineEdit( this );
        qobject_cast< QLineEdit * >( m_mainEdit )->setText( property.defaultText() );
        m_mainEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
        break;
    case Property::LIST:
        m_mainEdit = new QComboBox( this );
        qobject_cast< QComboBox * >( m_mainEdit )->addItems( property.prettyValues() );
        m_listValues = property.values();
        qobject_cast< QComboBox * >( m_mainEdit )->setCurrentIndex( property.defaultIndex() );
        m_mainEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
        break;
    default:
        debug() << "Muy bad!";
    }
    m_mainEdit->setToolTip( property.prettyName() );
    mainLayout->addWidget( m_mainEdit );
    m_mainLabel->setBuddy( m_mainEdit );
    mainLayout->addSpacing( 5 );
}

QVariant
PropertyWidget::value() const
{
    QByteArray className = m_mainEdit->metaObject()->className();

    if( className == "QSlider" )
        return qobject_cast< QSlider * >( m_mainEdit )->value();

    if( className == "QLineEdit" )
        return qobject_cast< QLineEdit * >( m_mainEdit )->text();

    if( className == "QComboBox" )
        return m_listValues.at( qobject_cast< QComboBox * >( m_mainEdit )->currentIndex() );

    return QVariant();
}

} //namespace Transcoding
