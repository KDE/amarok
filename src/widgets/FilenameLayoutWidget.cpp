/****************************************************************************************
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Daniel Dewald <Daniel.Dewald@time.shift.de>                       *
 * Copyright (c) 2012 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "FilenameLayoutWidget.h"
#include "TokenDropTarget.h"
#include "TokenPool.h"

#include "amarokconfig.h"

#include "MetaValues.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/meta/support/MetaConstants.h"

#include <QInputDialog>
#include <QLineEdit>
#include <KLocalizedString>

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QBoxLayout>
#include <QStackedWidget>

// the order of these strings depends on the order of the
// Type enum.
static const QStringList typeElements = ( QStringList()
<< QString()
<< QLatin1String("%ignore%")
<< QLatin1String("%track%")
<< QLatin1String("%title%")
<< QLatin1String("%artist%")
<< QLatin1String("%composer%")
<< QLatin1String("%year%")
<< QLatin1String("%album%")
<< QLatin1String("%albumartist%")
<< QLatin1String("%comment%")
<< QLatin1String("%genre%")
<< QLatin1String("%filetype%")
<< QLatin1String("%folder%")
<< QLatin1String("%initial%")
<< QLatin1String("%discnumber%")
<< QLatin1String(" ")
<< QLatin1String("/")
<< QLatin1String(".")
<< QLatin1String("-")
<< QLatin1String("_")
<< QLatin1String("%collectionroot%") );

using namespace Meta;

// ------------------------- FilenameLayoutWidget -------------------

FilenameLayoutWidget::FilenameLayoutWidget( QWidget *parent )
    : QWidget( parent )
    , m_advancedMode( false )
{
    m_mainLayout = new QVBoxLayout( this );
    m_mainLayout->setContentsMargins( 0, 0, 0, 0 );

    QGroupBox* schemeGroup = new QGroupBox( i18n("Scheme"), this );
    QVBoxLayout* schemeGroupLayout = new QVBoxLayout( schemeGroup );

    // --- presets
    QHBoxLayout* presetLayout1 = new QHBoxLayout();

    QLabel* presetLabel = new QLabel( i18n("Preset:"), this );
    presetLayout1->addWidget( presetLabel, 0 );

    m_presetCombo = new QComboBox( this );
    m_presetCombo->setWhatsThis( i18n("A list of selectable filename scheme/format presets." ) );
    presetLayout1->addWidget( m_presetCombo, 1 );

    // - the preset buttons
    m_addPresetButton = new QPushButton( i18n("Add preset"), this );
    m_addPresetButton->setToolTip( i18n("Saves the current scheme/format as new preset.") );
    presetLayout1->addWidget( m_addPresetButton, 0 );

    m_updatePresetButton = new QPushButton( i18n("Update preset"), this );
    m_updatePresetButton->setToolTip( i18n("Updates the preset with the current scheme/format.") );
    presetLayout1->addWidget( m_updatePresetButton, 0 );

    m_removePresetButton = new QPushButton( i18n("Remove preset"), this );
    m_removePresetButton->setToolTip( i18n("Removes the currently selected preset.") );
    presetLayout1->addWidget( m_removePresetButton, 0 );

    schemeGroupLayout->addLayout( presetLayout1 );

    // --- stacked widget
    m_schemeStack = new QStackedWidget( this );

    // -- simple schema
    QWidget* simpleLayoutWidget = new QWidget( this );
    QVBoxLayout *simpleLayout = new QVBoxLayout( simpleLayoutWidget );

    // a token pool
    m_tokenPool = new TokenPool( this );
    simpleLayout->addWidget( m_tokenPool, 1 );

    // token drop target inside a frame
    QFrame* dropTargetFrame = new QFrame( this );
    dropTargetFrame->setFrameShape(QFrame::StyledPanel);
    dropTargetFrame->setFrameShadow(QFrame::Sunken);
    m_dropTarget = new TokenDropTarget( this );
    m_dropTarget->setRowLimit( 1 );

    m_schemaLineLayout = new QHBoxLayout();
    m_schemaLineLayout->setSpacing( 0 );
    m_schemaLineLayout->setContentsMargins( 0, 0, 0, 0 );
    m_schemaLineLayout->addWidget( m_dropTarget );
    dropTargetFrame->setLayout( m_schemaLineLayout );
    simpleLayout->addWidget( dropTargetFrame, 0 );

    m_schemeStack->addWidget( simpleLayoutWidget );

    // -- advanced schema
    QWidget* advancedLayoutWidget = new QWidget( this );
    QVBoxLayout *advancedLayout = new QVBoxLayout( advancedLayoutWidget );

    m_syntaxLabel = new QLabel( this ); // placeholder for format description
    advancedLayout->addWidget( m_syntaxLabel );

    m_filenameLayoutEdit = new QLineEdit( this );
    advancedLayout->addWidget( m_filenameLayoutEdit );

    m_schemeStack->addWidget( advancedLayoutWidget );

    schemeGroupLayout->addWidget( m_schemeStack );

    m_advancedButton = new QPushButton( i18n("Advanced"), this );
    schemeGroupLayout->addWidget( m_advancedButton );

    // --

    m_mainLayout->addWidget( schemeGroup );

    connect( m_tokenPool, &TokenPool::onDoubleClick,
             m_dropTarget, &TokenDropTarget::appendToken );
    connect( m_advancedButton, &QAbstractButton::clicked,
             this, &FilenameLayoutWidget::toggleAdvancedMode );
    connect( m_dropTarget, &TokenDropTarget::changed,
             this, &FilenameLayoutWidget::schemeChanged );
    connect( m_dropTarget, &TokenDropTarget::changed,
             this, &FilenameLayoutWidget::slotUpdatePresetButton );
    connect( m_addPresetButton, &QPushButton::clicked,
             this, &FilenameLayoutWidget::slotAddFormat );
    connect( m_removePresetButton, &QPushButton::clicked,
             this, &FilenameLayoutWidget::slotRemoveFormat );
    connect( m_updatePresetButton, &QPushButton::clicked,
             this, &FilenameLayoutWidget::slotUpdateFormat );

    connect( m_filenameLayoutEdit, &QLineEdit::textChanged,
             this, &FilenameLayoutWidget::schemeChanged );
    connect( m_filenameLayoutEdit, &QLineEdit::textChanged,
             this, &FilenameLayoutWidget::slotUpdatePresetButton );
}

Token*
FilenameLayoutWidget::createToken(qint64 value) const
{
    struct TokenDefinition
    {
        QString name;
        QString iconName;
        Type    value;
    };

    static const TokenDefinition tokenDefinitions[] = {
        { i18nForField( valTrackNr ),     iconForField( valTrackNr ),     TrackNumber },
        { i18nForField( valDiscNr ),      iconForField( valDiscNr ),      DiscNumber },
        { i18nForField( valTitle ),       iconForField( valTitle ),       Title },
        { i18nForField( valArtist ),      iconForField( valArtist ),      Artist },
        { i18nForField( valComposer ),    iconForField( valComposer ),    Composer },
        { i18nForField( valYear ),        iconForField( valYear ),        Year },
        { i18nForField( valAlbum ),       iconForField( valAlbum ),       Album },
        { i18nForField( valAlbumArtist ), iconForField( valAlbumArtist ), AlbumArtist },
        { i18nForField( valComment ),     iconForField( valComment ),     Comment },
        { i18nForField( valGenre ),       iconForField( valGenre ),       Genre },
        { i18nForField( valFormat ),      iconForField( valFormat ),      FileType },

        { i18n( "Ignore" ), QStringLiteral("filename-ignore-amarok"), Ignore },
        { i18n( "Folder" ), QStringLiteral("filename-folder-amarok"), Folder },
        { i18nc( "Artist's Initial", "Initial" ), QStringLiteral("filename-initial-amarok"), Initial },

        { QStringLiteral("/"), QStringLiteral("filename-slash-amarok"), Slash },
        { QStringLiteral("_"), QStringLiteral("filename-underscore-amarok"), Underscore },
        { QStringLiteral("-"), QStringLiteral("filename-dash-amarok"), Dash },
        { QStringLiteral("."), QStringLiteral("filename-dot-amarok"), Dot },
        { QStringLiteral(" "), QStringLiteral("filename-space-amarok"), Space },
        { i18n( "Collection root" ), QStringLiteral("drive-harddisk"), CollectionRoot },
        { QString(), QString(), Space }
    };

    for( int i = 0; !tokenDefinitions[i].name.isNull(); ++i )
    {
        if( value == tokenDefinitions[i].value )
        {
            return new Token( tokenDefinitions[i].name,
                              tokenDefinitions[i].iconName,
                              static_cast<qint64>(tokenDefinitions[i].value) );
        }
    }

    return nullptr;
}

Token*
FilenameLayoutWidget::createStaticToken(qint64 value) const
{
    Token* token = createToken( value );
    token->setEnabled( false );
    token->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );

    return token;
}

//Stores the configuration when the dialog is accepted.
void
FilenameLayoutWidget::onAccept()    //SLOT
{
    QString custom = getParsableScheme();

    // Custom scheme is stored per dialog
    debug() << "--- saving custom scheme for" << m_configCategory << custom;
    Amarok::config( m_configCategory ).writeEntry( "Custom Scheme", custom );
}

QString
FilenameLayoutWidget::getParsableScheme() const
{
    return m_advancedMode ? m_filenameLayoutEdit->text() : dropTargetScheme();
}

// attempts to set the scheme
void FilenameLayoutWidget::setScheme(const QString& scheme)
{
    m_filenameLayoutEdit->setText( scheme );
    inferScheme( scheme );

    slotUpdatePresetButton();
    Q_EMIT schemeChanged();
}

//Handles the modifications to the dialog to toggle between advanced and basic editing mode.
void
FilenameLayoutWidget::toggleAdvancedMode()
{
    setAdvancedMode( !m_advancedMode );
}

//handles switching between basic and advanced mode
void
FilenameLayoutWidget::setAdvancedMode( bool isAdvanced )
{
    setScheme( getParsableScheme() ); // setScheme set's both the edit and the drop target
    m_advancedMode = isAdvanced;

    if( isAdvanced )
    {
        m_advancedButton->setText( i18n( "&Basic..." ) );
        m_schemeStack->setCurrentIndex( 1 );
    }
    else // set Basic mode
    {
        m_advancedButton->setText( i18n( "&Advanced..." ) );
        m_schemeStack->setCurrentIndex( 0 );
    }

    QString entryValue  = m_advancedMode ? QStringLiteral("Advanced") : QStringLiteral("Basic");

    Amarok::config( m_configCategory ).writeEntry( "Mode", entryValue );
}

QString
FilenameLayoutWidget::dropTargetScheme() const
{
    QString parsableScheme = QStringLiteral("");

    QList< Token *> list = m_dropTarget->tokensAtRow();

    for( Token *token : list )
    {
        parsableScheme += typeElements[token->value()];
    }

    return parsableScheme;
}

void
FilenameLayoutWidget::inferScheme( const QString &s ) //SLOT
{
    DEBUG_BLOCK

    debug() << "inferring scheme: " << s;

    m_dropTarget->clear();
    for( int i = 0; i < s.size(); )
    {
        // - search if there is a type with the matching string
        //   representation.
        bool found = false;
        for( int j = 1; j < typeElements.size() && !found; j++ )
        {
            int typeNameLength = typeElements[j].length();
            Type type = static_cast<Type>(j);
            if( s.mid( i, typeNameLength ) == typeElements[j] )
            {
                m_dropTarget->appendToken( createToken( type ) );
                i += typeNameLength;
                found = true;
            }
        }

        if( !found )
        {
            debug() << "'" << s.at(i) << "' can't be represented as TokenLayoutWidget Token";
            ++i; // skip junk
        }
    }
}

void
FilenameLayoutWidget::populateConfiguration()
{
    QString mode = Amarok::config( m_configCategory ).readEntry( "Mode" );
    setAdvancedMode( mode == QLatin1String( "Advanced" ) );

    // Custom scheme is stored per dialog
    QString custom = Amarok::config( m_configCategory ).readEntryUntranslated( "Custom Scheme" );
    debug() << "--- got custom scheme for" << m_configCategory << custom;

    populateFormatList( custom );

    setScheme( custom );
}

void
FilenameLayoutWidget::populateFormatList( const QString& custom )
{
    DEBUG_BLOCK

    // Configuration is not symmetric: dialog-specific settings are saved
    // using m_configCategory, that is different per dialog. The presets are saved
    // only in one single place, so these can be shared. This place is the "default" one,
    // that is the configuration for OrganizeCollectionDialog.

    // items are stored in the config list in the following format:
    // Label#DELIM#format string
    QStringList presets_raw;
    int selected_index = -1;
    m_presetCombo->clear();
    presets_raw = AmarokConfig::formatPresets(); // Always use the one in OrganizeCollectionDialog
    // presets_raw = Amarok::config( m_configCategory ).readEntry( QString::fromLatin1( "Format Presets" ), QStringList() );

    debug() << "--- got presets" << presets_raw;

    for( const QString &str : presets_raw )
    {
        QStringList items;
        items = str.split( QStringLiteral("#DELIM#"), Qt::SkipEmptyParts );
        if( items.size() < 2 )
            continue;
        m_presetCombo->addItem( items.at( 0 ), items.at( 1 ) ); // Label, format string
        if( items.at( 1 ) == custom )
            selected_index = m_presetCombo->findData( items.at( 1 ) );
    }

    if( selected_index >= 0 )
        m_presetCombo->setCurrentIndex( selected_index );

    connect( m_presetCombo, QOverload<int>::of(&QComboBox::activated),
             this, &FilenameLayoutWidget::slotFormatPresetSelected );
    connect( m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
             this, &FilenameLayoutWidget::slotFormatPresetSelected );
}

void
FilenameLayoutWidget::saveFormatList() const
{
    DEBUG_BLOCK

    QStringList presets_raw;
    int n = m_presetCombo->count();

    for( int i = 0; i < n; ++i )
    {
        QString item = QStringLiteral("%1#DELIM#%2");
        QString scheme = m_presetCombo->itemData( i ).toString();
        QString label = m_presetCombo->itemText( i );
        item = item.arg( label, scheme );
        presets_raw.append( item );
    }

   debug() << "--- saving presets" << presets_raw;
   AmarokConfig::setFormatPresets( presets_raw ); // Always use the one in OrganizeCollectionDialog
   // Amarok::config( m_configCategory ).writeEntry( QString::fromLatin1( "Format Presets" ), presets_raw );
}

void
FilenameLayoutWidget::slotUpdatePresetButton()
{
    QString comboScheme = m_presetCombo->itemData( m_presetCombo->currentIndex() ).toString();
    m_updatePresetButton->setEnabled( comboScheme != getParsableScheme() );
}

void
FilenameLayoutWidget::slotFormatPresetSelected( int index )
{
    QString scheme = m_presetCombo->itemData( index ).toString();
    setScheme( scheme );
}

void
FilenameLayoutWidget::slotAddFormat()
{
    bool ok = false;
    QString name = QInputDialog::getText( this, i18n( "New Preset" ), i18n( "Preset Name" ), QLineEdit::Normal, i18n( "New Preset" ), &ok );
    if( !ok )
        return; // user canceled.

    QString format = getParsableScheme();
    m_presetCombo->addItem( name, format );
    m_presetCombo->setCurrentIndex( m_presetCombo->count() - 1 );

    saveFormatList();
}

void
FilenameLayoutWidget::slotRemoveFormat()
{
    int idx = m_presetCombo->currentIndex();
    m_presetCombo->removeItem( idx );

    saveFormatList();
}

void
FilenameLayoutWidget::slotUpdateFormat()
{
    int idx = m_presetCombo->currentIndex();
    QString formatString = getParsableScheme();
    m_presetCombo->setItemData( idx, formatString );
    m_updatePresetButton->setEnabled( false );

    saveFormatList();
}
