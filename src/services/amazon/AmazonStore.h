/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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
#include "AmazonInfoParser.h"
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
#include <QStack>

#include <QAction>
#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/Queue>

class AmazonMetaFactory;
class AmazonInfoParser;

class AmazonWantCountryWidget;

class AmazonServiceFactory : public ServiceFactory
{
    Q_OBJECT

public:
    AmazonServiceFactory( QObject* parent, const QVariantList &args );
    virtual ~AmazonServiceFactory() {}

    virtual void init();
    virtual QString name();
    virtual KConfigGroup config();

    virtual bool possiblyContainsTrack( const QUrl &url ) const { return url.url().contains( "amazon.", Qt::CaseInsensitive ); } // XXX: ???
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

    /**
     * Convert an ISO 3166 two-letter country code to Amazon
     * top level domain
     *
     * @returns a TLD for corresponding Amazon store, or "none"
     */
    static QString iso3166toAmazon(const QString& country);

public Q_SLOTS:
    /**
    * Adds the currently selected item to the cart.
    */
    void addToCart();

    /**
    * Shows a dialog with the cart contents.
    */
    void viewCart();

    /**
    * Checks the non-empty cart out.
    */
    void checkout();

    /**
    * Checks the currently selected item out directly, without adding it to the local Amarok shopping cart.
    */
    void directCheckout();

    /**
    * React to a double click on an item.
    * @param index The QModelIndex of the item.
    */
    void itemDoubleClicked( QModelIndex index );

    /**
    * Activates buttons required to interact with the currently selected item and updates the context view.
    * @param index The QModelIndex of the item.
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

    /**
    * Starts a search for the album of a track the given QModelIndex represents.
    * @param index The QModelIndex for the track.
    */
    void searchForAlbum( QModelIndex index );

private:
    /**
    * Helper method. Creates a valid request URL for the Amazon service.
    * @param request string to search for.
    */
    QUrl createRequestUrl( const QString request );

    /**
    * Inits the top part of the Amazon store browser view with its widgets.
    */
    void initTopPanel();

    /**
     * Initializes the bottom panel of Amazon store browser.
     *
     * Currently this contains nothing, except for possible "Select country"
     * widget.
     */
    void initBottomPanel();

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

    AmazonWantCountryWidget* m_wantCountryWidget;

    QSpinBox* m_resultpageSpinBox;
    QAction * m_forwardAction;
    QAction * m_backwardAction;

    bool m_isNavigation;

    QString m_lastSearch;
    QStack<QString> m_backStack;
    QStack<QString> m_forwardStack;

    AmazonItemTreeView* m_itemView;
    AmazonItemTreeModel* m_itemModel;

    QModelIndex m_selectedIndex;

    AmazonInfoParser* m_amazonInfoParser;

private Q_SLOTS:
    /**
    * Parse the API reply XML document.
    */
    void parseReply( KJob* requestJob );

    /**
    * Clean up after parsing the API reply XML document, update collection.
    */
    void parsingDone( ThreadWeaver::JobPointer parserJob );

    /**
    * Clean up after parsing the API reply XML document.
    */
    void parsingFailed( ThreadWeaver::JobPointer parserJob );

    /**
    * Go backward in Amazon store.
    */
    void back();

    /**
    * Go forward in Amazon store.
    */
    void forward();

    /**
     * Country has been updated in the configuration
     */
    void countryUpdated();
};

#endif // AMAZONSTORE_H
