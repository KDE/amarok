/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#ifndef AMAROKSERVICEBASE_H
#define AMAROKSERVICEBASE_H


#include "Amarok.h"
//#include "servicemodelitembase.h"
//#include "servicemodelbase.h"

#include "infoparserbase.h"
#include "ServiceMetaBase.h"
#include "collection/CollectionManager.h"

#include "amarok_export.h"
#include "../collectionbrowser/SingleCollectionTreeItemModel.h"
#include "../collectionbrowser/CollectionTreeItem.h"
#include "ServiceCollectionTreeView.h"
#include "plugin/plugin.h"

#include <khtml_part.h>
//#include <klistwidget.h>
#include <kvbox.h>

#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
//#include <QTreeView>

#include <KPluginInfo>

class ServiceBase;
class SearchWidget;
class KMenuBar;

class AMAROK_EXPORT ServiceFactory : public QObject, public Amarok::Plugin, public TrackProvider
{
    Q_OBJECT
    public:
        ServiceFactory();
        virtual ~ServiceFactory();

        virtual void init() = 0;
        virtual QString name() = 0;
        virtual KConfigGroup config() = 0;
        virtual KPluginInfo info() = 0;

        virtual bool possiblyContainsTrack( const KUrl &url ) const { Q_UNUSED( url ); return false; }
        virtual Meta::TrackPtr trackForUrl( const KUrl &url );

        void clearActiveServices();

    signals:
        void newService( class ServiceBase *newService );

    protected:
        QList<ServiceBase *> m_activeServices;
};


/**
A very basic composite widget used as a base for building service browsers.

@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class AMAROK_EXPORT ServiceBase : public KVBox
{
    Q_OBJECT

public:
     

     /**
      * Constructor
      */
    ServiceBase( const QString &name );
    
    /**
     * Destructor
     */
    ~ServiceBase();


    QString getName();

    void setShortDescription( const QString &shortDescription );
    QString getShortDescription();
    void setLongDescription( const QString &longDescription );
    QString getLongDescription();
    void setIcon( const QIcon &icon );
    QIcon getIcon();
    void setModel( SingleCollectionTreeItemModel * model );
    SingleCollectionTreeItemModel * getModel();

    void setPlayableTracks( bool playable );
    void setInfoParser( InfoParserBase * infoParser );
    InfoParserBase * infoParser();
    
    virtual Collection * collection() = 0;
    
    virtual void polish() = 0;
    virtual bool updateContextView() { return false; }

    void setFilter( const QString &filter );

    /**
     * Returns a list of the messages that the current service accepts. Default impelentation does not
     * accept any.
     * @return A string containing a description of accepted messages.
     */
    virtual QString messages();

    /**
     * Send amessage to this service. Default implementation returns an error as no messages are
     * accepted
     * @param message The message to send to the service
     * @return The reply to the message
     */
    virtual QString sendMessage( const QString &message );

    //virtual void reset() = 0;

public slots:
    //void treeViewSelectionChanged( const QItemSelection & selected );
    void infoChanged ( const QString &infoHtml );

    void sortByArtistAlbum();

signals:
    void home();
    void selectionChanged ( CollectionTreeItem * );
    
protected slots:
    void homeButtonClicked();
    void itemActivated ( const QModelIndex & index );
    void itemSelected( CollectionTreeItem * item  );

    void sortByArtist();

    void sortByAlbum();
    void sortByGenreArtist();
    void sortByGenreArtistAlbum();

protected:
    virtual void generateWidgetInfo( const QString &html = QString() ) const;
    
    static ServiceBase *s_instance;
    ServiceCollectionTreeView *m_contentView;

    QPushButton *m_homeButton;

    KVBox       *m_topPanel;
    KVBox       *m_bottomPanel;
    bool         m_polished;

    QString      m_name;
    QString      m_shortDescription;
    QString      m_longDescription;
    QIcon        m_icon;

    KUrl::List   m_urlsToInsert;

    InfoParserBase * m_infoParser;

    KMenuBar *m_menubar;
    QMenu *m_filterMenu;
    SearchWidget * m_searchWidget;

    //void addToPlaylist( CollectionTreeItem * item );

private: // need to move stuff here
     SingleCollectionTreeItemModel * m_model;
     QSortFilterProxyModel * m_filterModel;
};


#endif
