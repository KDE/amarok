/****************************************************************************************
 * Copyright (c) 2008-2009 Nikolaj Hald Nielsen <nhn@kde.org>                           *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "LayoutManager.h"

#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core/logger/Logger.h"
#include "playlist/PlaylistDefines.h"
#include "playlist/PlaylistModelStack.h"

#include <KConfigGroup>
#include <KMessageBox>

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QStandardPaths>
#include <QStringList>


namespace Playlist {

static const QString PREVIEW_LAYOUT = QStringLiteral("%%PREVIEW%%");
LayoutManager* LayoutManager::s_instance = nullptr;

LayoutManager* LayoutManager::instance()
{
    if( !s_instance )
        s_instance = new LayoutManager();
    return s_instance;
}

LayoutManager::LayoutManager()
    : QObject()
{
    DEBUG_BLOCK

    loadDefaultLayouts();
    loadUserLayouts();
    orderLayouts();

    KConfigGroup config = Amarok::config(QStringLiteral("Playlist Layout"));
    m_activeLayout = config.readEntry( "CurrentLayout", "Default" );
    if( !layouts().contains( m_activeLayout ) )
        m_activeLayout = QStringLiteral("Default");
    Playlist::ModelStack::instance()->groupingProxy()->setGroupingCategory( activeLayout().groupBy() );
}

QStringList LayoutManager::layouts() const
{
    return m_layoutNames;
}

void LayoutManager::setActiveLayout( const QString &layout )
{
    m_activeLayout = layout;
    Amarok::config( QStringLiteral("Playlist Layout") ).writeEntry( "CurrentLayout", m_activeLayout );
    Q_EMIT( activeLayoutChanged() );

    //Change the grouping style to that of this layout.
    Playlist::ModelStack::instance()->groupingProxy()->setGroupingCategory( activeLayout().groupBy() );

}

void LayoutManager::setPreviewLayout( const PlaylistLayout &layout )
{
    DEBUG_BLOCK
    m_activeLayout = PREVIEW_LAYOUT;
    m_previewLayout = layout;
    Q_EMIT( activeLayoutChanged() );

    //Change the grouping style to that of this layout.
    Playlist::ModelStack::instance()->groupingProxy()->setGroupingCategory( activeLayout().groupBy() );
}

void LayoutManager::updateCurrentLayout( const PlaylistLayout &layout )
{
    //Do not store preview layouts.
    if ( m_activeLayout == PREVIEW_LAYOUT )
        return;

    if ( m_layouts.value( m_activeLayout ).isEditable() )
    {
        addUserLayout( m_activeLayout, layout );
        setActiveLayout( m_activeLayout );
    }
    else
    {
        //create a writable copy of this layout. (Copy on Write)
        QString newLayoutName = i18n( "copy of %1", m_activeLayout );
        QString orgCopyName = newLayoutName;

        int copyNumber = 1;
        QStringList existingLayouts = LayoutManager::instance()->layouts();
        while( existingLayouts.contains( newLayoutName ) )
        {
            copyNumber++;
            newLayoutName = i18nc( "adds a copy number to a generated name if the name already exists, for instance 'copy of Foo 2' if 'copy of Foo' is taken", "%1 %2", orgCopyName, copyNumber );
        }


        Amarok::Logger::longMessage( i18n( "Current layout '%1' is read only. " \
                    "Creating a new layout '%2' with your changes and setting this as active",
                                                         m_activeLayout, newLayoutName )
                                                 );

        addUserLayout( newLayoutName, layout );
        setActiveLayout( newLayoutName );
    }
}

PlaylistLayout LayoutManager::activeLayout() const
{
    if ( m_activeLayout == PREVIEW_LAYOUT )
        return m_previewLayout;
    return m_layouts.value( m_activeLayout );
}

void LayoutManager::loadUserLayouts()
{
    QDir layoutsDir = QDir( Amarok::saveLocation( QStringLiteral("playlist_layouts/") ) );

    layoutsDir.setSorting( QDir::Name );

    QStringList filters;
    filters << QStringLiteral("*.xml") << QStringLiteral("*.XML");
    layoutsDir.setNameFilters( filters );
    layoutsDir.setSorting( QDir::Name );

    QFileInfoList list = layoutsDir.entryInfoList();

    for ( int i = 0; i < list.size(); ++i )
    {
        QFileInfo fileInfo = list.at(i);
        loadLayouts( layoutsDir.filePath( fileInfo.fileName() ), true );
    }
}

void LayoutManager::loadDefaultLayouts()
{
    const QString dataLocation = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                       QStringLiteral("amarok/data"),
                                                       QStandardPaths::LocateDirectory);


    QString configFile = dataLocation + QStringLiteral("/DefaultPlaylistLayouts.xml");
    loadLayouts( configFile, false );
}


void LayoutManager::loadLayouts( const QString &fileName, bool user )
{
    DEBUG_BLOCK
    QDomDocument doc( QStringLiteral("layouts") );

    if ( !QFile::exists( fileName ) )
    {
        debug() << "file " << fileName << "does not exist";
        return;
    }

    QFile *file = new QFile( fileName );
    if( !file || !file->open( QIODevice::ReadOnly ) )
    {
        debug() << "error reading file " << fileName;
        return;
    }
    if ( !doc.setContent( file ) )
    {
        debug() << "error parsing file " << fileName;
        file->close();
        return ;
    }
    file->close();
    delete file;

    QDomElement layouts_element = doc.firstChildElement( QStringLiteral("playlist_layouts") );
    QDomNodeList layouts = layouts_element.elementsByTagName(QStringLiteral("layout"));

    int index = 0;
    while ( index < layouts.size() )
    {
        QDomNode layout = layouts.item( index );
        index++;

        QString layoutName = layout.toElement().attribute( QStringLiteral("name"), QLatin1String("") );
        debug() << "loading layout " << layoutName;

        PlaylistLayout currentLayout;
        currentLayout.setEditable( user );
        currentLayout.setInlineControls( layout.toElement().attribute( QStringLiteral("inline_controls"), QStringLiteral("false") ).compare( QLatin1String("true"), Qt::CaseInsensitive ) == 0 );
        currentLayout.setTooltips( layout.toElement().attribute( QStringLiteral("tooltips"), QStringLiteral("false") ).compare( QLatin1String("true"), Qt::CaseInsensitive ) == 0 );

        //For backwards compatibility, if a grouping is not set in the XML file assume "group by album" (which was previously the default)
        currentLayout.setGroupBy( layout.toElement().attribute( QStringLiteral("group_by"), QStringLiteral("Album") ) );
        debug() << "grouping mode is: " << layout.toElement().attribute( QStringLiteral("group_by"), QStringLiteral("Album") );


        currentLayout.setLayoutForPart( PlaylistLayout::Head, parseItemConfig( layout.toElement().firstChildElement( QStringLiteral("group_head") ) ) );
        currentLayout.setLayoutForPart( PlaylistLayout::StandardBody, parseItemConfig( layout.toElement().firstChildElement( QStringLiteral("group_body") ) ) );
        QDomElement variousArtistsXML = layout.toElement().firstChildElement( QStringLiteral("group_variousArtistsBody") );
        if ( !variousArtistsXML.isNull() )
            currentLayout.setLayoutForPart( PlaylistLayout::VariousArtistsBody, parseItemConfig( variousArtistsXML ) );
        else    // Handle old custom layout XMLs
            currentLayout.setLayoutForPart( PlaylistLayout::VariousArtistsBody, parseItemConfig( layout.toElement().firstChildElement( QStringLiteral("group_body") ) ) );
        currentLayout.setLayoutForPart( PlaylistLayout::Single, parseItemConfig( layout.toElement().firstChildElement( QStringLiteral("single_track") ) ) );

        if ( !layoutName.isEmpty() )
            m_layouts.insert( layoutName, currentLayout );
    }
}

LayoutItemConfig LayoutManager::parseItemConfig( const QDomElement &elem ) const
{
    const bool showCover = ( elem.attribute( QStringLiteral("show_cover"), QStringLiteral("false") ).compare( QLatin1String("true"), Qt::CaseInsensitive ) == 0 );
    const int activeIndicatorRow = elem.attribute( QStringLiteral("active_indicator_row"), QStringLiteral("0") ).toInt();

    LayoutItemConfig config;
    config.setShowCover( showCover );
    config.setActiveIndicatorRow( activeIndicatorRow );

    QDomNodeList rows = elem.elementsByTagName(QStringLiteral("row"));

    int index = 0;
    while ( index < rows.size() )
    {
        QDomNode rowNode = rows.item( index );
        index++;

        LayoutItemConfigRow row;

        QDomNodeList elements = rowNode.toElement().elementsByTagName(QStringLiteral("element"));

        int index2 = 0;
        while ( index2 < elements.size() )
        {
            QDomNode elementNode = elements.item( index2 );
            index2++;

            int value = columnForName( elementNode.toElement().attribute( QStringLiteral("value"), QStringLiteral("Title") ) );
            QString prefix = elementNode.toElement().attribute( QStringLiteral("prefix"), QString() );
            QString sufix = elementNode.toElement().attribute( QStringLiteral("suffix"), QString() );
            qreal size = elementNode.toElement().attribute( QStringLiteral("size"), QStringLiteral("0.0") ).toDouble();
            bool bold = ( elementNode.toElement().attribute( QStringLiteral("bold"), QStringLiteral("false") ).compare( QLatin1String("true"), Qt::CaseInsensitive ) == 0 );
            bool italic = ( elementNode.toElement().attribute( QStringLiteral("italic"), QStringLiteral("false") ).compare( QLatin1String("true"), Qt::CaseInsensitive ) == 0 );
            bool underline = ( elementNode.toElement().attribute( QStringLiteral("underline"), QStringLiteral("false") ).compare( QLatin1String("true"), Qt::CaseInsensitive ) == 0 );
            QString alignmentString = elementNode.toElement().attribute( QStringLiteral("alignment"), QStringLiteral("left") );
            Qt::Alignment alignment;


            if ( alignmentString.compare( QLatin1String("left"), Qt::CaseInsensitive ) == 0 )
                alignment = Qt::AlignLeft | Qt::AlignVCenter;
            else if ( alignmentString.compare( QLatin1String("right"), Qt::CaseInsensitive ) == 0 )
                 alignment = Qt::AlignRight| Qt::AlignVCenter;
            else
                alignment = Qt::AlignCenter| Qt::AlignVCenter;

            row.addElement( LayoutItemConfigRowElement( value, size, bold, italic, underline,
                                                        alignment, prefix, sufix ) );
        }

        config.addRow( row );
    }

    return config;
}

PlaylistLayout LayoutManager::layout( const QString &layout ) const
{
    return m_layouts.value( layout );
}

void LayoutManager::addUserLayout( const QString &name, PlaylistLayout layout )
{
    layout.setEditable( true );
    if( m_layouts.find( name ) != m_layouts.end() )
        m_layouts.remove( name );
    else
        m_layoutNames.append( name );

    m_layouts.insert( name, layout );


    QDomDocument doc( QStringLiteral("layouts") );
    QDomElement layouts_element = doc.createElement( QStringLiteral("playlist_layouts") );
    QDomElement newLayout = doc.createElement( QStringLiteral("layout" ) );
    newLayout.setAttribute( QStringLiteral("name"), name );

    doc.appendChild( layouts_element );
    layouts_element.appendChild( newLayout );

    Q_EMIT( layoutListChanged() );

    QDomElement body = doc.createElement( QStringLiteral("body") );
    QDomElement single = doc.createElement( QStringLiteral("single") );

    newLayout.appendChild( createItemElement( doc, QStringLiteral("single_track"), layout.layoutForPart( PlaylistLayout::Single ) ) );
    newLayout.appendChild( createItemElement( doc, QStringLiteral("group_head"), layout.layoutForPart( PlaylistLayout::Head ) ) );
    newLayout.appendChild( createItemElement( doc, QStringLiteral("group_body"), layout.layoutForPart( PlaylistLayout::StandardBody ) ) );
    newLayout.appendChild( createItemElement( doc, QStringLiteral("group_variousArtistsBody"), layout.layoutForPart( PlaylistLayout::VariousArtistsBody ) ) );

    if( layout.inlineControls() )
        newLayout.setAttribute( QStringLiteral("inline_controls"), QStringLiteral("true") );

    if( layout.tooltips() )
        newLayout.setAttribute( QStringLiteral("tooltips"), QStringLiteral("true") );

    newLayout.setAttribute( QStringLiteral("group_by"), layout.groupBy() );

    QDir layoutsDir = QDir( Amarok::saveLocation( QStringLiteral("playlist_layouts/") ) );

    //make sure that this directory exists
    if ( !layoutsDir.exists() )
        layoutsDir.mkpath( Amarok::saveLocation( QStringLiteral("playlist_layouts/") ) );

    QFile file( layoutsDir.filePath( name + QStringLiteral(".xml") ) );
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Text) )
        return;

    QTextStream out( &file );
    out << doc.toString();
}

QDomElement LayoutManager::createItemElement( QDomDocument doc, const QString &name, const LayoutItemConfig & item ) const
{
    QDomElement element = doc.createElement( name );

    QString showCover = item.showCover() ? QStringLiteral("true") : QStringLiteral("false");
    element.setAttribute ( QStringLiteral("show_cover"), showCover );
    element.setAttribute ( QStringLiteral("active_indicator_row"), QString::number( item.activeIndicatorRow() ) );

    for( int i = 0; i < item.rows(); i++ )
    {
        LayoutItemConfigRow row = item.row( i );

        QDomElement rowElement = doc.createElement( QStringLiteral("row") );
        element.appendChild( rowElement );

        for( int j = 0; j < row.count(); j++ ) {
            LayoutItemConfigRowElement element = row.element( j );
            QDomElement elementElement = doc.createElement( QStringLiteral("element") );

            elementElement.setAttribute ( QStringLiteral("prefix"), element.prefix() );
            elementElement.setAttribute ( QStringLiteral("suffix"), element.suffix() );
            elementElement.setAttribute ( QStringLiteral("value"), internalColumnName( static_cast<Playlist::Column>( element.value() ) ) );
            elementElement.setAttribute ( QStringLiteral("size"), QString::number( element.size() ) );
            elementElement.setAttribute ( QStringLiteral("bold"), element.bold() ? QStringLiteral("true") : QStringLiteral("false") );
            elementElement.setAttribute ( QStringLiteral("italic"), element.italic() ? QStringLiteral("true") : QStringLiteral("false") );
            elementElement.setAttribute ( QStringLiteral("underline"), element.underline() ? QStringLiteral("true") : QStringLiteral("false") );

            QString alignmentString;
            if ( element.alignment() & Qt::AlignLeft )
                alignmentString = QStringLiteral("left");
            else  if ( element.alignment() & Qt::AlignRight )
                alignmentString = QStringLiteral("right");
            else
                alignmentString = QStringLiteral("center");

            elementElement.setAttribute ( QStringLiteral("alignment"), alignmentString );

            rowElement.appendChild( elementElement );
        }
    }

    return element;
}

bool LayoutManager::isDefaultLayout( const QString & layout ) const
{
    if ( m_layouts.keys().contains( layout ) )
        return !m_layouts.value( layout ).isEditable();

    return false;
}

QString LayoutManager::activeLayoutName() const
{
    return m_activeLayout;
}

void LayoutManager::deleteLayout( const QString &layout )
{
    //check if layout is editable
    if ( m_layouts.value( layout ).isEditable() )
    {
        QDir layoutsDir = QDir( Amarok::saveLocation( QStringLiteral("playlist_layouts/") ) );
        QString xmlFile = layoutsDir.path() + QLatin1Char('/') + layout + QStringLiteral(".xml");

        if ( !QFile::remove( xmlFile ) )
            debug() << "error deleting file" << xmlFile;

        m_layouts.remove( layout );
        m_layoutNames.removeAll( layout );
        Q_EMIT( layoutListChanged() );

        if ( layout == m_activeLayout )
            setActiveLayout( QStringLiteral("Default") );
    }
    else
        KMessageBox::error( nullptr, i18n( "The layout '%1' is one of the default layouts and cannot be deleted.", layout ), i18n( "Cannot Delete Default Layouts" ) );
}

bool LayoutManager::isDeleteable( const QString &layout ) const
{
    return m_layouts.value( layout ).isEditable();
}

int LayoutManager::moveUp( const QString &layout )
{
    int index = m_layoutNames.indexOf( layout );
    if ( index > 0 ) {
        m_layoutNames.swapItemsAt ( index, index - 1 );
        Q_EMIT( layoutListChanged() );
        storeLayoutOrdering();
        return index - 1;
    }

    return index;
}

int LayoutManager::moveDown( const QString &layout )
{
    int index = m_layoutNames.indexOf( layout );
    if ( index < m_layoutNames.size() -1 ) {
        m_layoutNames.swapItemsAt ( index, index + 1 );
        Q_EMIT( layoutListChanged() );
        storeLayoutOrdering();
        return index + 1;
    }

    return index;
}

void LayoutManager::orderLayouts()
{
    KConfigGroup config = Amarok::config( QStringLiteral("Playlist Layout") );
    QString orderString = config.readEntry( "Order", "Default" );

    QStringList knownLayouts = m_layouts.keys();

    QStringList orderingList = orderString.split( QLatin1Char(';'), Qt::SkipEmptyParts );

    for( const QString &layout : orderingList )
    {
        if ( knownLayouts.contains( layout ) )
        {
            //skip any layout names that are in config but that we don't know. Perhaps someone manually deleted a layout file
            m_layoutNames.append( layout );
            knownLayouts.removeAll( layout );
        }
    }

    //now add any layouts that were not in the order config to end of list:
    for( const QString &layout : knownLayouts )
        m_layoutNames.append( layout );
}

} //namespace Playlist

void Playlist::LayoutManager::storeLayoutOrdering()
{

    QString ordering;

    for( const QString &name : m_layoutNames )
    {
        ordering += name;
        ordering += QLatin1Char(';');
    }

    if ( !ordering.isEmpty() )
        ordering.chop( 1 ); //remove trailing;

    KConfigGroup config = Amarok::config(QStringLiteral("Playlist Layout"));
    config.writeEntry( "Order", ordering );
}




