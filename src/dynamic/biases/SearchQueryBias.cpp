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

#include "TrackSet.h"

#include <KLineEdit>
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

Dynamic::BiasPtr
Dynamic::SearchQueryBiasFactory:: createBias( QXmlStreamReader *reader )
{ return Dynamic::BiasPtr( new Dynamic::SearchQueryBias( reader ) ); }


// ----- SearchQueryBias --------

Dynamic::SearchQueryBias::SearchQueryBias( QString filter )
    : SimpleMatchBias()
    , m_unique( true )
    , m_filter( filter )
{ }

Dynamic::SearchQueryBias::SearchQueryBias( QXmlStreamReader *reader )
    : SimpleMatchBias()
{
    DEBUG_BLOCK;
    m_unique = reader->attributes().value( "unique" ).toString().toInt();

    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "filter" )
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

Dynamic::TrackSet
Dynamic::SearchQueryBias::matchingTracks( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount,
                                       Dynamic::TrackCollectionPtr universe ) const
{
    m_existingTracks.clear();
    if( m_unique )
    {
        for( int i = 0; i < position; i++ )
            m_existingTracks.append( playlist.at(i)->uidUrl() );
    }

    if( m_tracksValid )
    {
        Dynamic::TrackSet tracks( m_tracks );
        tracks.subtract( m_existingTracks );
        return tracks;
    }
    else
        return Dynamic::SimpleMatchBias::matchingTracks( position,
                                                         playlist,
                                                         contextCount,
                                                         universe );
}

void
Dynamic::SearchQueryBias::updateFinished()
{
    m_tracksValid = true;
    m_qm.reset();

    if( m_unique && !m_existingTracks.isEmpty() )
    {
        Dynamic::TrackSet tracks( m_tracks );
        tracks.subtract( m_existingTracks );
        emit resultReady( tracks );
    }
    else
        emit resultReady( m_tracks );
}

bool
Dynamic::SearchQueryBias::trackMatches( int position,
                                        const Meta::TrackList& playlist,
                                        int contextCount ) const
{
    Q_UNUSED( contextCount );

    if( m_unique )
    {
        for( int i = 0; i < position; i++ )
            if( playlist.at( i ) == playlist.at( position ) )
                return false;
    }

    return Dynamic::SimpleMatchBias::trackMatches( position,
                                                   playlist,
                                                   contextCount );
}

void
Dynamic::SearchQueryBias::toXml( QXmlStreamWriter *writer ) const
{
    if( m_unique )
        writer->writeAttribute("unique", "1");
    writer->writeTextElement( "filter", m_filter );
}

QString
Dynamic::SearchQueryBias::sName()
{
    return QLatin1String( "searchQueryBias" );
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
    {
        if( m_unique )
            return i18nc("Unique bias representation",
                         "Only once in the current playlist");
        else
            return i18nc("Random bias representation",
                         "Random songs");
    }
    else
    {
        if( m_unique )
            return i18nc("SearchQuery bias representation",
                         "Unique search for: %1").arg( m_filter );
        else
            return i18nc("SearchQuery bias representation",
                         "Search for: %1").arg( m_filter );
    }
}

QWidget*
Dynamic::SearchQueryBias::widget( QWidget* parent )
{
    QWidget *widget = new QWidget( parent );
    QVBoxLayout *layout = new QVBoxLayout( widget );

    QHBoxLayout *hLayout = new QHBoxLayout();
    QCheckBox *box = new QCheckBox();
    box->setChecked( unique() );
    QLabel *label = new QLabel( i18n("Unique songs") );
    label->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    label->setBuddy( box );
    hLayout->addWidget( box );
    hLayout->addWidget( label );
    layout->addLayout( hLayout );

    KLineEdit *edit = new KLineEdit( m_filter );
    layout->addWidget( edit );

    connect( box, SIGNAL( toggled( bool ) ),
             this, SLOT( setUnique( bool ) ) );
    connect( edit, SIGNAL( textChanged( const QString& ) ),
             this, SLOT( setFilter( const QString& ) ) );


    return widget;
}

bool
Dynamic::SearchQueryBias::unique() const
{
    return m_unique;
}

void
Dynamic::SearchQueryBias::setUnique( bool value )
{
    DEBUG_BLOCK;
    if( value == m_unique )
        return;

    m_unique = value;
    // setting "unique" does not invalidate the search results invalidate();
    emit changed( BiasPtr(this) );
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
    emit changed( BiasPtr(this) );
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

    connect( m_qm.data(), SIGNAL(newResultReady( QString, QStringList )),
             this, SLOT(updateReady( QString, QStringList )) );
    connect( m_qm.data(), SIGNAL(queryDone()),
             this, SLOT(updateFinished()) );
    m_qm.data()->run();
}

#include "SearchQueryBias.moc"

