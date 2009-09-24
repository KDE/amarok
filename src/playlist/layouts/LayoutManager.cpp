/****************************************************************************************
 * Copyright (c) 2008-2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "Amarok.h"
#include "Debug.h"
#include "playlist/PlaylistDefines.h"
#include "playlist/PlaylistModelStack.h"

#include <KMessageBox>
#include <KStandardDirs>
#include <KUrl>

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QStringList>

namespace Playlist {

static const QString PREVIEW_LAYOUT = "%%PREVIEW%%";
LayoutManager* LayoutManager::s_instance = 0;

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

    KConfigGroup config = Amarok::config("Playlist Layout");
    m_activeLayout = config.readEntry( "CurrentLayout", "Default" );
    Playlist::ModelStack::instance()->top()->setGroupingCategory( activeLayout().groupBy() );
}

QStringList LayoutManager::layouts() const
{
    return m_layoutNames;
}

void LayoutManager::setActiveLayout( const QString &layout )
{
    m_activeLayout = layout;
    Amarok::config( "Playlist Layout" ).writeEntry( "CurrentLayout", m_activeLayout );  
    emit( activeLayoutChanged() );

    //Change the grouping style to that of this layout.
    Playlist::ModelStack::instance()->top()->setGroupingCategory( activeLayout().groupBy() );

}

void LayoutManager::setPreviewLayout( const PlaylistLayout &layout )
{
    m_activeLayout = PREVIEW_LAYOUT;
    m_previewLayout = layout;
    emit( activeLayoutChanged() );

    //Change the grouping style to that of this layout.
    Playlist::ModelStack::instance()->top()->setGroupingCategory( activeLayout().groupBy() );
}

PlaylistLayout LayoutManager::activeLayout() const
{
    if ( m_activeLayout == PREVIEW_LAYOUT )
        return m_previewLayout;
    return m_layouts.value( m_activeLayout );
}

void LayoutManager::loadUserLayouts()
{
    QDir layoutsDir = QDir( Amarok::saveLocation( "playlist_layouts/" ) );

    layoutsDir.setSorting( QDir::Name );

    QStringList filters;
    filters << "*.xml" << "*.XML";
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
    const KUrl url( KStandardDirs::locate( "data", "amarok/data/" ) );
    QString configFile = url.toLocalFile() + "DefaultPlaylistLayouts.xml";
    loadLayouts( configFile, false );
}


void LayoutManager::loadLayouts( const QString &fileName, bool user )
{
    DEBUG_BLOCK
    QDomDocument doc( "layouts" );

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

    QDomElement layouts_element = doc.firstChildElement( "playlist_layouts" );
    QDomNodeList layouts = layouts_element.elementsByTagName("layout");

    int index = 0;
    while ( index < layouts.size() )
    {
        QDomNode layout = layouts.item( index );
        index++;

        QString layoutName = layout.toElement().attribute( "name", "" );
        debug() << "loading layout " << layoutName;

        PlaylistLayout currentLayout;
        currentLayout.setEditable( user );
        currentLayout.setInlineControls( layout.toElement().attribute( "inline_controls", "false" ).compare( "true", Qt::CaseInsensitive ) == 0 );

        //For backwards compatibility, if a grouping is not set in the XML file assume "group by album" (which was previously the default)
        currentLayout.setGroupBy( layout.toElement().attribute( "group_by", "Album" ) );
        debug() << "grouping mode is: " << layout.toElement().attribute( "group_by", "Album" );
        

        currentLayout.setHead( parseItemConfig( layout.toElement().firstChildElement( "group_head" ) ) );
        currentLayout.setBody( parseItemConfig( layout.toElement().firstChildElement( "group_body" ) ) );
        currentLayout.setSingle( parseItemConfig( layout.toElement().firstChildElement( "single_track" ) ) );

        if ( !layoutName.isEmpty() )
            m_layouts.insert( layoutName, currentLayout );
    }
}

LayoutItemConfig LayoutManager::parseItemConfig( const QDomElement &elem ) const
{
    const bool showCover = ( elem.attribute( "show_cover", "false" ).compare( "true", Qt::CaseInsensitive ) == 0 );
    const int activeIndicatorRow = elem.attribute( "active_indicator_row", "0" ).toInt();

    LayoutItemConfig config;
    config.setShowCover( showCover );
    config.setActiveIndicatorRow( activeIndicatorRow );

    QDomNodeList rows = elem.elementsByTagName("row");

    int index = 0;
    while ( index < rows.size() )
    {
        QDomNode rowNode = rows.item( index );
        index++;

        LayoutItemConfigRow row;

        QDomNodeList elements = rowNode.toElement().elementsByTagName("element");

        int index2 = 0;
        while ( index2 < elements.size() )
        {
            QDomNode elementNode = elements.item( index2 );
            index2++;

            int value = internalColumnNames.indexOf( elementNode.toElement().attribute( "value", "Title" ) );
            QString prefix = elementNode.toElement().attribute( "prefix", QString() );
            QString sufix = elementNode.toElement().attribute( "suffix", QString() );
            qreal size = elementNode.toElement().attribute( "size", "0.0" ).toDouble();
            bool bold = ( elementNode.toElement().attribute( "bold", "false" ).compare( "true", Qt::CaseInsensitive ) == 0 );
            bool italic = ( elementNode.toElement().attribute( "italic", "false" ).compare( "true", Qt::CaseInsensitive ) == 0 );
            QString alignmentString = elementNode.toElement().attribute( "alignment", "left" );
            Qt::Alignment alignment;


            if ( alignmentString.compare( "left", Qt::CaseInsensitive ) == 0 )
                alignment = Qt::AlignLeft | Qt::AlignVCenter;
            else if ( alignmentString.compare( "right", Qt::CaseInsensitive ) == 0 )
                 alignment = Qt::AlignRight| Qt::AlignVCenter;
            else
                alignment = Qt::AlignCenter| Qt::AlignVCenter;

            row.addElement( LayoutItemConfigRowElement( value, size, bold, italic, alignment, prefix, sufix ) );
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


    QDomDocument doc( "layouts" );
    QDomElement layouts_element = doc.createElement( "playlist_layouts" );
    QDomElement newLayout = doc.createElement( ("layout" ) );
    newLayout.setAttribute( "name", name );

    doc.appendChild( layouts_element );
    layouts_element.appendChild( newLayout );

    emit( layoutListChanged() );

    QDomElement body = doc.createElement( "body" );
    QDomElement single = doc.createElement( "single" );

    newLayout.appendChild( createItemElement( doc, "single_track", layout.single() ) );
    newLayout.appendChild( createItemElement( doc, "group_head", layout.head() ) );
    newLayout.appendChild( createItemElement( doc, "group_body", layout.body() ) );

    if( layout.inlineControls() )
        newLayout.setAttribute( "inline_controls", "true" );

    newLayout.setAttribute( "group_by", layout.groupBy() );

    QDir layoutsDir = QDir( Amarok::saveLocation( "playlist_layouts/" ) );

    //make sure that this directory exists
    if ( !layoutsDir.exists() )
        layoutsDir.mkpath( Amarok::saveLocation( "playlist_layouts/" ) );

    QFile file( layoutsDir.filePath( name + ".xml" ) );
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Text) )
        return;

    QTextStream out( &file );
    out << doc.toString();
}

QDomElement LayoutManager::createItemElement( QDomDocument doc, const QString &name, const LayoutItemConfig & item ) const
{
    QDomElement element = doc.createElement( name );

    QString showCover = item.showCover() ? "true" : "false";
    element.setAttribute ( "show_cover", showCover );
    element.setAttribute ( "active_indicator_row", QString::number( item.activeIndicatorRow() ) );

    for( int i = 0; i < item.rows(); i++ )
    {
        LayoutItemConfigRow row = item.row( i );

        QDomElement rowElement = doc.createElement( "row" );
        element.appendChild( rowElement );

        for( int j = 0; j < row.count(); j++ ) {
            LayoutItemConfigRowElement element = row.element( j );
            QDomElement elementElement = doc.createElement( "element" );

            elementElement.setAttribute ( "prefix", element.prefix() );
            elementElement.setAttribute ( "suffix", element.suffix() );
            elementElement.setAttribute ( "value", internalColumnNames[element.value()] );
            elementElement.setAttribute ( "size", QString::number( element.size() ) );
            elementElement.setAttribute ( "bold", element.bold() ? "true" : "false" );
            elementElement.setAttribute ( "italic", element.italic() ? "true" : "false" );

            QString alignmentString;
            if ( element.alignment() & Qt::AlignLeft )
                alignmentString = "left";
            else  if ( element.alignment() & Qt::AlignRight )
                alignmentString = "right";
            else
                alignmentString = "center";

            elementElement.setAttribute ( "alignment", alignmentString );

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
        QDir layoutsDir = QDir( Amarok::saveLocation( "playlist_layouts/" ) );
        QString xmlFile = layoutsDir.path() + '/' + layout + ".xml";

        if ( !QFile::remove( xmlFile ) )
            debug() << "error deleting file" << xmlFile;

        m_layouts.remove( layout );
        m_layoutNames.removeAll( layout );
        emit( layoutListChanged() );

        if ( layout == m_activeLayout )
            setActiveLayout( "Default" );
    }
    else
        KMessageBox::sorry( 0, i18n( "The layout '%1' is one of the default layouts and cannot be deleted.", layout ), i18n( "Cannot Delete Default Layouts" ) );
}

bool LayoutManager::isDeleteable( const QString &layout ) const
{
    return m_layouts.value( layout ).isEditable();
}

int LayoutManager::moveUp( const QString &layout )
{
    int index = m_layoutNames.indexOf( layout );
    if ( index > 0 ) {
        m_layoutNames.swap ( index, index - 1 );
        emit( layoutListChanged() );
        storeLayoutOrdering();
        return index - 1;
    }

    return index;
}

int LayoutManager::moveDown( const QString &layout )
{
    int index = m_layoutNames.indexOf( layout );
    if ( index < m_layoutNames.size() -1 ) {
        m_layoutNames.swap ( index, index + 1 );
        emit( layoutListChanged() );
        storeLayoutOrdering();
        return index + 1;
    }

    return index;
}

void LayoutManager::orderLayouts()
{
    KConfigGroup config = Amarok::config( "Playlist Layout" );
    QString orderString = config.readEntry( "Order", "Default" );

    QStringList knownLayouts = m_layouts.keys();

    QStringList orderingList = orderString.split( ';', QString::SkipEmptyParts );

    foreach( QString layout, orderingList )
    {
        if ( knownLayouts.contains( layout ) )
        {
            //skip any layout names that are in config but that we don't know. Perhaps someone manually deleted a layout file
            m_layoutNames.append( layout );
            knownLayouts.removeAll( layout );
        }
    }

    //now add any layouts that were not in the order config to end of list:
    foreach( QString layout, knownLayouts )
        m_layoutNames.append( layout );
}

} //namespace Playlist

void Playlist::LayoutManager::storeLayoutOrdering()
{

    QString ordering;

    foreach( QString name, m_layoutNames )
    {
        ordering += name;
        ordering += ';';
    }

    if ( !ordering.isEmpty() )
        ordering.chop( 1 ); //remove trailing;

    KConfigGroup config = Amarok::config("Playlist Layout");
    config.writeEntry( "Order", ordering );
}




