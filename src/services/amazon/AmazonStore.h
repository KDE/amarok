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
#include <QMutex>
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
    // cart handling
    void addToCart();
    void viewCart();
    void checkout();

    void itemSelected( QModelIndex index );

    void newSearchRequest( const QString request );
    void newSpinBoxSearchRequest( int i );

private:
    void createRequestUrl( const QString request );
    void initTopPanel();
    void initView();

    AmazonMetaFactory* m_metaFactory;

    Collections::AmazonCollection* m_collection;

    ServiceSqlRegistry* m_registry;

    QUrl m_requestUrl;
    QString m_tempFileName;
    KIO::FileCopyJob* m_requestJob;

    QPushButton* m_addToCartButton;
    QPushButton* m_removeFromCartButton;
    QPushButton* m_viewCartButton;
    QPushButton* m_checkoutButton;

    QComboBox* m_searchSelectionBox;
    QSpinBox* m_resultpageSpinBox;

    AmazonItemTreeView* m_itemView;
    AmazonItemTreeModel* m_itemModel;

    QModelIndex m_selectedIndex;

    QMutex m_ApiMutex;

private slots:
    void parseReply( KJob* requestJob );
    void parsingDone( ThreadWeaver::Job* parserJob );
    void parsingFailed( ThreadWeaver::Job* parserJob );
    void openCheckoutUrl( KJob* requestJob );
};

#endif // AMAZONSTORE_H
