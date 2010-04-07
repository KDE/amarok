/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#define DEBUG_PREFIX "APG::PresetModel"

#include "PresetModel.h"

#include "Preset.h"
#include "PresetEditDialog.h"

#include "amarokconfig.h"
#include "core/collections/Collection.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "statusbar/StatusBar.h"

#include <QAbstractItemModel>
#include <QDialog>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QFileDialog>
#include <QList>
#include <QVariant>

APG::PresetModel* APG::PresetModel::s_instance = 0;

APG::PresetModel* APG::PresetModel::instance()
{
    if ( s_instance == 0 ) {
        s_instance = new PresetModel();
    }

    return s_instance;
}

void
APG::PresetModel::destroy()
{
    s_instance->savePresetsToXml( Amarok::saveLocation() + "playlistgenerator.xml", s_instance->m_presetList );
    delete s_instance;
    s_instance = 0;
}

APG::PresetModel::PresetModel()
        : QAbstractListModel()
        , m_activePresetIndex( 0 )
{
    loadPresetsFromXml( Amarok::saveLocation() + "playlistgenerator.xml" );
}

APG::PresetModel::~PresetModel()
{
}

QVariant
APG::PresetModel::data( const QModelIndex& idx, int role ) const
{
    if ( !idx.isValid() )
        return QVariant();

    if ( idx.row() >= m_presetList.size() )
        return QVariant();

    APG::PresetPtr item = m_presetList.at( idx.row() );

    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return item->title();
            break;
        default:
            return QVariant();
    }
    
    return QVariant();
}

QModelIndex
APG::PresetModel::index( int row, int column, const QModelIndex& ) const
{
    if ( rowCount() <= row )
        return QModelIndex();

    return createIndex( row, column, 0 );
}

int
APG::PresetModel::rowCount( const QModelIndex& ) const
{
    return m_presetList.size();
}

APG::PresetPtr
APG::PresetModel::activePreset() const
{
    if ( m_activePresetIndex && m_activePresetIndex->isValid() )
        return m_presetList.at( m_activePresetIndex->row() );
    else
        return APG::PresetPtr();
}

void
APG::PresetModel::addNew()
{
    insertPreset( APG::Preset::createNew() );
}

void
APG::PresetModel::edit()
{
    editPreset( createIndex( m_activePresetIndex->row(), 0 ) );
}

void
APG::PresetModel::editPreset( const QModelIndex& index )
{
    // TODO: possible enhancement: instead of using a modal dialog, use a QMap that allows
    // only one dialog per preset to be open at once
    PresetPtr ps = m_presetList.at( index.row() );
    QDialog* d = new PresetEditDialog( ps );
    d->exec();
}

void
APG::PresetModel::exportActive()
{
    QFileDialog* d = new ExportDialog( activePreset() );
    connect( d, SIGNAL( pleaseExport( const QString&, const QList<APG::PresetPtr> ) ),
          this, SLOT( savePresetsToXml( const QString&, const QList<APG::PresetPtr> ) ) );
    d->show();
}

void
APG::PresetModel::import()
{
    QFileDialog* d = new QFileDialog( 0, i18n("Import preset") );
    d->setAttribute( Qt::WA_DeleteOnClose );
    d->setDefaultSuffix( "xml" );
    d->setModal( true );
    d->setNameFilter( i18n("Preset files (*.xml)") );
    d->setLabelText( QFileDialog::Accept, i18n("Import") );
    connect( d, SIGNAL( fileSelected( const QString& ) ),
          this, SLOT( loadPresetsFromXml( const QString& ) ) );
    d->show();
}

void
APG::PresetModel::removeActive()
{
    if ( m_presetList.size() < 1 )
        return;

    if ( m_activePresetIndex && m_activePresetIndex->isValid() ) {
        int row = m_activePresetIndex->row();
        beginRemoveRows( QModelIndex(), row, row );
        APG::PresetPtr p = m_presetList.takeAt( row );
        p->deleteLater();
        endRemoveRows();
    }
}

void
APG::PresetModel::runGenerator( int q )
{
    activePreset()->generate( q );
}

void
APG::PresetModel::setActivePreset( const QModelIndex& index )
{
    if ( m_activePresetIndex )
        delete m_activePresetIndex;
    m_activePresetIndex = new QPersistentModelIndex( index );
}

void
APG::PresetModel::savePresetsToXml( const QString& filename, const QList<APG::PresetPtr> pl ) const
{
    QDomDocument xmldoc;
    QDomElement base = xmldoc.createElement( "playlistgenerator" );
    foreach ( APG::PresetPtr ps, pl ) {
        QDomElement* elemPtr = ps->toXml( xmldoc );
        base.appendChild( (*elemPtr) );
    }

    xmldoc.appendChild( base );
    QFile file( filename );
    if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        QTextStream out( &file );
        out.setCodec( "UTF-8" );
        xmldoc.save( out, 2, QDomNode::EncodingFromTextStream );
        if ( !filename.contains( "playlistgenerator.xml" ) ) {
            The::statusBar()->longMessage( i18n("Preset exported to %1", filename), StatusBar::Information );
        }
    } else {
        The::statusBar()->longMessage( i18n("Preset could not be exported to ", filename), StatusBar::Sorry );
        error() << "Can not write presets to " << filename;
    }
}

void
APG::PresetModel::loadPresetsFromXml( const QString& filename )
{
    QFile file( filename );
    if ( file.open( QIODevice::ReadOnly ) ) {
        QDomDocument document;
        if ( document.setContent( &file ) ) {
            debug() << "Reading presets from" << filename;
            parseXmlToPresets( document );
        } else {
            error() << "Failed to read" << filename;
            The::statusBar()->longMessage( i18n("Presets could not be imported from %1", filename), StatusBar::Sorry );
        }
        file.close();
    } else {
        if ( !filename.contains( "playlistgenerator.xml" ) ) {
            The::statusBar()->longMessage( i18n("%1 could not be opened for preset import", filename), StatusBar::Sorry );
        } else {
            QDomDocument document;
            document.setContent( presetExamples );
            debug() << "Reading built-in example presets";
            parseXmlToPresets( document );
        }
        error() << "Can not open" << filename;
    }
}

void
APG::PresetModel::insertPreset( APG::PresetPtr ps )
{
    if ( ps ) {
        int row = m_presetList.size();
        beginInsertRows( QModelIndex(), row, row );
        m_presetList.append( ps );
        endInsertRows();
        connect( ps.data(), SIGNAL( lock( bool ) ), this, SIGNAL( lock( bool ) ) );
    }
}

void
APG::PresetModel::parseXmlToPresets( QDomDocument& document )
{
    QDomElement rootelem = document.documentElement();
    for ( int i = 0; i < rootelem.childNodes().count(); i++ ) {
        QDomElement e = rootelem.childNodes().at( i ).toElement();
        if ( e.tagName() == "generatorpreset" ) {
            debug() << "creating a new generator preset";
            insertPreset( APG::Preset::createFromXml( e ) );
        } else {
            debug() << "Don't know what to do with tag: " << e.tagName();
        }
    }
}

/*
 * ExportDialog nested class
 */
APG::PresetModel::ExportDialog::ExportDialog( APG::PresetPtr ps )
    : QFileDialog( 0, i18n("Export `%1' preset", ps->title() ) )
{
    m_presetsToExportList.append( ps );
    selectFile( ps->title() + ".xml" );
    setAttribute( Qt::WA_DeleteOnClose );
    setDefaultSuffix( "xml" );
    setModal( true );
    setNameFilter( i18n("Preset files (*.xml)") );
    setLabelText( QFileDialog::Accept, i18n("Export") );
    connect( this, SIGNAL( fileSelected( const QString& ) ),
             this, SLOT( recvFileName( const QString& ) ) );
}

APG::PresetModel::ExportDialog::~ExportDialog() {}

void
APG::PresetModel::ExportDialog::recvFileName( const QString& filename ) const
{
    emit pleaseExport( filename, m_presetsToExportList );
}

const QString APG::PresetModel::presetExamples =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<playlistgenerator>"
"  <generatorpreset title=\"Example 1: new tracks added this week\">"
"    <constrainttree>"
"      <group matchtype=\"all\">"
"        <constraint field=\"added to collection\" comparison=\"3\" invert=\"false\" type=\"TagMatch\" value=\"7 days\" strictness=\"0.8\">Tag Match: added to collection within 7 days</constraint>"
"        <constraint field=\"play count\" comparison=\"0\" invert=\"false\" type=\"TagMatch\" value=\"\" strictness=\"1\">Tag Match: play count less than </constraint>"
"      </group>"
"    </constrainttree>"
"  </generatorpreset>"
"  <generatorpreset title=\"Example 2: rock or pop music\">"
"    <constrainttree>"
"      <group matchtype=\"any\">"
"        <constraint field=\"genre\" comparison=\"3\" invert=\"false\" type=\"TagMatch\" value=\"Rock\" strictness=\"1\">Tag Match: genre contains Rock</constraint>"
"        <constraint field=\"genre\" comparison=\"3\" invert=\"false\" type=\"TagMatch\" value=\"Pop\" strictness=\"1\">Tag Match: genre contains Pop</constraint>"
"      </group>"
"    </constrainttree>"
"  </generatorpreset>"
"  <generatorpreset title=\"Example 3: about one hour of tracks from different artists\">"
"    <constrainttree>"
"      <group matchtype=\"all\">"
"        <constraint comparison=\"1\" length=\"3600000\" type=\"PlaylistLength\" strictness=\"0.3\">Playlist length equals 1:00:00</constraint>"
"        <constraint field=\"2\" type=\"PreventDuplicates\">Prevent duplicate artists</constraint>"
"      </group>"
"    </constrainttree>"
"  </generatorpreset>"
"  <generatorpreset title=\"Example 4: like my favorite radio station\">"
"    <constrainttree>"
"      <group matchtype=\"all\">"
"        <constraint field=\"0\" type=\"PreventDuplicates\">Prevent duplicate tracks</constraint>"
"        <constraint field=\"last played\" comparison=\"3\" invert=\"true\" type=\"TagMatch\" value=\"7 days\" strictness=\"0.4\">Tag Match: not last played within 7 days</constraint>"
"        <constraint field=\"rating\" comparison=\"1\" invert=\"false\" type=\"TagMatch\" value=\"6\" strictness=\"1\">Tag Match: rating equals 3 stars</constraint>"
"        <constraint comparison=\"1\" length=\"10800000\" type=\"PlaylistLength\" strictness=\"0.3\">Playlist length equals 3:00:00</constraint>"
"      </group>"
"    </constrainttree>"
"  </generatorpreset>"
"  <generatorpreset title=\"Example 5: an 80-minute CD of rock, metal, and industrial\">"
"    <constrainttree>"
"      <group matchtype=\"all\">"
"        <group matchtype=\"any\">"
"          <constraint field=\"genre\" comparison=\"3\" invert=\"false\" type=\"TagMatch\" value=\"Rock\" strictness=\"1\">Tag Match: genre contains Rock</constraint>"
"          <constraint field=\"genre\" comparison=\"3\" invert=\"false\" type=\"TagMatch\" value=\"Metal\" strictness=\"1\">Tag Match: genre contains Metal</constraint>"
"          <constraint field=\"genre\" comparison=\"3\" invert=\"false\" type=\"TagMatch\" value=\"Industrial\" strictness=\"1\">Tag Match: genre contains Industrial</constraint>"
"        </group>"
"        <group matchtype=\"all\">"
"          <constraint comparison=\"2\" length=\"4500000\" type=\"PlaylistLength\" strictness=\"0.4\">Playlist length longer than 1:15:00</constraint>"
"          <constraint comparison=\"0\" length=\"4800000\" type=\"PlaylistLength\" strictness=\"1\">Playlist length shorter than 1:20:00</constraint>"
"        </group>"
"      </group>"
"    </constrainttree>"
"  </generatorpreset>"
"</playlistgenerator>";
