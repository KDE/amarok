/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "SimpleImporterConfigWidget.h"

#include "core/support/Debug.h"

#include <KLocalizedString>

#include <QBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

using namespace StatSyncing;

SimpleImporterConfigWidget::SimpleImporterConfigWidget( const QString &targetName,
                                                        const QVariantMap &config,
                                                        QWidget *parent,
                                                        Qt::WindowFlags f )
    : ProviderConfigWidget( parent, f )
    , m_config( config )
    , m_layout( new QGridLayout )
{
    m_layout->setColumnMinimumWidth( 0, 100 );
    m_layout->setColumnMinimumWidth( 1, 250 );
    m_layout->setColumnStretch( 0, 0 );
    m_layout->setColumnStretch( 1, 1 );

    QBoxLayout *mainLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    mainLayout->addLayout( m_layout, 0 );
    mainLayout->addStretch( 1 );
    setLayout( mainLayout );

    addField( QStringLiteral("name"), i18nc( "Name of the synchronization target", "Target name" ),
              new QLineEdit( targetName ),  QStringLiteral("text") );
}

SimpleImporterConfigWidget::~SimpleImporterConfigWidget()
{
}

void
SimpleImporterConfigWidget::addField( const QString &configName, const QString &label,
                                      QWidget * const field, const QString &property )
{
    if( !field )
    {
        warning() << __PRETTY_FUNCTION__ << "Attempted to add null field";
        return;
    }

    QLabel *lwidget = new QLabel( label );
    lwidget->setBuddy( field );

    const int row = m_layout->rowCount();
    m_layout->addWidget( lwidget, row, 0 );
    m_layout->addWidget( field, row, 1 );

    // Populate field with previously configured value
    if( m_config.contains( configName ) )
    {
        const QByteArray propertyName = property.toLocal8Bit();
        field->setProperty( propertyName.constData(), m_config.value( configName ) );
    }

    m_fieldForName.insert( configName, qMakePair( field, property ) );
}

QVariantMap
SimpleImporterConfigWidget::config() const
{
    QVariantMap cfg( m_config );

    for( const QString &key : m_fieldForName.keys() )
    {
        const QPair<QWidget*, QString> val = m_fieldForName.value( key );
        const QByteArray propertyName = val.second.toLocal8Bit();
        cfg.insert( key, val.first->property( propertyName.constData() ) );
    }

    return cfg;
}
