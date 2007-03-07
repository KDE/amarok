/*
  Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef AMAROKSERVICEBASE_H
#define AMAROKSERVICEBASE_H


#include "amarok.h"
#include "servicemodelitembase.h"
#include "servicemodelbase.h"

#include <khtml_part.h>
//#include <klistwidget.h>
#include <kvbox.h>

#include <QPushButton>
#include <QSplitter>
#include <QTreeView>


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
    ServiceBase( QString name );
    
    /**
     * Destructor
     */
    ~ServiceBase() { }


    QString getName();

    void setShortDescription( QString shortDescription );
    QString getShortDescription();
    void setLongDescription( QString longDescription );
    QString getLongDescription();
    void setIcon( QIcon pixmap );
    QIcon getIcon();
    void setModel( ServiceModelBase * model );
    ServiceModelBase * getModel();


signals:

    void home();

protected slots:

    /**
     * Toggles the info area on and off
     * @param show If true the info box is shown, if false it is hidden
     */
    void showInfo(bool show);

    void homeButtonClicked();

    void itemActivated ( const QModelIndex & index );


protected:

    

    static ServiceBase *s_instance;

    QSplitter *m_mainSplitter;
    QTreeView *m_contentView;
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

    void addToPlaylist( ServiceModelItemBase * item );

    ServiceModelBase * m_model;

};


#endif
