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

#include "FastForwardConfigWidget.h"

#include <QDir>
#include <QLayout>
#include <QMetaEnum>

using namespace StatSyncing;

FastForwardConfigWidget::FastForwardConfigWidget( const QVariantMap &config,
                                                  QWidget *parent, Qt::WindowFlags f )
    : ProviderConfigWidget( parent, f )
    , m_config( config )
{
    setupUi( this );

    m_embeddedDbSettings << m_databaseLocation << m_databaseLocationLabel;
    m_externalDbSettings << m_databaseName << m_databaseNameLabel << m_hostname
                         << m_hostnameLabel << m_password << m_passwordLabel
                         << m_port << m_portLabel << m_username << m_usernameLabel;

    connect( m_connectionType, QOverload<int>::of( &KComboBox::currentIndexChanged ),
             this, &FastForwardConfigWidget::connectionTypeChanged );
    populateFields();
}

FastForwardConfigWidget::~FastForwardConfigWidget()
{
}

QVariantMap
FastForwardConfigWidget::config() const
{
    QVariantMap cfg = m_config;

    const int enumId = metaObject()->indexOfEnumerator( "Driver" );
    QMetaEnum driverEnum = metaObject()->enumerator( enumId );

    cfg.insert( "name", m_targetName->text() );
    cfg.insert( "dbDriver", driverEnum.valueToKey(
                    Driver( m_connectionType->currentIndex() ) ) );

    cfg.insert( "dbPath", m_databaseLocation->text() );
    cfg.insert( "dbName", m_databaseName->text() );
    cfg.insert( "dbHost", m_hostname->text() );
    cfg.insert( "dbUser", m_username->text() );
    cfg.insert( "dbPass", m_password->text() );
    cfg.insert( "dbPort", m_port->value() );

    return cfg;
}

void
FastForwardConfigWidget::connectionTypeChanged( const int index )
{
    const bool embedded = ( index == QSQLITE );
    const QList<QWidget*> &hide = embedded ? m_externalDbSettings : m_embeddedDbSettings;
    const QList<QWidget*> &show = embedded ? m_embeddedDbSettings : m_externalDbSettings;

    for( QWidget *widget : hide )
        widget->hide();
    for( QWidget *widget : show )
        widget->show();
}

void
FastForwardConfigWidget::populateFields()
{
    m_targetName->setText( m_config.value( "name", "Amarok 1.4" ).toString() );

    const int enumId = metaObject()->indexOfEnumerator( "Driver" );
    QMetaEnum driverEnum = metaObject()->enumerator( enumId );

    m_connectionType->insertItem( QMYSQL,  "MySQL" );
    m_connectionType->insertItem( QPSQL,   "PostgreSQL" );
    m_connectionType->insertItem( QSQLITE, "SQLite" );

    const QByteArray dbDriver = m_config.value( "dbDriver",
                                         driverEnum.valueToKey( QSQLITE ) ).toByteArray();

    int index = driverEnum.keyToValue( dbDriver.constData() );
    if( index == -1 )
        index = QSQLITE;

    m_connectionType->setCurrentIndex( index );

    const QString defaultPath = QDir::toNativeSeparators(
                QDir::homePath() + QStringLiteral("/.kde/share/apps/amarok/collection.db") );

    m_databaseLocation->setText( m_config.value( "dbPath", defaultPath ).toString() );
    m_databaseName->setText( m_config.value( "dbName", "amarokdb" ).toString() );
    m_hostname->setText( m_config.value( "dbHost", "localhost" ).toString() );
    m_username->setText( m_config.value( "dbUser", "amarokuser" ).toString() );
    m_password->setText( m_config.value( "dbPass", "" ).toString() );
    m_port->setValue( m_config.value( "dbPort", 3306 ).toInt() );
}
