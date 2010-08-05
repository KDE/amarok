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

#include "TranscodeConfigurationElement.h"
#include "core/support/Debug.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

TranscodeConfigurationElement
TranscodeConfigurationElement::Numeric( const QString &label, long defaultValue, long min, long max )
{
    return TranscodeConfigurationElement( label, NUMERIC, defaultValue, min, max, QStringList(), QString() );
}

TranscodeConfigurationElement
TranscodeConfigurationElement::String( const QString &label, const QString &defaultText )
{
    return TranscodeConfigurationElement( label, TEXT, 0, 0, 0, QStringList(), defaultText );
}

TranscodeConfigurationElement
TranscodeConfigurationElement::List( const QString &label, const QStringList &values, int defaultValueIndex )
{
    return TranscodeConfigurationElement( label, LIST, defaultValueIndex, 0, 0, values, QString() );
}

TranscodeConfigurationElement::TranscodeConfigurationElement( const QString &label,
                                                              Type type,
                                                              long defaultNumber,
                                                              long min,
                                                              long max,
                                                              const QStringList &values,
                                                              const QString &defaultText )
    : m_label( label )
    , m_type( type )
    , m_defaultNumber( defaultNumber )
    , m_min( min )
    , m_max( max )
    , m_values( values )
    , m_defaultString( defaultText )
{}

QWidget *
TranscodeConfigurationElement::createConfigurationWidget( QWidget *parent )
{
    if( !m_widget.isNull() )
        return m_widget;
    m_widget = new QWidget( parent );
    QHBoxLayout *mainLayout = new QHBoxLayout( m_widget );
    QLabel *mainLabel = new QLabel( m_label, m_widget );
    mainLayout->addWidget( mainLabel, 1 );
    QWidget *mainEdit;
    switch( m_type )
    {
    case NUMERIC:
        mainEdit = new QSpinBox( m_widget );
        qobject_cast< QSpinBox * >( mainEdit )->setRange( m_min, m_max );
        qobject_cast< QSpinBox * >( mainEdit )->setValue( m_defaultNumber );
        break;
    case TEXT:
        mainEdit = new QLineEdit( m_widget );
        qobject_cast< QLineEdit * >( mainEdit )->setText( m_defaultString );
        break;
    case LIST:
        mainEdit = new QComboBox( m_widget );
        qobject_cast< QComboBox * >( mainEdit )->addItems( m_values );
        qobject_cast< QComboBox * >( mainEdit )->setCurrentIndex( m_defaultNumber );
        break;
    default:
        debug() << "Muy bad!";
    }
    mainEdit->setToolTip( m_label );
    mainLayout->addWidget( mainEdit );
    mainLabel->setBuddy( mainEdit );


    return m_widget;
}

QWidget *
TranscodeConfigurationElement::widget() const
{
    return m_widget;
}
