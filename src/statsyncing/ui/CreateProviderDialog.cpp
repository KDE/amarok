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

#include "CreateProviderDialog.h"

#include "statsyncing/Controller.h"
#include "statsyncing/Provider.h"
#include "core/support/Components.h"

#include <KLocalizedString>

#include <QLabel>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QPushButton>

namespace StatSyncing
{

CreateProviderDialog::CreateProviderDialog( QWidget *parent, Qt::WindowFlags f )
    : KAssistantDialog( parent, f )
{
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
    setWindowTitle( i18n( "Add Synchronization Target" ) );
    setModal( true );
    buttonBox()->button(QDialogButtonBox::Help)->setVisible(false);

    m_providerButtons.setExclusive( true );
    m_layout = new QVBoxLayout;

    QWidget *providerTypeWidget = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout;

    QLabel *warning = new QLabel( i18n( "<span style=\"color:red; font-weight:bold;\">"
                                  "Important:</span> before synchronizing tracks with a "
                                  "file-based target always make sure that "
                                  "the database file is not currently in use!" ) );
    warning->setWordWrap( true );
    mainLayout->addLayout( m_layout );
    mainLayout->addSpacing( 10 );
    mainLayout->addStretch();
    mainLayout->addWidget( warning );

    providerTypeWidget->setLayout( mainLayout );

    m_providerTypePage = new KPageWidgetItem( providerTypeWidget,
                                              i18n( "Choose Target Type" ) );
    providerTypeWidget->hide();
    addPage( m_providerTypePage );

    connect( this, &CreateProviderDialog::accepted, this, &CreateProviderDialog::slotAccepted );
}

CreateProviderDialog::~CreateProviderDialog()
{
}

void
CreateProviderDialog::addProviderType( const QString &id, const QString &prettyName,
                                       const QIcon &icon,
                                       ProviderConfigWidget *configWidget )
{
    QRadioButton *providerTypeButton = new QRadioButton;
    providerTypeButton->setText( prettyName );
    providerTypeButton->setIcon( icon );

    m_providerButtons.addButton( providerTypeButton );
    m_idForButton.insert( providerTypeButton, id );

    m_layout->insertWidget( buttonInsertPosition( prettyName ), providerTypeButton );

    KPageWidgetItem *configPage =
            new KPageWidgetItem( configWidget, i18n( "Configure Target" ) );
    m_configForButton.insert( providerTypeButton, configPage );
    addPage( configPage );
    setAppropriate( configPage, false );

    connect( providerTypeButton, &QAbstractButton::toggled,
             this, &CreateProviderDialog::providerButtonToggled );

    if( !m_providerButtons.checkedButton() )
        providerTypeButton->setChecked( true );
}

int
CreateProviderDialog::buttonInsertPosition( const QString &prettyName )
{
    for( int i = 0; i < m_layout->count(); ++i )
    {
        const QRadioButton * const button =
                dynamic_cast<const QRadioButton*>( m_layout->itemAt( i )->widget() );

        if( button != nullptr && prettyName.localeAwareCompare( button->text() ) <= 0 )
            return i;
    }

    // Nothing found, place the button at the end
    return -1;
}

void
CreateProviderDialog::providerButtonToggled( bool checked )
{
    KPageWidgetItem *configPage = m_configForButton[sender()];
    setAppropriate( configPage, checked );
}

void
CreateProviderDialog::slotAccepted()
{
    QAbstractButton *checkedButton = m_providerButtons.checkedButton();
    if( !checkedButton ) return;

    const QString id = m_idForButton[checkedButton];
    KPageWidgetItem *configPage = m_configForButton[checkedButton];
    const ProviderConfigWidget *configWidget =
            qobject_cast<ProviderConfigWidget*>( configPage->widget() );

    Q_EMIT providerConfigured( id, configWidget->config() );
}

} // namespace StatSyncing
