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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef AMAROKSERVICEBASE_H
#define AMAROKSERVICEBASE_H


#include "amarok.h"
//#include "servicemodelitembase.h"
//#include "servicemodelbase.h"

#include "infoparserbase.h"

#include "../collectionbrowser/singlecollectiontreeitemmodel.h"
#include "../collectionbrowser/collectiontreeitem.h"
#include "../collectionbrowser/collectiontreeview.h"

#include <khtml_part.h>
//#include <klistwidget.h>
#include <kvbox.h>

#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
//#include <QTreeView>



/**
A very basic composite widget used as a base for building service browsers.

@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class ServiceBase : public KVBox
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
    ~ServiceBase() { }


    QString getName();

    void setShortDescription( const QString &shortDescription );
    QString getShortDescription();
    void setLongDescription( const QString &longDescription );
    QString getLongDescription();
    void setIcon( const QIcon &icon );
    QIcon getIcon();
    void setModel( SingleCollectionTreeItemModel * model );
    SingleCollectionTreeItemModel * getModel();

    virtual void polish() = 0;
    virtual bool updateContextView() { return false; }

public slots:

    //void treeViewSelectionChanged( const QItemSelection & selected );
    void infoChanged ( const QString &infoHtml );


signals:

    void home();
    void selectionChanged ( CollectionTreeItem * );
    

protected slots:

    /**
     * Toggles the info area on and off
     * @param show If true the info box is shown, if false it is hidden
     */
    void showInfo(bool show);

    void homeButtonClicked();

    void itemActivated ( const QModelIndex & index );

    void itemSelected( CollectionTreeItem * item  );


protected:

    

    static ServiceBase *s_instance;

    QSplitter *m_mainSplitter;
    CollectionTreeView *m_contentView;
    KHTMLPart   *m_infoBox;

    QPushButton *m_homeButton;

    KVBox       *m_topPanel;
    KVBox       *m_bottomPanel;
    bool         m_isInfoShown;
    bool         m_polished;

    QString      m_name;
    QString      m_shortDescription;
    QString      m_longDescription;
    QIcon        m_icon;

    KUrl::List   m_urlsToInsert;

    InfoParserBase * m_infoParser;

    //void addToPlaylist( CollectionTreeItem * item );

   

private: // need to move stuff here
     SingleCollectionTreeItemModel * m_model;
     QSortFilterProxyModel * m_filterModel;

};


#endif
