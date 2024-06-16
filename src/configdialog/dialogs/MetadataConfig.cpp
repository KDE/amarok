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

#include "MetadataConfig.h"

#include "amarokconfig.h"
#include "configdialog/ConfigDialog.h"
#include "configdialog/dialogs/ExcludedLabelsDialog.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "statsyncing/Config.h"
#include "statsyncing/Controller.h"

#include "MetaValues.h"

MetadataConfig::MetadataConfig( Amarok2ConfigDialog *parent )
    : ConfigDialogBase( parent )
{
    connect( this, &MetadataConfig::changed,
             parent, &Amarok2ConfigDialog::updateButtons );

    setupUi( this );

    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Small (200 px)" ), QVariant( 200 ) );
    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Medium (400 px)" ), QVariant( 400 ) );
    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Large (800 px)" ), QVariant( 800 ) );
    m_writeBackCoverDimensions->addItem(
        i18nc("Maximum cover size option", "Huge (1600 px)" ), QVariant( 1600 ) );

    m_writeBack->setChecked( AmarokConfig::writeBack() );
    m_writeBack->setVisible( false ); // probably not a usecase
    m_writeBackStatistics->setChecked( AmarokConfig::writeBackStatistics() );
    m_writeBackStatistics->setEnabled( m_writeBack->isChecked() );
    m_writeBackCover->setChecked( AmarokConfig::writeBackCover() );
    m_writeBackCover->setEnabled( m_writeBack->isChecked() );
    if( m_writeBackCoverDimensions->findData( AmarokConfig::writeBackCoverDimensions() ) != -1 )
        m_writeBackCoverDimensions->setCurrentIndex( m_writeBackCoverDimensions->findData( AmarokConfig::writeBackCoverDimensions() ) );
    else
        m_writeBackCoverDimensions->setCurrentIndex( 1 ); // medium
    m_writeBackCoverDimensions->setEnabled( m_writeBackCover->isEnabled() && m_writeBackCover->isChecked() );
    m_useCharsetDetector->setChecked( AmarokConfig::useCharsetDetector() );
    connect( m_writeBack, &QCheckBox::toggled, this, &MetadataConfig::changed );
    connect( m_writeBackStatistics, &QCheckBox::toggled, this, &MetadataConfig::changed );
    connect( m_writeBackCover, &QCheckBox::toggled, this, &MetadataConfig::changed );
    connect( m_writeBackCoverDimensions, QOverload<int>::of(&KComboBox::currentIndexChanged),
             this, &MetadataConfig::changed );
    connect( m_useCharsetDetector, &QCheckBox::toggled, this, &MetadataConfig::changed );

    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    StatSyncing::Config *config = controller ? controller->config() : nullptr;
    m_statSyncingConfig = config;
    m_statSyncingProvidersView->setModel( config );
    m_synchronizeButton->setIcon( QIcon::fromTheme( QStringLiteral("amarok_playcount") ) );
    m_configureTargetButton->setIcon( QIcon::fromTheme( QStringLiteral("configure") ) );
    connect( config, &StatSyncing::Config::dataChanged, this, &MetadataConfig::changed );
    connect( config, &StatSyncing::Config::rowsInserted, this, &MetadataConfig::changed );
    connect( config, &StatSyncing::Config::rowsRemoved, this, &MetadataConfig::changed );
    connect( config, &StatSyncing::Config::modelReset, this, &MetadataConfig::changed );

    // Add target button
    m_addTargetButton->setEnabled( controller && controller->hasProviderFactories() );
    connect( m_addTargetButton, &QAbstractButton::clicked, this, &MetadataConfig::slotCreateProviderDialog );

    // Configure target button
    m_configureTargetButton->setEnabled( false );
    connect( m_statSyncingProvidersView->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &MetadataConfig::slotUpdateProviderConfigureButton );
    connect( m_configureTargetButton, &QAbstractButton::clicked, this, &MetadataConfig::slotConfigureProvider );

    // Forget target button
    connect( m_statSyncingProvidersView->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &MetadataConfig::slotUpdateForgetButton );
    connect( m_forgetTargetsButton, &QAbstractButton::clicked, this, &MetadataConfig::slotForgetCollections );

    // Synchronize button
    if( controller )
        connect( m_synchronizeButton, &QAbstractButton::clicked,
                 controller, &StatSyncing::Controller::synchronize );
    else
        m_synchronizeButton->setEnabled( false );

    slotUpdateForgetButton();

    const qint64 checkedFields = config ? config->checkedFields() : 0;
    QMap<qint64, QString> i18nSyncLabels; // to avoid word puzzles, bug 334561
    i18nSyncLabels.insert( Meta::valRating, i18nc( "Statistics sync checkbox label",
                                                   "Synchronize Ratings" ) );
    i18nSyncLabels.insert( Meta::valFirstPlayed, i18nc( "Statistics sync checkbox label",
                                                        "Synchronize First Played Times" ) );
    i18nSyncLabels.insert( Meta::valLastPlayed, i18nc( "Statistics sync checkbox label",
                                                       "Synchronize Last Played Times" ) );
    i18nSyncLabels.insert( Meta::valPlaycount, i18nc( "Statistics sync checkbox label",
                                                      "Synchronize Playcounts" ) );
    i18nSyncLabels.insert( Meta::valLabel, i18nc( "Statistics sync checkbox label",
                                                  "Synchronize Labels" ) );

    for( qint64 field : StatSyncing::Controller::availableFields() )
    {
        QString name = i18nSyncLabels.value( field );
        if( name.isEmpty() )
        {
            name = i18nc( "%1 is field name such as Play Count (this string is only used "
                          "as a fallback)", "Synchronize %1", Meta::i18nForField( field ) );
            warning() << Q_FUNC_INFO << "no explicit traslation for" << name << "using fallback";
        }
        QCheckBox *checkBox = new QCheckBox( name );

        if( field == Meta::valLabel ) // special case, we want plural:
        {
            QHBoxLayout *lineLayout = new QHBoxLayout();
            QLabel *button = new QLabel();
            button->setObjectName( QStringLiteral("configureLabelExceptions") );
            connect( button, &QLabel::linkActivated,
                     this, &MetadataConfig::slotConfigureExcludedLabels );

            lineLayout->addWidget( checkBox );
            lineLayout->addWidget( button );
            lineLayout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
            m_statSyncingFieldsLayout->addLayout( lineLayout );

            slotUpdateConfigureExcludedLabelsLabel();
        }
        else
        {
            m_statSyncingFieldsLayout->addWidget( checkBox );
        }
        checkBox->setCheckState( ( field & checkedFields ) ? Qt::Checked : Qt::Unchecked );
        checkBox->setProperty( "field", field );
        connect( checkBox, &QCheckBox::stateChanged, this, &MetadataConfig::changed );
    }
}

MetadataConfig::~MetadataConfig()
{
    if( m_statSyncingConfig )
    {
        disconnect( this, &MetadataConfig::changed, nullptr, nullptr );
        m_statSyncingConfig.data()->read(); // reset unsaved changes
    }
}

bool
MetadataConfig::isDefault()
{
    return false;
}

bool
MetadataConfig::hasChanged()
{
    // a bit hacky, but updating enabled status here does the trick
    m_writeBackStatistics->setEnabled( m_writeBack->isChecked() );
    m_writeBackCover->setEnabled( m_writeBack->isChecked() );
    m_writeBackCoverDimensions->setEnabled( m_writeBackCover->isEnabled() && m_writeBackCover->isChecked() );

    return
        m_writeBack->isChecked() != AmarokConfig::writeBack() ||
        m_writeBackStatistics->isChecked() != AmarokConfig::writeBackStatistics() ||
        m_writeBackCover->isChecked() != AmarokConfig::writeBackCover() ||
        writeBackCoverDimensions() != AmarokConfig::writeBackCoverDimensions() ||
        m_useCharsetDetector->isChecked() != AmarokConfig::useCharsetDetector() ||
        ( m_statSyncingConfig.data() ? ( checkedFields() != m_statSyncingConfig->checkedFields() ) : false ) ||
        ( m_statSyncingConfig.data() ? m_statSyncingConfig->hasChanged() : false );
}

void
MetadataConfig::updateSettings()
{
    AmarokConfig::setWriteBack( m_writeBack->isChecked() );
    AmarokConfig::setWriteBackStatistics( m_writeBackStatistics->isChecked() );
    AmarokConfig::setWriteBackCover( m_writeBackCover->isChecked() );
    if( writeBackCoverDimensions() > 0 )
        AmarokConfig::setWriteBackCoverDimensions( writeBackCoverDimensions() );
    AmarokConfig::setUseCharsetDetector( m_useCharsetDetector->isChecked() );
    if( m_statSyncingConfig )
    {
        m_statSyncingConfig->setCheckedFields( checkedFields() );
        m_statSyncingConfig->save();
    }
}

void
MetadataConfig::slotForgetCollections()
{
    if( !m_statSyncingConfig )
        return;
    for( const QModelIndex &idx : m_statSyncingProvidersView->selectionModel()->selectedIndexes() )
    {
        QString id = idx.data( StatSyncing::Config::ProviderIdRole ).toString();
        m_statSyncingConfig->forgetProvider( id );
    }
}

void
MetadataConfig::slotUpdateForgetButton()
{
    QItemSelectionModel *selectionModel = m_statSyncingProvidersView->selectionModel();
    // note: hasSelection() and selection() gives false positives!
    m_forgetTargetsButton->setEnabled( !selectionModel->selectedIndexes().isEmpty() );
}

void
MetadataConfig::slotUpdateConfigureExcludedLabelsLabel()
{
    QLabel *label = findChild<QLabel *>( QStringLiteral("configureLabelExceptions") );
    if( !label || !m_statSyncingConfig )
    {
        warning() << __PRETTY_FUNCTION__ << "label or m_statSyncingConfig is null!";
        return;
    }
    int exceptions = m_statSyncingConfig->excludedLabels().count();
    QString begin = QStringLiteral("<a href='dummy'>");
    QString end = QStringLiteral("</a>");
    label->setText( i18np( "(%2one exception%3)", "(%2%1 exceptions%3)", exceptions,
                           begin, end ) );
}

void
MetadataConfig::slotConfigureExcludedLabels()
{
    ExcludedLabelsDialog dialog( m_statSyncingConfig.data(), this );
    if( dialog.exec() == QDialog::Accepted )
    {
        slotUpdateConfigureExcludedLabelsLabel();
        Q_EMIT changed();
    }
}

void
MetadataConfig::slotConfigureProvider()
{
    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    if( controller )
    {
        QModelIndexList selected = m_statSyncingProvidersView->selectionModel()->selectedIndexes();
        if( selected.size() == 1 )
        {
            const QString id = selected.front().data( StatSyncing::Config::ProviderIdRole ).toString();

            QWidget *dialog = controller->providerConfigDialog( id );
            if( dialog )
            {
                dialog->show();
                dialog->activateWindow();
                dialog->raise();
            }
        }
    }
}

void
MetadataConfig::slotUpdateProviderConfigureButton()
{
    QModelIndexList selected = m_statSyncingProvidersView->selectionModel()->selectedIndexes();
    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();

    if( selected.size() != 1 || !controller )
    {
        m_configureTargetButton->setEnabled( false );
    }
    else
    {
        const QString id = selected.front().data( StatSyncing::Config::ProviderIdRole ).toString();
        m_configureTargetButton->setEnabled( controller->providerIsConfigurable( id ) );
    }
}

void
MetadataConfig::slotCreateProviderDialog()
{
    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    if( controller )
    {
        QWidget *dialog = controller->providerCreationDialog();
        if( dialog )
        {
            dialog->show();
            dialog->activateWindow();
            dialog->raise();
        }
    }
}

int
MetadataConfig::writeBackCoverDimensions() const
{
    return m_writeBackCoverDimensions->itemData( m_writeBackCoverDimensions->currentIndex() ).toInt();
}

qint64
MetadataConfig::checkedFields() const
{
    qint64 ret = 0;
    auto list = m_statSyncingFieldsLayout->parentWidget()->findChildren<QCheckBox *>();
    for( QCheckBox *checkBox : list )
    {
        if( checkBox->isChecked() && checkBox->property( "field" ).canConvert<qint64>() )
            ret |= checkBox->property( "field" ).value<qint64>();
    }
    return ret;
}
