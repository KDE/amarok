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

#include "AmarokConfigWidget.h"

#include <QDir>
#include <QLayout>

using namespace StatSyncing;

AmarokConfigWidget::AmarokConfigWidget( const QVariantMap &config,
                                        QWidget *parent, Qt::WindowFlags f )
    : ProviderConfigWidget( parent, f )
    , m_config( config )
{
    setupUi( this );

    m_embeddedDbSettings << m_databaseLocation << m_databaseLocationLabel
                         << m_mysqlBinary << m_mysqlBinaryLabel;
    m_externalDbSettings << m_databaseName << m_databaseNameLabel << m_hostname
                         << m_hostnameLabel << m_password << m_passwordLabel
                         << m_port << m_portLabel << m_username << m_usernameLabel;

    connect( m_connectionType, QOverload<int>::of( &KComboBox::currentIndexChanged ),
             this, &AmarokConfigWidget::connectionTypeChanged );
    populateFields();
}

AmarokConfigWidget::~AmarokConfigWidget()
{
}

QVariantMap
AmarokConfigWidget::config() const
{
    QVariantMap cfg( m_config );
    cfg.insert( QStringLiteral("name"), m_targetName->text() );
    cfg.insert( QStringLiteral("embedded"), m_connectionType->currentIndex() == Embedded );
    cfg.insert( QStringLiteral("mysqlBinary"), m_mysqlBinary->text() );
    cfg.insert( QStringLiteral("dbPath"), m_databaseLocation->text() );
    cfg.insert( QStringLiteral("dbName"), m_databaseName->text() );
    cfg.insert( QStringLiteral("dbHost"), m_hostname->text() );
    cfg.insert( QStringLiteral("dbUser"), m_username->text() );
    cfg.insert( QStringLiteral("dbPass"), m_password->text() );
    cfg.insert( QStringLiteral("dbPort"), m_port->value() );

    return cfg;
}

void
AmarokConfigWidget::connectionTypeChanged( const int index )
{
    const bool embedded = ( index == Embedded );
    const QList<QWidget*> &hide = embedded ? m_externalDbSettings : m_embeddedDbSettings;
    const QList<QWidget*> &show = embedded ? m_embeddedDbSettings : m_externalDbSettings;

    for( QWidget *widget : hide )
        widget->hide();
    for( QWidget *widget : show )
        widget->show();
}

void
AmarokConfigWidget::populateFields()
{
    m_targetName->setText( m_config.value( QStringLiteral("name"), QStringLiteral("Amarok") ).toString() );

    m_connectionType->insertItem( Embedded, i18nc( "Database type", "embedded" ) );
    m_connectionType->insertItem( External, i18nc( "Database type", "external" ) );

    m_connectionType->setCurrentIndex(
                m_config.value( QStringLiteral("embedded") ).toBool()
                ? Embedded : External );

    const QString defaultPath( QStringLiteral("/usr/sbin/mysqld") );
    m_mysqlBinary->setText( m_config.value( QStringLiteral("mysqlBinary"), defaultPath ).toString() );
    m_databaseLocation->setText( m_config.value( QStringLiteral("dbPath"), QStringLiteral("") ).toString() );
    m_databaseName->setText( m_config.value( QStringLiteral("dbName"), QStringLiteral("amarokdb") ).toString() );
    m_hostname->setText( m_config.value( QStringLiteral("dbHost"), QStringLiteral("localhost") ).toString() );
    m_username->setText( m_config.value( QStringLiteral("dbUser"), QStringLiteral("amarokuser") ).toString() );
    m_password->setText( m_config.value( QStringLiteral("dbPass"), QStringLiteral("") ).toString() );
    m_port->setValue( m_config.value( QStringLiteral("dbPort"), 3306 ).toInt() );
}
