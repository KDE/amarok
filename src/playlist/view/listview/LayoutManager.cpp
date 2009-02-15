/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "LayoutManager.h"

#include "Amarok.h"
#include "Debug.h"
#include "playlist/PlaylistDefines.h"

#include <KMessageBox>
#include <KStandardDirs>
#include <KUrl>

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QStringList>

namespace Playlist {

LayoutManager * Playlist::LayoutManager::s_instance = 0;

LayoutManager * Playlist::LayoutManager::instance()
{
    if ( s_instance == 0 )
        s_instance = new LayoutManager();

    return s_instance;
}

LayoutManager::LayoutManager()
    : QObject()
{
    DEBUG_BLOCK

    loadDefaultLayouts();
    loadUserLayouts();

    KConfigGroup config = Amarok::config("Playlist Layout");
    m_activeLayout = config.readEntry( "CurrentLayout", "Default" );
}

LayoutManager::~LayoutManager()
{}

QStringList Playlist::LayoutManager::layouts()
{
    return m_layouts.keys();
}

void Playlist::LayoutManager::setActiveLayout( const QString &layout )
{
    m_activeLayout = layout;
    Amarok::config( "Playlist Layout" ).writeEntry( "CurrentLayout", m_activeLayout );
    emit( activeLayoutChanged() );
}

void Playlist::LayoutManager::setPreviewLayout( const PlaylistLayout &layout )
{
    m_activeLayout = "%%PREVIEW%%";
    m_previewLayout = layout;
    emit( activeLayoutChanged() );
}

PlaylistLayout Playlist::LayoutManager::activeLayout()
{
    if ( m_activeLayout == "%%PREVIEW%%" )
        return m_previewLayout;
    return m_layouts.value( m_activeLayout );
}

void Playlist::LayoutManager::loadUserLayouts()
{
    QDir layoutsDir = QDir( Amarok::saveLocation( "playlist_layouts/" ) );

    layoutsDir.setSorting( QDir::Name );

    QStringList filters;
    filters << "*.xml" << "*.XML";
    layoutsDir.setNameFilters(filters);
    layoutsDir.setSorting( QDir::Name );

    QFileInfoList list = layoutsDir.entryInfoList();

    for ( int i = 0; i < list.size(); ++i )
    {
        QFileInfo fileInfo = list.at(i);
        debug() << "found user file: " << fileInfo.fileName();
        loadLayouts( layoutsDir.filePath( fileInfo.fileName() ), true );
    }
}

void Playlist::LayoutManager::loadDefaultLayouts()
{
    const KUrl url( KStandardDirs::locate( "data", "amarok/data/" ) );
    QString configFile = url.path() + "DefaultPlaylistLayouts.xml";
    loadLayouts( configFile, false );
}


void Playlist::LayoutManager::loadLayouts( const QString &fileName, bool user )
{
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
        PlaylistLayout currentLayout;
        currentLayout.setIsEditable( user );

        currentLayout.setHead( parseItemConfig( layout.toElement().firstChildElement( "group_head" ) ) );
        currentLayout.setBody( parseItemConfig( layout.toElement().firstChildElement( "group_body" ) ) );
        currentLayout.setSingle( parseItemConfig( layout.toElement().firstChildElement( "single_track" ) ) );

        if ( !layoutName.isEmpty() )
            m_layouts.insert( layoutName, currentLayout );
    }
}

PrettyItemConfig Playlist::LayoutManager::parseItemConfig( const QDomElement &elem )
{
    const bool showCover = ( elem.attribute( "show_cover", "false" ).compare( "true", Qt::CaseInsensitive ) == 0 );
    const int activeIndicatorRow = elem.attribute( "active_indicator_row", "0" ).toInt();

    PrettyItemConfig config;
    config.setShowCover( showCover );
    config.setActiveIndicatorRow( activeIndicatorRow );

    QDomNodeList rows = elem.elementsByTagName("row");

    int index = 0;
    while ( index < rows.size() )
    {
        QDomNode rowNode = rows.item( index );
        index++;

        PrettyItemConfigRow row;

        QDomNodeList elements = rowNode.toElement().elementsByTagName("element");

        int index2 = 0;
        while ( index2 < elements.size() )
        {
            QDomNode elementNode = elements.item( index2 );
            index2++;

            int value = columnNames.indexOf( elementNode.toElement().attribute( "value", "Title" ) );
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

            row.addElement( PrettyItemConfigRowElement( value, size, bold, italic, alignment, prefix, sufix ) );
        }

        config.addRow( row );
    }

    return config;
}

PlaylistLayout Playlist::LayoutManager::layout( const QString &layout )
{
    return m_layouts.value( layout );
}

void Playlist::LayoutManager::addUserLayout( const QString &name, PlaylistLayout layout )
{
    layout.setIsEditable( true );
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

    debug() << doc.toString();
    QDir layoutsDir = QDir( Amarok::saveLocation( "playlist_layouts/" ) );

    //make sure that this dir exists
    if ( !layoutsDir.exists() )
        layoutsDir.mkpath( Amarok::saveLocation( "playlist_layouts/" ) );

    QFile file( layoutsDir.filePath( name + ".xml" ) );
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Text) )
        return;

    QTextStream out( &file );
    out << doc.toString();
}

QDomElement Playlist::LayoutManager::createItemElement( QDomDocument doc, const QString &name, const PrettyItemConfig & item ) const
{
    QDomElement element = doc.createElement( name );

    QString showCover = item.showCover() ? "true" : "false";
    element.setAttribute ( "show_cover", showCover );
    element.setAttribute ( "active_indicator_row", QString::number( item.activeIndicatorRow() ) );

    for( int i = 0; i < item.rows(); i++ )
    {
        PrettyItemConfigRow row = item.row( i );

        QDomElement rowElement = doc.createElement( "row" );
        element.appendChild( rowElement );

        for( int j = 0; j < row.count(); j++ ) {
            PrettyItemConfigRowElement element = row.element( j );
            QDomElement elementElement = doc.createElement( "element" );

            elementElement.setAttribute ( "value", columnNames[element.value()] );
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

QString LayoutManager::activeLayoutName()
{
    return m_activeLayout;
}

void LayoutManager::deleteLayout( const QString & layout )
{
    //check if layout is editable
    if ( m_layouts.value( layout ).isEditable() )
    {
        QDir layoutsDir = QDir( Amarok::saveLocation( "playlist_layouts/" ) );
        QString xmlFile = layoutsDir.path() + '/' + layout + ".xml";
        debug() << "deleting file: " << xmlFile;
       
        if ( !QFile::remove( xmlFile ) )
            debug() << "error deleting file....";

        m_layouts.remove( layout );
        emit( layoutListChanged() );

        if ( layout == m_activeLayout )
            setActiveLayout( "Default" );
    }
    else
        KMessageBox::sorry( 0, i18n( "The layout '%1' is one of the default layouts and cannot be deleted.", layout ), i18n( "Cannot Delete Default Layouts" ) );
}

} //namespace Playlist
