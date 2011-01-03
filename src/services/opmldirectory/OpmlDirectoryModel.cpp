/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org                              *
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

#include "OpmlDirectoryModel.h"

#include "core/support/Amarok.h"
#include "MainWindow.h"
#include "OpmlParser.h"
#include "OpmlWriter.h"
#include "core/support/Debug.h"

#include "ui_AddOpmlWidget.h"

#include <ThreadWeaver/Weaver>

#include <KDialog>

#include <QAction>

OpmlDirectoryModel::OpmlDirectoryModel( KUrl outlineUrl, QObject *parent )
    : QAbstractItemModel( parent )
    , m_rootOpmlUrl( outlineUrl )
{
    //fetchMore will be called by the view
    m_addOpmlAction = new QAction( KIcon( "list-add" ), i18n( "Add OPML" ), this );
    connect( m_addOpmlAction, SIGNAL(triggered()), SLOT(slotAddOpmlAction()) );

    m_addFolderAction = new QAction( KIcon( "folder-add" ), i18n( "Add Folder"), this );
    connect( m_addFolderAction, SIGNAL(triggered()), SLOT(slotAddFolderAction()) );
}

OpmlDirectoryModel::~OpmlDirectoryModel()
{
}

QModelIndex
OpmlDirectoryModel::index( int row, int column, const QModelIndex &parent ) const
{
    if( !parent.isValid() )
    {
        if( m_rootOutlines.isEmpty() || m_rootOutlines.count() <= row )
            return QModelIndex();
        else
            return createIndex( row, column, m_rootOutlines[row] );
    }

    OpmlOutline *parentOutline = static_cast<OpmlOutline *>( parent.internalPointer() );
    if( !parentOutline )
        return QModelIndex();

    if( !parentOutline->hasChildren() || parentOutline->children().count() <= row )
        return QModelIndex();

    return createIndex( row, column, parentOutline->children()[row] );
}

Qt::ItemFlags
OpmlDirectoryModel::flags( const QModelIndex &idx ) const
{
    if( !idx.isValid() )
        return Qt::ItemIsDropEnabled;

    OpmlOutline *outline = static_cast<OpmlOutline *>( idx.internalPointer() );
    if( outline && !outline->attributes().contains( "type" ) ) //probably a folder
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled
                | Qt::ItemIsDropEnabled;

    return QAbstractItemModel::flags( idx );
}

QModelIndex
OpmlDirectoryModel::parent( const QModelIndex &idx ) const
{
    if( !idx.isValid() )
        return QModelIndex();
    debug() << idx;
    OpmlOutline *outline = static_cast<OpmlOutline *>( idx.internalPointer() );
    if( outline->isRootItem() )
        return QModelIndex();

    OpmlOutline *parentOutline = outline->parent();
    int childIndex;
    if( parentOutline->isRootItem() )
        childIndex = m_rootOutlines.indexOf( parentOutline );
    else
        childIndex = parentOutline->parent()->children().indexOf( parentOutline );
    return createIndex( childIndex, 0, parentOutline );
}

int
OpmlDirectoryModel::rowCount( const QModelIndex &parent ) const
{
    if( !parent.isValid() )
        return m_rootOutlines.count();

    OpmlOutline *outline = static_cast<OpmlOutline *>( parent.internalPointer() );

    if( !outline || !outline->hasChildren() )
        return 0;
    else
        return outline->children().count();
}

bool
OpmlDirectoryModel::hasChildren( const QModelIndex &parent ) const
{
    debug() << parent;
    if( !parent.isValid() )
        return !m_rootOutlines.isEmpty();

    OpmlOutline *outline = static_cast<OpmlOutline *>( parent.internalPointer() );

    if( !outline )
        return false;

    if( outline->hasChildren() )
        return true;

    return outline->attributes().value( "type" ) == "include";
}

int
OpmlDirectoryModel::columnCount( const QModelIndex &parent ) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant
OpmlDirectoryModel::data( const QModelIndex &idx, int role ) const
{
    if( !idx.isValid() )
    {
        if( role == ActionRole )
        {
            QList<QAction *> actions;
            actions << m_addOpmlAction << m_addFolderAction;
            return QVariant::fromValue( actions );
        }
        return QVariant();
    }

    OpmlOutline *outline = static_cast<OpmlOutline *>( idx.internalPointer() );
    if( !outline )
        return QVariant();

    switch( role )
    {
        case Qt::DisplayRole:
            return outline->attributes()["text"];
        case Qt::DecorationRole:
            return m_imageMap.contains( outline ) ? m_imageMap.value( outline ) : QVariant();
        case ActionRole:
        {
            if( opmlNodeType( outline ) == CategoryNode ) //probably a folder
                return QVariant::fromValue( QActionList() << m_addOpmlAction << m_addFolderAction );
        }
        default:
            return QVariant();
    }

    return QVariant();
}

bool
OpmlDirectoryModel::setData( const QModelIndex &idx, const QVariant &value, int role )
{
    Q_UNUSED(role);

    if( !idx.isValid() )
        return false;

    OpmlOutline *outline = static_cast<OpmlOutline *>( idx.internalPointer() );
    if( !outline )
        return false;

    outline->mutableAttributes()["text"] = value.toString();

    saveOpml( m_rootOpmlUrl );

    return true;
}

void
OpmlDirectoryModel::saveOpml( const KUrl &saveLocation )
{
    if( !saveLocation.isLocalFile() )
    {
        //TODO:implement
        error() << "can not save OPML to remote location";
        return;
    }

    QFile *opmlFile = new QFile( saveLocation.toLocalFile(), this );
    if( !opmlFile->open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        error() << "could not open OPML file for writing " << saveLocation.url();
        return;
    }

    QMap<QString,QString> headerData;
    //TODO: set header data such as date

    OpmlWriter *opmlWriter = new OpmlWriter( m_rootOutlines, headerData, opmlFile );
    connect( opmlWriter, SIGNAL(result(int)), SLOT(slotOpmlWriterDone(int)) );
    opmlWriter->run();
}

void
OpmlDirectoryModel::slotOpmlWriterDone( int result )
{
    Q_UNUSED( result )

    OpmlWriter *writer = qobject_cast<OpmlWriter *>( QObject::sender() );
    Q_ASSERT( writer );
    writer->device()->close();
    delete writer;
}

OpmlNodeType
OpmlDirectoryModel::opmlNodeType( const QModelIndex &idx ) const
{
    OpmlOutline *outline = static_cast<OpmlOutline *>( idx.internalPointer() );
    return opmlNodeType( outline );
}

OpmlNodeType
OpmlDirectoryModel::opmlNodeType( const OpmlOutline *outline ) const
{
    if( !outline || !outline->attributes().contains( "text" ) )
        return InvalidNode;

    if( !outline->attributes().contains( "type") )
        return CategoryNode;

    if( outline->attributes()["type"] == "rss" )
        return RssUrlNode;

    if( outline->attributes()["type"] == "include" )
        return IncludeNode;

    return UnknownNode;

}

void
OpmlDirectoryModel::slotAddOpmlAction()
{
    KDialog *dialog = new KDialog( The::mainWindow() );
    dialog->setCaption( i18n( "Add OPML" ) );
    dialog->setButtons( KDialog::Ok | KDialog::Cancel );
    QWidget *opmlAddWidget = new QWidget( dialog );
    Ui::AddOpmlWidget widget;
    widget.setupUi( opmlAddWidget );
    widget.urlEdit->setMode( KFile::File );
    dialog->setMainWidget( opmlAddWidget );

    if( dialog->exec() != QDialog::Accepted )
        return;

    QString url = widget.urlEdit->url().url();
    QString title = widget.titleEdit->text();
    debug() << QString( "creating a new OPML outline with url = %1 and title \"%2\"." ).arg( url, title );
    OpmlOutline *outline = new OpmlOutline();
    outline->addAttribute( "type", "include" );
    outline->addAttribute( "text", title );
    outline->addAttribute( "url", url );

    int newRow = m_rootOutlines.count();
    beginInsertRows( QModelIndex(), newRow, newRow );
    m_rootOutlines << outline;

    //Folder icon with down-arrow emblem
    m_imageMap.insert( outline, KIcon( "folder", 0, QStringList( "go-down" ) ).pixmap( 24, 24 ) );
    endInsertRows();

    saveOpml( m_rootOpmlUrl );

    delete dialog;
}

void
OpmlDirectoryModel::slotAddFolderAction()
{
    OpmlOutline *outline = new OpmlOutline();
    outline->addAttribute( "text", i18n( "New Folder" ) );
    int newRow = m_rootOutlines.count();

    beginInsertRows( QModelIndex(), newRow, newRow );
    m_rootOutlines << outline;
    m_imageMap.insert( outline, KIcon( "folder" ).pixmap( 24, 24 ) );
    endInsertRows();

    saveOpml( m_rootOpmlUrl );
}

bool
OpmlDirectoryModel::canFetchMore( const QModelIndex &parent ) const
{
    debug() << parent;
    //already fetched or just started?
    if( rowCount( parent ) || m_currentFetchingMap.values().contains( parent ) )
        return false;
    if( !parent.isValid() )
        return m_rootOutlines.isEmpty();

    OpmlOutline *outline = static_cast<OpmlOutline *>( parent.internalPointer() );

    return outline && ( outline->attributes().value( "type" ) == "include" );
}

void
OpmlDirectoryModel::fetchMore( const QModelIndex &parent )
{
    debug() << parent;
    if( m_currentFetchingMap.values().contains( parent ) )
    {
        error() << "trying to start second fetch job for same item";
        return;
    }
    KUrl urlToFetch;
    if( !parent.isValid() )
    {
        urlToFetch = m_rootOpmlUrl;
    }
    else
    {
        OpmlOutline *outline = static_cast<OpmlOutline *>( parent.internalPointer() );
        if( !outline )
            return;
        if( outline->attributes().value( "type" ) != "include" )
            return;
        urlToFetch = KUrl( outline->attributes()["url"] );
    }

    if( !urlToFetch.isValid() )
        return;

    OpmlParser *parser = new OpmlParser( urlToFetch );
    connect( parser, SIGNAL( outlineParsed( OpmlOutline * ) ),
             SLOT( slotOpmlOutlineParsed( OpmlOutline * ) ) );
    connect( parser, SIGNAL( doneParsing() ), SLOT( slotOpmlParsingDone() ) );

    m_currentFetchingMap.insert( parser, parent );

//    ThreadWeaver::Weaver::instance()->enqueue( parser );
    parser->run();
}

void
OpmlDirectoryModel::slotOpmlOutlineParsed( OpmlOutline *outline )
{
    OpmlParser *parser = qobject_cast<OpmlParser *>( QObject::sender() );
    QModelIndex idx = m_currentFetchingMap.value( parser );

    int beginRow = rowCount( idx );
    beginInsertRows( idx, beginRow, beginRow );

    //no reparenting required when the item is already parented.
    if( outline->isRootItem() )
    {
        if( !idx.isValid() )
        {
            m_rootOutlines << outline;
        }
        else
        {
            //children need to be manually added to include outlines
            OpmlOutline *parentOutline = static_cast<OpmlOutline *>( idx.internalPointer() );
            if( !parentOutline )
                return;

            parentOutline->addChild( outline );
            parentOutline->setHasChildren( true );
            outline->setParent( parentOutline );
        }
    }

    endInsertRows();

    //TODO: begin image fetch
    switch( opmlNodeType( outline ) )
    {
        case CategoryNode:
            m_imageMap.insert( outline, KIcon( "folder" ).pixmap( 24, 24 ) ); break;
        case IncludeNode:
        {
            m_imageMap.insert( outline,
                               KIcon( "folder", 0, QStringList( "go-down" ) ).pixmap( 24, 24 )
                             );
            break;
        }
        case RssUrlNode:
        default: break;
    }
}

void
OpmlDirectoryModel::slotOpmlParsingDone()
{
    OpmlParser *parser = qobject_cast<OpmlParser *>( QObject::sender() );
    m_currentFetchingMap.remove( parser );
    parser->deleteLater();
}
