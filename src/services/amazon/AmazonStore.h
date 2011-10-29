/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
 * The Amazon store in based upon the Magnatune store in Amarok,                        *
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#ifndef AMAZONSTORE_H
#define AMAZONSTORE_H

#include "AmazonCollection.h"
#include "AmazonItemTreeModel.h"
#include "AmazonItemTreeView.h"
#include "AmazonMeta.h"

#include "../ServiceBase.h"
#include "ServiceSqlRegistry.h"

#include <QComboBox>
#include <QModelIndex>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTreeView>
#include <QUrl>

#include <threadweaver/ThreadWeaver.h>

class AmazonMetaFactory;

class AmazonServiceFactory : public ServiceFactory
{
    Q_OBJECT

public:
    AmazonServiceFactory( QObject* parent, const QVariantList &args );
    virtual ~AmazonServiceFactory() {}

    virtual void init();
    virtual QString name();
    virtual KConfigGroup config();

    virtual bool possiblyContainsTrack( const KUrl &url ) const { return url.url().contains( "amazon.", Qt::CaseInsensitive ); } // XXX: ???
};


class AmazonStore : public ServiceBase
{
    Q_OBJECT

public:
    /**
    * Constructor
    */
    AmazonStore( AmazonServiceFactory* parent, const char *name );

    /**
    * Destructor
    */
    ~AmazonStore();

    virtual Collections::Collection* collection() { return m_collection; }
    void polish();

public slots:
    /**
    * Adds the currently selected item to the cart.
    */
    void addToCart();

    /**
    * Shows a dialog with the cart contents.
    */
    void viewCart();

    /**
    * Checks the none-empty cart out.
    */
    void checkout();

    /**
    * React to a double click on an item.
    */
    void itemDoubleClicked( QModelIndex index );

    /**
    * Activates buttons required to interact with the currently selected item.
    */
    void itemSelected( QModelIndex index );

    /**
    * The user requested us to query the service.
    * @param request string to search for.
    */
    void newSearchRequest( const QString request );

    /**
    * The user wants to go to another page of search results.
    * @param i page to go to.
    */
    void newSpinBoxSearchRequest( int i );

private:
    /**
    * Helper function. Creates a valid request URL for the Amazon service.
    * @param request string to search for.
    */
    QUrl createRequestUrl( const QString request );

    /**
    * Inits the top part of the Amazon store browser view with its widgets.
    */
    void initTopPanel();

    /**
    * Inits the Amazon store browser view with its widgets.
    */
    void initView();

    AmazonMetaFactory* m_metaFactory;

    Collections::AmazonCollection* m_collection;

    ServiceSqlRegistry* m_registry;

    QPushButton* m_addToCartButton;
    QPushButton* m_removeFromCartButton;
    QPushButton* m_viewCartButton;
    QPushButton* m_checkoutButton;

    QSpinBox* m_resultpageSpinBox;

    AmazonItemTreeView* m_itemView;
    AmazonItemTreeModel* m_itemModel;

    QModelIndex m_selectedIndex;

private slots:
    /**
    * Parse the API reply XML document.
    */
    void parseReply( KJob* requestJob );

    /**
    * Clean up after parsing the API reply XML document, update collection.
    */
    void parsingDone( ThreadWeaver::Job* parserJob );

    /**
    * Clean up after parsing the API reply XML document.
    */
    void parsingFailed( ThreadWeaver::Job* parserJob );

    /**
    * Opens the shopping cart with the cart items in the default browser.
    */
    void openCheckoutUrl( KJob* requestJob );
};

#endif // AMAZONSTORE_H
