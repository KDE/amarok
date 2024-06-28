/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "ChooseProvidersPage.h"

#include "App.h"
#include "core/meta/support/MetaConstants.h"
#include "statsyncing/models/ProvidersModel.h"

#include <QCheckBox>
#include <QPushButton>

using namespace StatSyncing;

ChooseProvidersPage::ChooseProvidersPage( QWidget *parent, Qt::WindowFlags f )
    : QWidget( parent, f )
    , m_providersModel( nullptr )
{
    setupUi( this );
    QPushButton *configure = buttonBox->addButton( i18n( "Configure Synchronization..." ), QDialogButtonBox::ActionRole );
    connect( configure, &QPushButton::clicked, this, &ChooseProvidersPage::openConfiguration );
    QPushButton *next = buttonBox->addButton( i18n( "Next" ), QDialogButtonBox::ActionRole );
    next->setIcon( QIcon( QStringLiteral( "go-next" ) ) );
    connect( next, &QPushButton::clicked, buttonBox, &QDialogButtonBox::accepted );
    connect( buttonBox, &QDialogButtonBox::accepted, this, &ChooseProvidersPage::accepted );
    connect( buttonBox, &QDialogButtonBox::rejected, this, &ChooseProvidersPage::rejected );
    progressBar->hide();
}

ChooseProvidersPage::~ChooseProvidersPage()
{
}

void
ChooseProvidersPage::setFields( const QList<qint64> &fields, qint64 checkedFields )
{
    QLayout *fieldsLayout = fieldsBox->layout();
    for( qint64 field : fields )
    {
        QString name = Meta::i18nForField( field );
        QCheckBox *checkBox = new QCheckBox( name );
        fieldsLayout->addWidget( checkBox );
        checkBox->setCheckState( ( field & checkedFields ) ? Qt::Checked : Qt::Unchecked );
        checkBox->setProperty( "field", field );
        connect( checkBox, &QCheckBox::stateChanged, this, &ChooseProvidersPage::checkedFieldsChanged );
    }
    fieldsLayout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding ) );

    connect( this, &ChooseProvidersPage::checkedFieldsChanged, this, &ChooseProvidersPage::updateEnabledFields );
    updateEnabledFields();
}

qint64
ChooseProvidersPage::checkedFields() const
{
    qint64 ret = 0;
    QLayout *fieldsLayout = fieldsBox->layout();
    for( int i = 0; i < fieldsLayout->count(); i++ )
    {
        QCheckBox *checkBox = qobject_cast<QCheckBox *>( fieldsLayout->itemAt( i )->widget() );
        if( !checkBox )
            continue;
        if( checkBox->isChecked() && checkBox->property( "field" ).canConvert<qint64>() )
            ret |= checkBox->property( "field" ).value<qint64>();
    }
    return ret;
}

void
ChooseProvidersPage::setProvidersModel( ProvidersModel *model, QItemSelectionModel *selectionModel )
{
    m_providersModel = model;
    providersView->setModel( model );
    providersView->setSelectionModel( selectionModel );

    connect( model, &StatSyncing::ProvidersModel::selectedProvidersChanged,
             this, &ChooseProvidersPage::updateMatchedLabel );
    connect( model, &StatSyncing::ProvidersModel::selectedProvidersChanged,
             this, &ChooseProvidersPage::updateEnabledFields );
    updateMatchedLabel();
    updateEnabledFields();
}

void
ChooseProvidersPage::disableControls()
{
    // disable checkboxes
    QLayout *fieldsLayout = fieldsBox->layout();
    for( int i = 0; i < fieldsLayout->count(); i++ )
    {
        QWidget *widget = fieldsLayout->itemAt( i )->widget();
        if( widget )
            widget->setEnabled( false );
    }

    // disable view
    providersView->setEnabled( false );

    // disable all but Cancel button
    for( QAbstractButton *button : buttonBox->buttons() )
    {
        if( buttonBox->buttonRole( button ) != QDialogButtonBox::RejectRole )
            button->setEnabled( false );
    }
}

void
ChooseProvidersPage::setProgressBarText( const QString &text )
{
    progressBar->setFormat( text );
    progressBar->show();
}

void
ChooseProvidersPage::setProgressBarMaximum( int maximum )
{
    progressBar->setMaximum( maximum );
    progressBar->show();
}

void
ChooseProvidersPage::progressBarIncrementProgress()
{
    progressBar->setValue( progressBar->value() + 1 );
    progressBar->show();
}

void
ChooseProvidersPage::updateMatchedLabel()
{
    qint64 fields = m_providersModel->reliableTrackMetadataIntersection();
    QString fieldNames = m_providersModel->fieldsToString( fields );
    matchLabel->setText( i18n( "Tracks matched by: %1", fieldNames ) );
}

void
ChooseProvidersPage::updateEnabledFields()
{
    if( !m_providersModel )
        return;

    qint64 writableFields = m_providersModel->writableTrackStatsDataUnion();
    QLayout *fieldsLayout = fieldsBox->layout();
    for( int i = 0; i < fieldsLayout->count(); i++ )
    {
        QWidget *checkBox = fieldsLayout->itemAt( i )->widget();
        if( !checkBox || !checkBox->property( "field" ).canConvert<qint64>() )
            continue;
        qint64 field = checkBox->property( "field" ).value<qint64>();
        bool enabled = writableFields & field;
        checkBox->setEnabled( enabled );
        QString text = i18nc( "%1 is field name such as Rating", "No selected collection "
                "supports writing %1 - it doesn't make sense to synchronize it.",
                Meta::i18nForField( field ) );
        checkBox->setToolTip( enabled ? QString() : text );
    }

    QAbstractButton *nextButton = nullptr;
    for( QAbstractButton *button : buttonBox->buttons() )
    {
        if( buttonBox->buttonRole( button ) == QDialogButtonBox::AcceptRole )
            nextButton = button;
    }
    if( nextButton )
        nextButton->setEnabled( writableFields != 0 );
}

void ChooseProvidersPage::openConfiguration()
{
    App *app = pApp;
    if( app )
        app->slotConfigAmarok( QStringLiteral("MetadataConfig") );
}
