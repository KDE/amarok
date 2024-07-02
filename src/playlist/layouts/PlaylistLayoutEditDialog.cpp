/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Oleksandr Khayrullin <saniokh@gmail.com>                          *
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

#include "PlaylistLayoutEditDialog.h"

#include "core/support/Debug.h"

#include "playlist/layouts/LayoutManager.h"
#include "playlist/PlaylistDefines.h"

#include <KMessageBox>

#include <QInputDialog>
#include <QLineEdit>

Playlist::PlaylistLayoutEditDialog::PlaylistLayoutEditDialog( QWidget *parent )
    : QDialog( parent )
{
    setupUi( this );

    // -- add tokens to the token pool
    Column tokenValues[] = {
        Album,
        AlbumArtist,
        Artist,
        Bitrate,
        Bpm,
        Comment,
        Composer,
        Directory,
        DiscNumber,
        Divider,
        Filename,
        Filesize,
        Genre,
        GroupLength,
        GroupTracks,
        LastPlayed,
        Labels,
        Length,
        Moodbar,
        PlaceHolder,
        PlayCount,
        Rating,
        SampleRate,
        Score,
        Source,
        Title,
        TitleWithTrackNum,
        TrackNumber,
        Type,
        Year };

    for( uint i = 0; i < sizeof( tokenValues ) / sizeof( tokenValues[0] ); i++ )
        tokenPool->addToken( new Token( columnName( tokenValues[i] ),
                                        iconName( tokenValues[i] ),
                                        static_cast<qint64>(tokenValues[i]) ) );

    m_firstActiveLayout = LayoutManager::instance()->activeLayoutName();

    //add an editor to each tab
    for( int part = 0; part < PlaylistLayout::NumParts; part++ )
        m_partsEdit[part] = new Playlist::LayoutEditWidget( this );
    m_layoutsMap = new QMap<QString, PlaylistLayout>();

    elementTabs->addTab( m_partsEdit[PlaylistLayout::Head], i18n( "Head" ) );
    elementTabs->addTab( m_partsEdit[PlaylistLayout::StandardBody], i18n( "Body" ) );
    elementTabs->addTab( m_partsEdit[PlaylistLayout::VariousArtistsBody], i18n( "Body (Various artists)" ) );
    elementTabs->addTab( m_partsEdit[PlaylistLayout::Single], i18n( "Single" ) );

    QStringList layoutNames = LayoutManager::instance()->layouts();
    for( const QString &layoutName : layoutNames )
    {
        PlaylistLayout layout = LayoutManager::instance()->layout( layoutName );
        layout.setDirty( false );
        m_layoutsMap->insert( layoutName, layout );
    }

    layoutListWidget->addItems( layoutNames );

    layoutListWidget->setCurrentRow( LayoutManager::instance()->layouts().indexOf( LayoutManager::instance()->activeLayoutName() ) );

    setupGroupByCombo();

    if ( layoutListWidget->currentItem() )
        setLayout( layoutListWidget->currentItem()->text() );

    connect( previewButton, &QAbstractButton::clicked, this, &PlaylistLayoutEditDialog::preview );
    connect( layoutListWidget, &QListWidget::currentTextChanged, this, &PlaylistLayoutEditDialog::setLayout );
    connect( layoutListWidget, &QListWidget::currentRowChanged, this, &PlaylistLayoutEditDialog::toggleEditButtons );
    connect( layoutListWidget, &QListWidget::currentRowChanged, this, &PlaylistLayoutEditDialog::toggleUpDownButtons );

    connect( moveUpButton, &QAbstractButton::clicked, this, &PlaylistLayoutEditDialog::moveUp );
    connect( moveDownButton, &QAbstractButton::clicked, this, &PlaylistLayoutEditDialog::moveDown );

    buttonBox->button(QDialogButtonBox::Apply)->setIcon( QIcon::fromTheme( QStringLiteral("dialog-ok-apply") ) );
    buttonBox->button(QDialogButtonBox::Ok)->setIcon( QIcon::fromTheme( QStringLiteral("dialog-ok") ) );
    buttonBox->button(QDialogButtonBox::Cancel)->setIcon( QIcon::fromTheme( QStringLiteral("dialog-cancel") ) );
    connect( buttonBox->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &PlaylistLayoutEditDialog::apply );

    const QIcon newIcon = QIcon::fromTheme( QStringLiteral("document-new") );
    newLayoutButton->setIcon( newIcon );
    newLayoutButton->setToolTip( i18n( "New playlist layout" ) );
    connect( newLayoutButton, &QAbstractButton::clicked, this, &PlaylistLayoutEditDialog::newLayout );

    const QIcon copyIcon = QIcon::fromTheme( QStringLiteral("edit-copy") );
    copyLayoutButton->setIcon( copyIcon );
    copyLayoutButton->setToolTip( i18n( "Copy playlist layout" ) );
    connect( copyLayoutButton, &QAbstractButton::clicked, this, &PlaylistLayoutEditDialog::copyLayout );

    const QIcon deleteIcon = QIcon::fromTheme( QStringLiteral("edit-delete") );
    deleteLayoutButton->setIcon( deleteIcon );
    deleteLayoutButton->setToolTip( i18n( "Delete playlist layout" ) );
    connect( deleteLayoutButton, &QAbstractButton::clicked, this, &PlaylistLayoutEditDialog::deleteLayout );

    const QIcon renameIcon = QIcon::fromTheme( QStringLiteral("edit-rename") );
    renameLayoutButton->setIcon( renameIcon );
    renameLayoutButton->setToolTip( i18n( "Rename playlist layout" ) );
    connect( renameLayoutButton, &QAbstractButton::clicked, this, &PlaylistLayoutEditDialog::renameLayout );

    toggleEditButtons();
    toggleUpDownButtons();

    for( int part = 0; part < PlaylistLayout::NumParts; part++ )
        connect( m_partsEdit[part], &Playlist::LayoutEditWidget::changed, this, &PlaylistLayoutEditDialog::setLayoutChanged );

    connect( inlineControlsChekbox, &QCheckBox::stateChanged, this, &PlaylistLayoutEditDialog::setLayoutChanged );
    connect( tooltipsCheckbox, &QCheckBox::stateChanged, this, &PlaylistLayoutEditDialog::setLayoutChanged );
    connect( groupByComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
             this, &PlaylistLayoutEditDialog::setLayoutChanged );
}


Playlist::PlaylistLayoutEditDialog::~PlaylistLayoutEditDialog()
{
}

void
Playlist::PlaylistLayoutEditDialog::newLayout()      //SLOT
{
    bool ok;
    QString layoutName = QInputDialog::getText( this, i18n( "Choose a name for the new playlist layout" ),
                                                i18n( "Please enter a name for the playlist layout you are about to define:" ),
                                                QLineEdit::Normal, QString(), &ok );
    if( !ok )
	return;
    if( layoutName.isEmpty() )
    {
        KMessageBox::error( this, i18n( "Cannot create a layout with no name." ), i18n( "Layout name error" ) );
        return;
    }
    if( m_layoutsMap->keys().contains( layoutName ) )
    {
        KMessageBox::error( this, i18n( "Cannot create a layout with the same name as an existing layout." ), i18n( "Layout name error" ) );
        return;
    }
    if( layoutName.contains( QLatin1Char('/') ) )
    {
        KMessageBox::error( this, i18n( "Cannot create a layout containing '/'." ), i18n( "Layout name error" ) );
        return;
    }

    PlaylistLayout layout;
    layout.setEditable( true );      //Should I use true, TRUE or 1?
    layout.setDirty( true );

    layoutListWidget->addItem( layoutName );
    layoutListWidget->setCurrentItem( (layoutListWidget->findItems( layoutName, Qt::MatchExactly ) ).first() );
    for( int part = 0; part < PlaylistLayout::NumParts; part++ )
    {
        m_partsEdit[part]->clear();
        layout.setLayoutForPart( (PlaylistLayout::Part)part, m_partsEdit[part]->config() );
    }
    m_layoutsMap->insert( layoutName, layout );

    LayoutManager::instance()->addUserLayout( layoutName, layout );

    setLayout( layoutName );
}

void
Playlist::PlaylistLayoutEditDialog::copyLayout()
{
    LayoutItemConfig configs[PlaylistLayout::NumParts];
    for( int part = 0; part < PlaylistLayout::NumParts; part++ )
        configs[part] = m_partsEdit[part]->config();

    QString layoutName = layoutListWidget->currentItem()->text();

    bool ok;
    layoutName = QInputDialog::getText( this,
                                        i18n( "Choose a name for the new playlist layout" ),
                                        i18n( "Please enter a name for the playlist layout you are about to define as copy of the layout '%1':", layoutName ),
                                        QLineEdit::Normal, layoutName, &ok );

    if( !ok)
        return;
    if( layoutName.isEmpty() )
    {
        KMessageBox::error( this, i18n( "Cannot create a layout with no name." ), i18n( "Layout name error" ) );
        return;
    }
    if( m_layoutsMap->keys().contains( layoutName ) )
    {
        KMessageBox::error( this, i18n( "Cannot create a layout with the same name as an existing layout." ), i18n( "Layout name error" ) );
        return;
    }
    //layoutListWidget->addItem( layoutName );
    PlaylistLayout layout;
    layout.setEditable( true );      //Should I use true, TRUE or 1?
    layout.setDirty( true );

    configs[PlaylistLayout::Head].setActiveIndicatorRow( -1 );
    for( int part = 0; part < PlaylistLayout::NumParts; part++ )
        layout.setLayoutForPart( (PlaylistLayout::Part)part, configs[part] );

    layout.setInlineControls( inlineControlsChekbox->isChecked() );
    layout.setTooltips( tooltipsCheckbox->isChecked() );
    layout.setGroupBy( groupByComboBox->itemData( groupByComboBox->currentIndex() ).toString() );

    LayoutManager::instance()->addUserLayout( layoutName, layout );

    //reload from manager:
    layoutListWidget->clear();
    layoutListWidget->addItems( LayoutManager::instance()->layouts() );

    m_layoutsMap->insert( layoutName, layout );
    layoutListWidget->setCurrentItem( ( layoutListWidget->findItems( layoutName, Qt::MatchExactly ) ).first() );
    setLayout( layoutName );
}

void
Playlist::PlaylistLayoutEditDialog::deleteLayout()   //SLOT
{
    m_layoutsMap->remove( layoutListWidget->currentItem()->text() );
    if( LayoutManager::instance()->layouts().contains( layoutListWidget->currentItem()->text() ) )  //if the layout is already saved in the LayoutManager
        LayoutManager::instance()->deleteLayout( layoutListWidget->currentItem()->text() );         //delete it
    delete layoutListWidget->currentItem();
}

void
Playlist::PlaylistLayoutEditDialog::renameLayout()
{
    PlaylistLayout layout = m_layoutsMap->value( layoutListWidget->currentItem()->text() );

    QString layoutName;
    while( layoutName.isEmpty() || m_layoutsMap->keys().contains( layoutName ) )
    {
        bool ok;
        layoutName = QInputDialog::getText( this,
                                            i18n( "Choose a new name for the playlist layout" ),
                                            i18n( "Please enter a new name for the playlist layout you are about to rename:" ),
                                            QLineEdit::Normal, layoutListWidget->currentItem()->text(), &ok );
        if ( !ok )
        {
            //Cancelled so just return
            return;
        }
        if( layoutName.isEmpty() )
            KMessageBox::error( this, i18n( "Cannot rename a layout to have no name." ), i18n( "Layout name error" ) );
        if( m_layoutsMap->keys().contains( layoutName ) )
            KMessageBox::error( this, i18n( "Cannot rename a layout to have the same name as an existing layout." ), i18n( "Layout name error" ) );
    }
    m_layoutsMap->remove( layoutListWidget->currentItem()->text() );
    if( LayoutManager::instance()->layouts().contains( layoutListWidget->currentItem()->text() ) )  //if the layout is already saved in the LayoutManager
        LayoutManager::instance()->deleteLayout( layoutListWidget->currentItem()->text() );         //delete it
    LayoutManager::instance()->addUserLayout( layoutName, layout );
    m_layoutsMap->insert( layoutName, layout );
    layoutListWidget->currentItem()->setText( layoutName );

    setLayout( layoutName );
}

void
Playlist::PlaylistLayoutEditDialog::setLayout( const QString &layoutName )   //SLOT
{
    DEBUG_BLOCK
    m_layoutName = layoutName;

    if( m_layoutsMap->keys().contains( layoutName ) )   //is the layout exists in the list of loaded layouts
    {
        debug() << "loaded layout";
        PlaylistLayout layout = m_layoutsMap->value( layoutName );
        for( int part = 0; part < PlaylistLayout::NumParts; part++ )
            m_partsEdit[part]->readLayout( layout.layoutForPart( (PlaylistLayout::Part)part ) );
        inlineControlsChekbox->setChecked( layout.inlineControls() );
        tooltipsCheckbox->setChecked( layout.tooltips() );
        groupByComboBox->setCurrentIndex( groupByComboBox->findData( layout.groupBy() ) );

        setEnabledTabs();
        //make sure that it is not marked dirty (it will be because of the changed signal triggering when loading it)
        //unless it is actually changed
        debug() << "not dirty anyway!!";
        (*m_layoutsMap)[m_layoutName].setDirty( false );
    }
    else
    {
        for( int part = 0; part < PlaylistLayout::NumParts; part++ )
            m_partsEdit[part]->clear();
    }
}

void
Playlist::PlaylistLayoutEditDialog::preview()
{
    PlaylistLayout layout;

    for( int part = 0; part < PlaylistLayout::NumParts; part++ )
    {
        LayoutItemConfig config = m_partsEdit[part]->config();
        if( part == PlaylistLayout::Head )
            config.setActiveIndicatorRow( -1 );
        layout.setLayoutForPart( (PlaylistLayout::Part)part, config );
    }

    layout.setInlineControls( inlineControlsChekbox->isChecked() );
    layout.setTooltips( tooltipsCheckbox->isChecked() );
    layout.setGroupBy( groupByComboBox->itemData( groupByComboBox->currentIndex() ).toString() );

    LayoutManager::instance()->setPreviewLayout( layout );
}

void
Playlist::PlaylistLayoutEditDialog::toggleEditButtons() //SLOT
{
    if ( !layoutListWidget->currentItem() ) {
        deleteLayoutButton->setEnabled( 0 );
        renameLayoutButton->setEnabled( 0 );
    } else if( LayoutManager::instance()->isDefaultLayout( layoutListWidget->currentItem()->text() ) ) {
        deleteLayoutButton->setEnabled( 0 );
        renameLayoutButton->setEnabled( 0 );
    } else {
        deleteLayoutButton->setEnabled( 1 );
        renameLayoutButton->setEnabled( 1 );
    }
}

void
Playlist::PlaylistLayoutEditDialog::toggleUpDownButtons()
{
    if ( !layoutListWidget->currentItem() )
    {
        moveUpButton->setEnabled( 0 );
        moveDownButton->setEnabled( 0 );
    }
    else if ( layoutListWidget->currentRow() == 0 )
    {
        moveUpButton->setEnabled( 0 );
        if ( layoutListWidget->currentRow() >= m_layoutsMap->size() -1 )
            moveDownButton->setEnabled( 0 );
        else
            moveDownButton->setEnabled( 1 );
    }
    else if ( layoutListWidget->currentRow() >= m_layoutsMap->size() -1 )
    {
        moveDownButton->setEnabled( 0 );
        moveUpButton->setEnabled( 1 ); //we already checked that this is not row 0
    }
    else
    {
        moveDownButton->setEnabled( 1 );
        moveUpButton->setEnabled( 1 );
    }


}

void
Playlist::PlaylistLayoutEditDialog::apply()  //SLOT
{
    for( const QString &layoutName : m_layoutsMap->keys() )
    {
        PlaylistLayout layout = m_layoutsMap->value( layoutName );

        if( layout.isDirty() )
        {
            // search a new name for changed default layouts
            if( LayoutManager::instance()->isDefaultLayout( layoutName ) )
            {
                QString newLayoutName = i18n( "copy of %1", layoutName );
                QString orgCopyName = newLayoutName;

                int copyNumber = 1;
                QStringList existingLayouts = LayoutManager::instance()->layouts();
                while( existingLayouts.contains( newLayoutName ) )
                {
                    copyNumber++;
                    newLayoutName = i18nc( "adds a copy number to a generated name if the name already exists, for instance 'copy of Foo 2' if 'copy of Foo' is taken", "%1 %2", orgCopyName, copyNumber );
                }

                const QString msg = i18n( "The layout '%1' you modified is one of the default layouts and cannot be overwritten. "
                                          "Saved as new layout '%2'", layoutName, newLayoutName );
                KMessageBox::error( this, msg, i18n( "Default Layout" ) );


                layout.setDirty( false );
                m_layoutsMap->insert( newLayoutName, layout );
                LayoutManager::instance()->addUserLayout( newLayoutName, layout );
                layoutListWidget->addItem( newLayoutName );

                if( layoutName == m_layoutName )
                    layoutListWidget->setCurrentItem( ( layoutListWidget->findItems( newLayoutName, Qt::MatchExactly ) ).first() );

                // restore the default layout
                m_layoutsMap->insert( layoutName, LayoutManager::instance()->layout( layoutName ) );
            }
            else
            {
                layout.setDirty( false );

                m_layoutsMap->insert( layoutName, layout );
                LayoutManager::instance()->addUserLayout( layoutName, layout );
            }
        }
    }
    LayoutManager::instance()->setActiveLayout( layoutListWidget->currentItem()->text() );  //important to override the previewed layout if preview is used
}

void
Playlist::PlaylistLayoutEditDialog::accept()     //SLOT
{
    apply();
    QDialog::accept();
}

void
Playlist::PlaylistLayoutEditDialog::reject()     //SLOT
{
    DEBUG_BLOCK

    debug() << "Applying initial layout: " << m_firstActiveLayout;
    if( layoutListWidget->findItems( m_firstActiveLayout, Qt::MatchExactly ).isEmpty() )
        LayoutManager::instance()->setActiveLayout( QStringLiteral("Default") );
    else
        LayoutManager::instance()->setActiveLayout( m_firstActiveLayout );

    QDialog::reject();
}

void
Playlist::PlaylistLayoutEditDialog::moveUp()
{
    int newRow = LayoutManager::instance()->moveUp( m_layoutName );

    layoutListWidget->clear();
    layoutListWidget->addItems( LayoutManager::instance()->layouts() );

    layoutListWidget->setCurrentRow( newRow );
}

void
Playlist::PlaylistLayoutEditDialog::moveDown()
{
    int newRow = LayoutManager::instance()->moveDown( m_layoutName );

    layoutListWidget->clear();
    layoutListWidget->addItems( LayoutManager::instance()->layouts() );

    layoutListWidget->setCurrentRow( newRow );
}

void
Playlist::PlaylistLayoutEditDialog::setEnabledTabs()
{
    DEBUG_BLOCK

    //Enable or disable tabs depending on whether grouping is allowed.
    QString grouping = groupByComboBox->itemData( groupByComboBox->currentIndex() ).toString();
    bool groupingEnabled = ( !grouping.isEmpty() && grouping != QLatin1String("None") );

    if ( !groupingEnabled )
        elementTabs->setCurrentWidget( m_partsEdit[PlaylistLayout::Single] );

    debug() << groupByComboBox->itemData( groupByComboBox->currentIndex() ).toString();
    debug() << groupingEnabled;

    elementTabs->setTabEnabled( elementTabs->indexOf( m_partsEdit[PlaylistLayout::Head] ), groupingEnabled );
    elementTabs->setTabEnabled( elementTabs->indexOf( m_partsEdit[PlaylistLayout::StandardBody] ), groupingEnabled );
    elementTabs->setTabEnabled( elementTabs->indexOf( m_partsEdit[PlaylistLayout::VariousArtistsBody] ), groupingEnabled );
}

//Sets up a combo box that presents the possible grouping categories, as well as the option
//to perform no grouping.
//We'll use the "user data" to store the un-i18n-ized category name for internal use.
void
Playlist::PlaylistLayoutEditDialog::setupGroupByCombo()
{
    for( const Playlist::Column &col : Playlist::groupableCategories() )
    {
        groupByComboBox->addItem( QIcon::fromTheme( iconName( col ) ),
                                  columnName( col ),
                                  QVariant( internalColumnName( col ) ) );
    }

    //Add the option to not perform grouping
    //Use a null string to specify "no grouping"
    groupByComboBox->addItem( i18n( "No Grouping" ), QVariant( QStringLiteral("None") ) );
}

void
Playlist::PlaylistLayoutEditDialog::setLayoutChanged()
{
    DEBUG_BLOCK

    setEnabledTabs();

    for( int part = 0; part < PlaylistLayout::NumParts; part++ )
        (*m_layoutsMap)[m_layoutName].setLayoutForPart( (PlaylistLayout::Part)part, m_partsEdit[part]->config() );

    (*m_layoutsMap)[m_layoutName].setInlineControls( inlineControlsChekbox->isChecked() );
    (*m_layoutsMap)[m_layoutName].setTooltips( tooltipsCheckbox->isChecked() );
    (*m_layoutsMap)[m_layoutName].setGroupBy( groupByComboBox->itemData( groupByComboBox->currentIndex() ).toString() );
    (*m_layoutsMap)[m_layoutName].setDirty( true );
}


