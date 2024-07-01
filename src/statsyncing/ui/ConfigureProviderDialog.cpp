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

#include "ConfigureProviderDialog.h"

#include "statsyncing/Provider.h"

#include <KLocalizedString>

#include <QPushButton>

namespace StatSyncing
{

ConfigureProviderDialog::ConfigureProviderDialog( const QString &providerId,
                                                  QWidget *configWidget, QWidget *parent,
                                                  Qt::WindowFlags f )
    : KPageDialog( parent, f )
    , m_providerId( providerId )
{
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    setWindowTitle( i18n( "Configure Synchronization Target" ) );
    setModal( true );
    buttonBox()->button(QDialogButtonBox::Help)->setVisible(false);

    mainWidget = configWidget;

    connect( this, &ConfigureProviderDialog::accepted, this, &ConfigureProviderDialog::slotAccepted );
}

ConfigureProviderDialog::~ConfigureProviderDialog()
{
}

void
ConfigureProviderDialog::slotAccepted()
{
    const ProviderConfigWidget *configWidget = qobject_cast<ProviderConfigWidget*>(mainWidget);

    Q_EMIT providerConfigured( m_providerId, configWidget->config() );
}

} // namespace StatSyncing
