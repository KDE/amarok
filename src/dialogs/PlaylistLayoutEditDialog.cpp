/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2009  Teo Mrnjavac <teo.mrnjavac@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "PlaylistLayoutEditDialog.h"

#include "Debug.h"

#include "playlist/layouts/LayoutManager.h"
#include "playlist/PlaylistDefines.h"

#include <KMessageBox>

#include <QInputDialog>

using namespace Playlist;

/**
 * Constructor for PlaylistLayoutEditDialog.
 * Populates the token pool, loads the available layouts from the LayoutManager in the right area and loads the configuration of the currently active layout.
 */
PlaylistLayoutEditDialog::PlaylistLayoutEditDialog( QWidget *parent )
    : QDialog( parent )
{
    setupUi( this );

    tokenPool->addToken( new Token( columnNames[Album], iconNames[Album], Album ) );
    tokenPool->addToken( new Token( columnNames[AlbumArtist], iconNames[AlbumArtist], AlbumArtist ) );
    tokenPool->addToken( new Token( columnNames[Artist], iconNames[Artist], Artist ) );
    tokenPool->addToken( new Token( columnNames[Bitrate], iconNames[Bitrate], Bitrate ) );
    tokenPool->addToken( new Token( columnNames[Comment], iconNames[Comment], Comment ) );
    tokenPool->addToken( new Token( columnNames[Composer], iconNames[Composer], Composer ) );
    tokenPool->addToken( new Token( columnNames[DiscNumber], iconNames[DiscNumber], DiscNumber ) );
    tokenPool->addToken( new Token( columnNames[Directory], iconNames[Directory], Directory ) );
    tokenPool->addToken( new Token( columnNames[Divider], iconNames[Divider], Divider ) );
    tokenPool->addToken( new Token( columnNames[Filename], iconNames[Filename], Filename ) );
    tokenPool->addToken( new Token( columnNames[Filesize], iconNames[Filesize], Filesize ) );
    tokenPool->addToken( new Token( columnNames[GroupLength], iconNames[GroupLength], GroupLength ) );
    tokenPool->addToken( new Token( columnNames[GroupTracks], iconNames[GroupTracks], GroupTracks ) );
    tokenPool->addToken( new Token( columnNames[LastPlayed], iconNames[LastPlayed], LastPlayed ) );
    tokenPool->addToken( new Token( columnNames[Length], iconNames[Length], Length ) );
    tokenPool->addToken( new Token( columnNames[PlaceHolder], iconNames[PlaceHolder], PlaceHolder ) );
    tokenPool->addToken( new Token( columnNames[PlayCount], iconNames[PlayCount], PlayCount ) );
    tokenPool->addToken( new Token( columnNames[Rating], iconNames[Rating], Rating ) );
    tokenPool->addToken( new Token( columnNames[SampleRate], iconNames[SampleRate], SampleRate ) );
    tokenPool->addToken( new Token( columnNames[Score], iconNames[Score], Score ) );
    tokenPool->addToken( new Token( columnNames[Source], iconNames[Source], Source ) );
    tokenPool->addToken( new Token( columnNames[Title], iconNames[Title], Title ) );
    tokenPool->addToken( new Token( columnNames[TitleWithTrackNum], iconNames[TitleWithTrackNum], TitleWithTrackNum ) );
    tokenPool->addToken( new Token( columnNames[TrackNumber], iconNames[TrackNumber], TrackNumber ) );
    tokenPool->addToken( new Token( columnNames[Type], iconNames[Type], Type ) );
    tokenPool->addToken( new Token( columnNames[Year], iconNames[Year], Year ) );

    m_firstActiveLayout = LayoutManager::instance()->activeLayoutName();

    //add an editor to each tab
    m_headEdit = new Playlist::LayoutEditWidget( this );
    m_bodyEdit = new Playlist::LayoutEditWidget( this );
    m_singleEdit = new Playlist::LayoutEditWidget( this );
    m_layoutsMap = new QMap<QString, PlaylistLayout>();

    elementTabs->addTab( m_headEdit, i18n( "Head" ) );
    elementTabs->addTab( m_bodyEdit, i18n( "Body" ) );
    elementTabs->addTab( m_singleEdit, i18n( "Single" ) );

    elementTabs->removeTab( 0 );

    QStringList layoutNames = LayoutManager::instance()->layouts();
    foreach( QString layoutName, layoutNames )
    {
        PlaylistLayout layout = LayoutManager::instance()->layout( layoutName );
        layout.setDirty( false );
        m_layoutsMap->insert( layoutName, layout );
    }

    layoutListWidget->addItems( layoutNames );

    layoutListWidget->setCurrentRow( LayoutManager::instance()->layouts().indexOf( LayoutManager::instance()->activeLayoutName() ) );
    setLayout( layoutListWidget->currentItem()->text() );

    connect( previewButton, SIGNAL( clicked() ), this, SLOT( preview() ) );
    connect( layoutListWidget, SIGNAL( currentTextChanged( const QString & ) ), this, SLOT( setLayout( const QString & ) ) );
    connect( layoutListWidget, SIGNAL( currentRowChanged( int ) ), this, SLOT( toggleDeleteButton() ) );

    const KIcon newIcon( "list-add" );
    newLayoutButton->setIcon( newIcon );
    newLayoutButton->setToolTip( i18n( "New playlist layout" ) );
    connect( newLayoutButton, SIGNAL( clicked() ), this, SLOT( newLayout() ) );
    
    const KIcon copyIcon( "edit-copy" );
    copyLayoutButton->setIcon( copyIcon );
    copyLayoutButton->setToolTip( i18n( "Copy playlist layout" ) );
    connect( copyLayoutButton, SIGNAL( clicked() ), this, SLOT( copyLayout() ) );
    
    const KIcon deleteIcon( "edit-delete" );
    deleteLayoutButton->setIcon( deleteIcon );
    deleteLayoutButton->setToolTip( i18n( "Delete playlist layout" ) );
    connect( deleteLayoutButton, SIGNAL( clicked() ), this, SLOT( deleteLayout() ) );
    toggleDeleteButton();

    const KIcon renameIcon( "edit-rename" );
    renameLayoutButton->setIcon( renameIcon );
    renameLayoutButton->setToolTip( i18n( "Rename playlist layout" ) );
    connect( renameLayoutButton, SIGNAL( clicked() ), this, SLOT( renameLayout() ) );
}


PlaylistLayoutEditDialog::~PlaylistLayoutEditDialog()
{
}

/**
 * Creates a new PlaylistLayout with a given name and loads it in the right area to configure it.
 * The new layout is not saved in the LayoutManager but in m_layoutsMap.
 */
void PlaylistLayoutEditDialog::newLayout()      //SLOT
{
    QString layoutName( "" );
    while( layoutName == "" || m_layoutsMap->keys().contains( layoutName ) )
    {
        layoutName = QInputDialog::getText( this, i18n( "Choose a name for the new playlist layout" ),
                    i18n( "Please enter a name for the playlist layout you are about to define:" ) );
        if( layoutName == "" )
            KMessageBox::sorry( this, i18n( "Layout name error" ), i18n( "Can't create a layout with no name." ) );
        if( m_layoutsMap->keys().contains( layoutName ) )
            KMessageBox::sorry( this, i18n( "Layout name error" ), i18n( "Can't create a layout with the same name as an existing layout." ) );
    }
    debug() << "Creating new layout " << layoutName;
    layoutListWidget->addItem( layoutName );
    layoutListWidget->setCurrentItem( (layoutListWidget->findItems( layoutName, Qt::MatchExactly )).first() );
    PlaylistLayout layout;
    layout.setEditable( true );      //Should I use true, TRUE or 1?
    layout.setDirty( true );

    setLayout( layoutName );

    LayoutItemConfig headConfig = m_headEdit->config();
    headConfig.setActiveIndicatorRow( -1 );
    layout.setHead( headConfig );
    layout.setBody( m_bodyEdit->config() );
    layout.setSingle( m_singleEdit->config() );
    m_layoutsMap->insert( layoutName, layout );
}

/**
 * Creates a new PlaylistLayout with a given name as a copy of an existing layout and loads it in the right area to configure it.
 * The new layout is not saved in the LayoutManager but in m_layoutsMap.
 */
void PlaylistLayoutEditDialog::copyLayout()
{
    LayoutItemConfig headConfig = m_headEdit->config();
    LayoutItemConfig bodyConfig = m_bodyEdit->config();
    LayoutItemConfig singleConfig = m_singleEdit->config();
    
    QString layoutName( "" );
    while( layoutName == "" || m_layoutsMap->keys().contains( layoutName ) )
    {
        layoutName = QInputDialog::getText( this, i18n( "Choose a name for the new playlist layout" ),
                    i18n( "Please enter a name for the playlist layout you are about to define as copy of the layout '%1':",
                    layoutListWidget->currentItem()->text() ) );
        if( layoutName == "" )
            KMessageBox::sorry( this, i18n( "Can't create a layout with no name." ), i18n( "Layout name error" ) );
        if( m_layoutsMap->keys().contains( layoutName ) )
            KMessageBox::sorry( this, i18n( "Can't create a layout with the same name as an existing layout." ), i18n( "Layout name error" ) );
    }
    debug() << "Copying layout " << layoutName;
    layoutListWidget->addItem( layoutName );
    layoutListWidget->setCurrentItem( (layoutListWidget->findItems( layoutName, Qt::MatchExactly )).first() );
    PlaylistLayout layout;
    layout.setEditable( true );      //Should I use true, TRUE or 1?
    layout.setDirty( true );

    headConfig.setActiveIndicatorRow( -1 );
    layout.setHead( headConfig );
    layout.setBody( bodyConfig );
    layout.setSingle( singleConfig );
    m_layoutsMap->insert( layoutName, layout );

    setLayout( layoutName );
}

/**
 * Deletes the current layout selected in the layoutListWidget.
 */
void PlaylistLayoutEditDialog::deleteLayout()   //SLOT
{
    m_layoutsMap->remove( layoutListWidget->currentItem()->text() );
    if( LayoutManager::instance()->layouts().contains( layoutListWidget->currentItem()->text() ) )  //if the layout is already saved in the LayoutManager
        LayoutManager::instance()->deleteLayout( layoutListWidget->currentItem()->text() );         //delete it
    delete layoutListWidget->currentItem();
}

/**
 * Renames the current layout selected in the layoutListWidget.
 */
void PlaylistLayoutEditDialog::renameLayout()
{
    PlaylistLayout layout = m_layoutsMap->value( layoutListWidget->currentItem()->text() );
    
    QString layoutName( "" );
    while( layoutName == "" || m_layoutsMap->keys().contains( layoutName ) )
    {
        layoutName = QInputDialog::getText( this, i18n( "Choose a new name for the playlist layout" ),
                    i18n( "Please enter a new name for the playlist layout you are about to rename:" ) );
        if( LayoutManager::instance()->isDefaultLayout( layoutName ) )
        {
            KMessageBox::sorry( this, i18n( "Can't rename one of the default layouts." ), i18n( "Layout name error" ) );
            return;
        }
        if( layoutName == "" )
            KMessageBox::sorry( this, i18n( "Can't rename a layout with no name." ), i18n( "Layout name error" ) );
        if( m_layoutsMap->keys().contains( layoutName ) )
            KMessageBox::sorry( this, i18n( "Can't rename a layout with the same name as an existing layout." ), i18n( "Layout name error" ) );
    }
    debug() << "Renaming layout " << layoutName;
    m_layoutsMap->remove( layoutListWidget->currentItem()->text() );
    if( LayoutManager::instance()->layouts().contains( layoutListWidget->currentItem()->text() ) )  //if the layout is already saved in the LayoutManager
        LayoutManager::instance()->deleteLayout( layoutListWidget->currentItem()->text() );         //delete it
    LayoutManager::instance()->addUserLayout( layoutName, layout );
    m_layoutsMap->insert( layoutName, layout );
    layoutListWidget->currentItem()->setText( layoutName );

    setLayout( layoutName );
}

/**
 * Loads the configuration of the layout layoutName from the m_layoutsMap to the LayoutItemConfig area.
 * @param layoutName the name of the PlaylistLayout to be loaded for configuration
 */
void PlaylistLayoutEditDialog::setLayout( const QString &layoutName )   //SLOT
{
    m_layoutName = layoutName;
    debug()<< "Trying to load layout for configuration " << layoutName;

    if( m_layoutsMap->keys().contains( layoutName ) )   //is the layout exists in the list of loaded layouts
    {
        PlaylistLayout layout = m_layoutsMap->value( layoutName );
        m_headEdit->readLayout( layout.head() );
        m_bodyEdit->readLayout( layout.body() );
        m_singleEdit->readLayout( layout.single() );
    }
    else
    {
        debug() << "Empty layout, clearing config view";
        m_headEdit->clear();
        m_bodyEdit->clear();
        m_singleEdit->clear();
    }
}

/**
 * Applies to the playlist a preview of the currently defined layout.
 */
void PlaylistLayoutEditDialog::preview()
{
    PlaylistLayout layout;

    LayoutItemConfig headConfig = m_headEdit->config() ;
    headConfig.setActiveIndicatorRow( -1 );
    
    layout.setHead( headConfig );
    layout.setBody( m_bodyEdit->config() );
    layout.setSingle( m_singleEdit->config() );

    LayoutManager::instance()->setPreviewLayout( layout );
}

/**
 * Disables the delete button if the selected layout is one of the default layouts and enables it otherwise.
 */
void PlaylistLayoutEditDialog::toggleDeleteButton() //SLOT
{
    if( LayoutManager::instance()->isDefaultLayout( layoutListWidget->currentItem()->text() ) )
        deleteLayoutButton->setEnabled( 0 );
    else
        deleteLayoutButton->setEnabled( 1 );
}

/**
 * Saves the edited layouts from m_layoutMap to the LayoutManager and closes the dialog.
 */
void PlaylistLayoutEditDialog::accept()
{
    DEBUG_BLOCK

    QMap<QString, PlaylistLayout>::Iterator i = m_layoutsMap->begin();
    while( i != m_layoutsMap->end() )
    {
        debug() << "I'm on layout " << i.key();
        if( i.value().isDirty() )   //TODO: do setDirty if tokens have been moved
        {
            debug() << "Layout " << i.key() << " has been modified and will be saved.";
            if ( LayoutManager::instance()->isDefaultLayout( i.key() ) )
            {
                const QString msg = i18n( "The layout '%1' you modified is one of the default layouts and cannot be overwritten. \
                Please select a different name to save a copy.", layoutListWidget->currentItem()->text() );
                KMessageBox::sorry( this, msg, i18n( "Reserved Layout Name" ) );
                //TODO: handle this on layout switch maybe? this is not the right time to tell users they needed to make a copy in the first place
                return;
            }
            i.value().setDirty( 0 );
            LayoutManager::instance()->addUserLayout( i.key(), i.value() );
            debug() << "Layout " << i.key() << " saved to LayoutManager";
        }
        i++;
    }
    LayoutManager::instance()->setActiveLayout( layoutListWidget->currentItem()->text() );  //important to override the previewed layout if preview is used
    QDialog::accept();
}

/**
 * Closes the dialog without saving (almost) any changes.
 */
void PlaylistLayoutEditDialog::reject()
{
    DEBUG_BLOCK

    debug() << "Applying initial layout: " << m_firstActiveLayout;
    LayoutManager::instance()->setActiveLayout( m_firstActiveLayout );

    QDialog::reject();
}


#include "PlaylistLayoutEditDialog.moc"

