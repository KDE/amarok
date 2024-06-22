/****************************************************************************************
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "SearchQueryBias"

#include "SearchQueryBias.h"

#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/support/TextualQueryFilter.h"
#include "dynamic/TrackSet.h"

#include <QLineEdit>

#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


QString
Dynamic::SearchQueryBiasFactory::i18nName() const
{ return i18nc("Name of the \"SearchQuery\" bias", "Search"); }

QString
Dynamic::SearchQueryBiasFactory::name() const
{ return Dynamic::SearchQueryBias::sName(); }

QString
Dynamic::SearchQueryBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"SearchQuery\" bias",
               "The \"SearchQuery\" bias adds tracks that are\n"
               "found by a search query. It uses the same search\n"
               "query as the collection browser."); }

Dynamic::BiasPtr
Dynamic::SearchQueryBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::SearchQueryBias() ); }

// ----- SearchQueryBias --------

Dynamic::SearchQueryBias::SearchQueryBias( const QString &filter )
    : SimpleMatchBias()
    , m_filter( filter )
{ }

void
Dynamic::SearchQueryBias::fromXml( QXmlStreamReader *reader )
{
    DEBUG_BLOCK;

    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringView name = reader->name();
            if( name == QStringLiteral("filter") )
                m_filter = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else
            {
                debug()<<"Unexpected xml start element"<<reader->name()<<"in input";
                reader->skipCurrentElement();
            }
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }
}

void
Dynamic::SearchQueryBias::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( QStringLiteral("filter"), m_filter );
}

QString
Dynamic::SearchQueryBias::sName()
{
    return QStringLiteral( "searchQueryBias" );
}

QString
Dynamic::SearchQueryBias::name() const
{
    return Dynamic::SearchQueryBias::sName();
}

QString
Dynamic::SearchQueryBias::toString() const
{
    if( m_filter.isEmpty() )
        return i18nc("Random bias representation",
                     "Random tracks");
    else
        return i18nc("SearchQuery bias representation",
                     "Search for: %1", m_filter );
}

QWidget*
Dynamic::SearchQueryBias::widget( QWidget* parent )
{
    QWidget *widget = new QWidget( parent );
    QVBoxLayout *layout = new QVBoxLayout( widget );

    QLineEdit *edit = new QLineEdit( m_filter );
    layout->addWidget( edit );

    connect( edit, &QLineEdit::textChanged,
             this, &SearchQueryBias::setFilter );

    return widget;
}

QString
Dynamic::SearchQueryBias::filter() const
{
    return m_filter;
}

void
Dynamic::SearchQueryBias::setFilter( const QString &filter )
{
    DEBUG_BLOCK;
    if( filter == m_filter )
        return;

    m_filter = filter;
    invalidate();
    Q_EMIT changed( BiasPtr(this) );
}

void
Dynamic::SearchQueryBias::newQuery()
{
    DEBUG_BLOCK;

    // ok, I need a new query maker
    m_qm.reset( CollectionManager::instance()->queryMaker() );
    Collections::addTextualFilter( m_qm.data(), m_filter );
    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );

    connect( m_qm.data(), &Collections::QueryMaker::newResultReady,
             this, &SearchQueryBias::updateReady, Qt::QueuedConnection );
    connect( m_qm.data(), &Collections::QueryMaker::queryDone,
             this, &SearchQueryBias::updateFinished, Qt::QueuedConnection );
    m_qm.data()->run();
}


