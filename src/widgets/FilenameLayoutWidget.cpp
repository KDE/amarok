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

#include <KInputDialog>
#include <KLineEdit>
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
<< QLatin1String("_") );


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
    m_addPresetButton->setToolTip( i18n("Saves the current scheme/format above as a preset.", 0));
    presetLayout1->addWidget( m_addPresetButton, 0 );

    m_updatePresetButton = new QPushButton( i18n("Update preset"), this );
    presetLayout1->addWidget( m_updatePresetButton, 0 );

    m_removePresetButton = new QPushButton( i18n("Remove preset"), this );
    m_removePresetButton->setToolTip( i18n("Removes the currently selected format preset") );
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

    m_filenameLayoutEdit = new KLineEdit( this );
    advancedLayout->addWidget( m_filenameLayoutEdit );

    m_schemeStack->addWidget( advancedLayoutWidget );

    schemeGroupLayout->addWidget( m_schemeStack );

    m_advancedButton = new QPushButton( i18n("Advanced"), this );
    schemeGroupLayout->addWidget( m_advancedButton );

    // --

    m_mainLayout->addWidget( schemeGroup );

    connect( m_tokenPool, SIGNAL( onDoubleClick( Token * ) ),
             m_dropTarget, SLOT( insertToken( Token* ) ) );
    connect( m_advancedButton, SIGNAL( clicked() ),
             this, SLOT( toggleAdvancedMode() ) );
    connect( m_dropTarget, SIGNAL( changed() ),
             this, SIGNAL( schemeChanged() ) );
    connect( m_dropTarget, SIGNAL( changed() ),
             this, SLOT( slotUpdatePresetButton() ) );
    connect( m_addPresetButton, SIGNAL( clicked( bool ) ),
             this, SLOT( slotAddFormat() ) );
    connect( m_removePresetButton, SIGNAL( clicked( bool ) ),
             this, SLOT( slotRemoveFormat() ) );
    connect( m_updatePresetButton, SIGNAL( clicked( bool ) ),
             this, SLOT( slotUpdateFormat() ) );

    connect( m_filenameLayoutEdit, SIGNAL( textChanged( const QString & ) ),
             this, SIGNAL( schemeChanged() ) );
debug() << "st3.1";
}

Token*
FilenameLayoutWidget::createToken(qint64 value) const
{
    struct TokenDefinition
    {
        QString name;
        const char* iconName;
        qint64 value;
    };

    static const TokenDefinition tokenDefinitions[] = {
        { i18n( "Track number" ),"filename-track-amarok", TrackNumber },
        { i18n( "Title" ), "filename-title-amarok", Title },
        { i18n( "Artist" ), "filename-artist-amarok", Artist },
        { i18n( "Composer" ), "filename-composer-amarok", Composer },
        { i18n( "Year" ), "filename-year-amarok", Year },
        { i18n( "Album" ), "filename-album-amarok", Album },
        { i18n( "Album Artist" ), "filename-artist-amarok", AlbumArtist },
        { i18n( "Comment" ), "filename-comment-amarok", Comment },
        { i18n( "Genre" ), "filename-genre-amarok", Genre },

        { i18n( "File type" ), "filename-filetype-amarok", FileType },
        { i18n( "Ignore" ), "filename-ignore-amarok", Ignore },
        { i18n( "Folder" ), "filename-folder-amarok", Folder },
        { i18nc( "Artist's Initial", "Initial" ), "filename-initial-amarok", Initial },
        { i18n( "Disc number" ), "filename-discnumber-amarok", DiscNumber },

        { "/", "filename-slash-amarok", Slash },
        { "_", "filename-underscore-amarok", Underscore },
        { "-", "filename-dash-amarok", Dash },
        { ".", "filename-dot-amarok", Dot },
        { " ", "filename-space-amarok", Space },
        { i18n( "Collection root" ), "collection-amarok", CollectionRoot },
        { QString(), 0, Space }
    };

    for( int i = 0; !tokenDefinitions[i].name.isNull(); ++i )
    {
        if( value == tokenDefinitions[i].value )
        {
            return new Token( tokenDefinitions[i].name,
                              tokenDefinitions[i].iconName,
                              tokenDefinitions[i].value );
        }
    }

    return 0;
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
    slotSaveFormatList();
}

QString
FilenameLayoutWidget::getParsableScheme() const
{
    QString scheme = m_advancedMode ? m_filenameLayoutEdit->text() : dropTargetScheme();

    Amarok::config( m_configCategory ).writeEntry( "Custom Scheme", scheme );
    return scheme;
}

// attempts to set the scheme
void FilenameLayoutWidget::setScheme(const QString& scheme)
{
    m_filenameLayoutEdit->setText( scheme );
    inferScheme( scheme );

    slotUpdatePresetButton();
    emit schemeChanged();
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

    QString entryValue  = m_advancedMode ? "Advanced" : "Basic";

    Amarok::config( m_configCategory ).writeEntry( "Mode", entryValue );
}


QString
FilenameLayoutWidget::dropTargetScheme() const
{
    QString parsableScheme = "";

    QList< Token *> list = m_dropTarget->tokensAtRow();

    foreach( Token *token, list )
    {
        parsableScheme += typeElements[token->value()];
    }

    return parsableScheme;
}

void
FilenameLayoutWidget::inferScheme( const QString &s ) //SLOT
{
    DEBUG_BLOCK

    debug() << "infering scheme: " << s;

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
            if( s.midRef( i, typeNameLength ) == typeElements[j] )
            {
                m_dropTarget->insertToken( createToken( type ) );
                i += typeNameLength;
                found = true;
            }
        }

        if( !found )
        {
            ++i; // skip junk
            debug() << "'" << s.at(i) << "' can't be represented as TokenLayoutWidget Token";
        }
    }
}

void
FilenameLayoutWidget::populateConfiguration()
{
    QString mode = Amarok::config( m_configCategory ).readEntry( "Mode" );
    setAdvancedMode( mode == QLatin1String( "Advanced" ) );

    setScheme( Amarok::config( m_configCategory ).readEntryUntranslated( "Custom Scheme" ) );

    populateFormatList();
}


void
FilenameLayoutWidget::populateFormatList()
{
    DEBUG_BLOCK

    // items are stored in the config list in the following format:
    // Label#DELIM#format string#DELIM#selected
    // the last item to have the third parameter is the default selected preset
    // the third param isnis optional
    QStringList presets_raw;
    int selected_index = -1;
    m_presetCombo->clear();
    presets_raw = AmarokConfig::formatPresets();
    // presets_raw = Amarok::config( m_configCategory ).readEntry( QString::fromLatin1( "Format Presets" ), QStringList() );

    debug() << "--- got preset for" << m_configCategory << presets_raw;

    foreach( QString str, presets_raw )
    {
        QStringList items;
        items = str.split( "#DELIM#", QString::SkipEmptyParts );
        if( items.size() < 2 )
            continue;
        m_presetCombo->addItem( items.at( 0 ), items.at( 1 ) ); // Label, format string
        if( items.size() == 3 )
            selected_index = m_presetCombo->findData( items.at( 1 ) );
    }

    if( selected_index > 0 )
        m_presetCombo->setCurrentIndex( selected_index );

    slotFormatPresetSelected( selected_index );
    connect( m_presetCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( slotFormatPresetSelected( int ) ) );
}

void
FilenameLayoutWidget::slotUpdatePresetButton()
{
    QString comboScheme = m_presetCombo->itemData( m_presetCombo->currentIndex() ).  toString();
    m_updatePresetButton->setEnabled( comboScheme != getParsableScheme() );
}

void
FilenameLayoutWidget::slotSaveFormatList()
{
    if( !m_formatListModified )
        return;

    QStringList presets;
    int n = m_presetCombo->count();
    int current_idx = m_presetCombo->currentIndex();

    for( int i = 0; i < n; ++i )
    {
        QString item;
        if( i == current_idx )
            item = "%1#DELIM#%2#DELIM#selected";
        else
            item = "%1#DELIM#%2";

        QString scheme = m_presetCombo->itemData( i ).toString();
        QString label = m_presetCombo->itemText( i );
        item = item.arg( label, scheme );
        presets.append( item );
    }

   Amarok::config( m_configCategory ).writeEntry( QString::fromLatin1( "Format Presets" ), presets );
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
    QString name = KInputDialog::getText( i18n( "New Format Preset" ), i18n( "Preset Name" ), i18n( "New Preset" ),  &ok, this );
    if( !ok )
        return; // user canceled.

    QString format = getParsableScheme();
    m_presetCombo->insertItem(0, name, format);
    m_presetCombo->setCurrentIndex( 0 );
    m_formatListModified = true;
}

void
FilenameLayoutWidget::slotRemoveFormat()
{
    int idx = m_presetCombo->currentIndex();
    m_presetCombo->removeItem( idx );
    m_formatListModified = true;
}

void
FilenameLayoutWidget::slotUpdateFormat()
{
    int idx = m_presetCombo->currentIndex();
    QString formatString = getParsableScheme();
    m_presetCombo->setItemData( idx, formatString );
    m_updatePresetButton->setEnabled( false );
    m_formatListModified = true;
}
