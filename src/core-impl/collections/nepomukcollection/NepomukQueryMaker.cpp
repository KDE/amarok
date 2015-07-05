/****************************************************************************************
 * Copyright (c) 2013 Edward Toroshchin <amarok@hades.name>
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

#include "NepomukQueryMaker.h"

#include "NepomukInquirer.h"
#include "NepomukParser.h"
#include "NepomukSelectors.h"

#include "meta/NepomukAlbum.h"
#include "meta/NepomukArtist.h"
#include "meta/NepomukComposer.h"
#include "meta/NepomukTrack.h"

#include "core/support/Debug.h"

#include <ThreadWeaver/Queue>
#include <ThreadWeaver/Job>

#include <QStack>

namespace Collections {

/**
 * This class holds private data for a NepomukQueryMaker instance and provides
 * some helper routines.
 */
class NepomukQueryMakerPrivate
{
public:
    QString info; ///< a text description of the query (for debug purposes)
    QueryMaker::QueryType type;
    QStringList customSelectors; ///< SPARQL selectors for custom queries @see constructQuery
    QString filters; ///< SPARQL filters
    bool filterNeedsConjunction; ///< true, if a logical operator (AND or OR) must be added to filters
    QString extra; ///< extra SPARQL clauses (ORDER BY, LIMIT, etc.)
    QStack<QString> logic; ///< stack of logic operators (AND or OR) @see popLogic and pushLogic
    bool distinct; ///< if a "DISTINCT" operator must be present in the SPARQL query

    /**
     * @return a text representation of SPARQL selectors for the current query
     * type
     */
    QString constructSelector();

    /**
     * @return the final SPARQL query
     */
    QString constructQuery();

    /**
     * Convert a single Meta::val* value to a SPARQL selector.
     *
     * Won't (and shouldn't) work for a mask of Meta::val* values.
     */
    QString valueToSelector(qint64 value);

    /**
     * Convert a ReturnFunction type to a SPARQL function.
     */
    QString returnFunctionSelector(QueryMaker::ReturnFunction function, qint64 value);

    /**
     * Add a SPARQL filter term to the current filter expression along with
     * correct logic operator if needed.
     */
    void addFilter(QString);

    /**
     * Add a filter term that does not match anything
     */
    void matchNothing();

    /**
     * Escape a string literal to be included in a SPARQL query
     */
    QString escape(QString);

    /**
     * Convert a matchBegin/matchEnd flag combination to a SPARQL logical string
     * function, like this:
     *
     * - !matchBegin && !matchEnd: CONTAINS( haystack, needle )
     * - !matchBegin &&  matchEnd: STRENDS( haystack, needle )
     * -  matchBegin && !matchEnd: STRSTARTS( haystack, needle )
     * -  matchBegin &&  matchEnd: haystack = needle
     *
     * The result is returned as a string with "%1" and "%2" placeholders for
     * the haystack and the needle respectively.
     */
    QString stringOperation(bool matchBegin, bool matchEnd);

    /**
     * Convert a NumberComparison operator to a SPARQL operator
     */
    QString numberOperator(QueryMaker::NumberComparison);

    /**
     * Begin a new filtering subexpression with the given logical operator.
     *
     * All filtration terms added by subsequent calls to addFilter() will be
     * enclosed in braces and separated by the given operator.
     *
     * Example:
     *
     * pushLogic("OR")
     * addFilter("a")
     * pushLogic("AND")
     * addFilter("b")
     * addFilter("c")
     * popLogic()
     * popLogic()
     *
     * will yield the following filter expression:
     *
     * (a OR (b AND c))
     */
    void pushLogic(QString oper);

    /**
     * End a filtering subexpression.
     *
     * @see pushLogic
     */
    void popLogic();

    NepomukInquirer *inquirer;
};

QString
NepomukQueryMakerPrivate::constructSelector()
{
    // These are different SPARQL selectors for different sets of data
    // Q macro is a question mark ("track" is a name, "?track" is a selector)
    static const QString trackSelector(Q NS_track " " Q NS_trackTitle " " Q NS_trackUrl " "
                                       Q NS_trackType " (concat(str(" Q NS_trackLengthSeconds "), \"000\") AS " Q NS_trackLength ") " // don't ask
                                       "(" Q NS_trackBitrateBPS " / 1000 AS " Q NS_trackBitrate ") "
                                       Q NS_trackNumber " " Q NS_trackBPM " " Q NS_trackComment " "
                                       Q NS_trackSampleRate " " Q NS_trackFileSize " "
                                       Q NS_trackGain " " Q NS_trackPeakGain " "
                                       Q NS_trackModifyDate " " Q NS_trackCreateDate " ");
    static const QString artistSelector(Q NS_artist " " Q NS_artistName " ");
    static const QString albumSelector(Q NS_album " " Q NS_albumTitle " " Q NS_albumGain " " Q NS_albumPeakGain " ");
    static const QString genreSelector(Q NS_genre " ");
    static const QString composerSelector(Q NS_composer " " Q NS_composerName " ");
    static const QString yearSelector(Q NS_date " (IF( bound(" Q NS_date "), year(" Q NS_date "), 0 ) AS " Q NS_year ") ");

    static const QString allSelector( trackSelector
                                    + artistSelector
                                    + albumSelector
                                    + genreSelector
                                    + composerSelector
                                    + yearSelector );

    switch(type)
    {
        case QueryMaker::None:
            return QString();
        case QueryMaker::Track:
            return allSelector;
        case QueryMaker::Artist:
            return artistSelector;
        case QueryMaker::Album:
            return albumSelector;
        case QueryMaker::AlbumArtist:
            return artistSelector;
        case QueryMaker::Genre:
            return genreSelector;
        case QueryMaker::Composer:
            return composerSelector;
        case QueryMaker::Year:
            return yearSelector;
        case QueryMaker::Custom:
            return customSelectors.join(" ");
        case QueryMaker::Label:
            return QString();
    }
    warning() << "unknown QueryMaker type " << type;
    return QString();
}

QString
NepomukQueryMakerPrivate::constructQuery()
{
    // This is the big SPARQL relationship constraint clause that defines all
    // the relationships between objects we want to know about. Basically this
    // defines the semantics of the SPARQL selectors we're using everywhere
    // Q macro is a question mark ("track" is a name, "?track" is a selector)
    static const QString queryTemplate(
                  "SELECT %1 {"
                  " " Q NS_track " a nfo:Audio ;"
                  "        nie:title " Q NS_trackTitle " ;"
                  "        nie:url " Q NS_trackUrl " ."
                  " OPTIONAL { " Q NS_track " nmm:performer " Q NS_artist " ."
                  "            " Q NS_artist " nco:fullname " Q NS_artistName " . }"
                  " OPTIONAL { " Q NS_track " nmm:musicAlbum " Q NS_album " ."
                  "            " Q NS_album " nie:title " Q NS_albumTitle " ."
                  "            OPTIONAL { " Q NS_album " nmm:albumGain " Q NS_albumGain " . }"
                  "            OPTIONAL { " Q NS_album " nmm:albumPeakGain " Q NS_albumPeakGain " . } }"
                  " OPTIONAL { " Q NS_track " nmm:genre " Q NS_genre " . }"
                  " OPTIONAL { " Q NS_track " nmm:composer " Q NS_composer " ."
                  "            " Q NS_composer " nco:fullname " Q NS_composerName " . }"
                  " OPTIONAL { " Q NS_track " nmm:releaseDate " Q NS_year " . }"
                  " OPTIONAL { " Q NS_track " nfo:codec " Q NS_trackType " . }"
                  " OPTIONAL { " Q NS_track " nfo:duration " Q NS_trackLengthSeconds " . }"
                  " OPTIONAL { " Q NS_track " nfo:averageBitrate " Q NS_trackBitrateBPS " . }"
                  " OPTIONAL { " Q NS_track " nmm:trackNumber " Q NS_trackNumber " . }"
                  " OPTIONAL { " Q NS_track " nmm:beatsPerMinute " Q NS_trackBPM " . }"
                  " OPTIONAL { " Q NS_track " nie:comment " Q NS_trackComment " . }"
                  " OPTIONAL { " Q NS_track " nfo:sampleRate " Q NS_trackSampleRate " . }"
                  " OPTIONAL { " Q NS_track " nfo:fileSize " Q NS_trackFileSize " . }"
                  " OPTIONAL { " Q NS_track " nie:contentSize " Q NS_trackFileSize " . }"
                  " OPTIONAL { " Q NS_track " nmm:trackGain " Q NS_trackGain " . }"
                  " OPTIONAL { " Q NS_track " nmm:trackPeakGain " Q NS_trackPeakGain " . }"
                  " OPTIONAL { " Q NS_track " nie:modified " Q NS_trackModifyDate " . }"
                  " OPTIONAL { " Q NS_track " nie:created " Q NS_trackCreateDate " . }"
                  " %2 " // a placeholder for filter expression
                  " }" );

    // This is a special query for labels
    static const QString labelQueryTemplate(
                  "SELECT DISTINCT " Q NS_tag " " Q NS_tagLabel " {"
                  " " Q NS_track " a nfo:Audio ."
                  " " Q NS_track " nao:hasTag " Q NS_tag " ."
                  " " Q NS_tag " nao:prefLabel " Q NS_tagLabel " ."
                  " }" );

    if( type == QueryMaker::None )
    {
        error() << "requested to perform a none-query";
        return QString();
    }
    else if( type == QueryMaker::Label )
    {
        return labelQueryTemplate;
    }

    QString filter;
    if( !filters.isEmpty() ) filter = QString( "FILTER( %1 )" ).arg( filters );

    QString selector( constructSelector() );

    if( distinct )
        selector = QString("DISTINCT ") + selector;

    return queryTemplate.arg(selector).arg(filter) + extra;
}

/**
 * Helper class to construct the map in valueToSelector
 */
template< class K, class V >
class ConstMap: public QHash< K, V >
{
public:
    inline ConstMap &add( const K &key, const V &val )
    {
        this->insert( key, val );
        return *this;
    }
};

QString
NepomukQueryMakerPrivate::valueToSelector(qint64 bitValue)
{
    typedef ConstMap< qint64, QString > ValueMap;
    static const ValueMap map = ValueMap()
        .add( Meta::valUrl, Q NS_trackUrl )
        .add( Meta::valTitle, Q NS_trackTitle )
        .add( Meta::valArtist, Q NS_artistName )
        .add( Meta::valAlbum, Q NS_albumTitle )
        .add( Meta::valGenre, Q NS_genre )
        .add( Meta::valComposer, Q NS_composerName )
        .add( Meta::valYear, "IF( bound(" Q NS_date "), year(" Q NS_date "), 0 )" )
        .add( Meta::valComment, Q NS_trackComment )
        .add( Meta::valTrackNr, Q NS_trackNumber )
        .add( Meta::valDiscNr, Q NS_trackDiscNumber )
        .add( Meta::valBpm, Q NS_trackBPM )
        .add( Meta::valLength, "(concat(str(" Q NS_trackLengthSeconds "), \"000\"))" ) // I said, don't ask
        .add( Meta::valBitrate, "(" Q NS_trackBitrateBPS " / 1000)" )
        .add( Meta::valSamplerate, Q NS_trackSampleRate )
        .add( Meta::valFilesize, Q NS_trackFileSize )
        .add( Meta::valFormat, Q NS_trackType )
        .add( Meta::valCreateDate, Q NS_trackCreateDate )
        .add( Meta::valScore, Q NS_trackScore )
        .add( Meta::valRating, Q NS_trackRating )
        .add( Meta::valFirstPlayed, Q NS_trackFirstPlayed )
        .add( Meta::valLastPlayed, Q NS_trackLastPlayed )
        .add( Meta::valPlaycount, Q NS_trackPlaycount )
        .add( Meta::valUniqueId, "STR(" Q NS_track ")" )
        .add( Meta::valTrackGain, Q NS_trackGain )
        .add( Meta::valTrackGainPeak, Q NS_trackPeakGain )
        .add( Meta::valAlbumGain, Q NS_albumGain )
        .add( Meta::valAlbumGainPeak, Q NS_albumPeakGain )
        .add( Meta::valAlbumArtist, Q NS_albumArtist )
        .add( Meta::valLabel, Q NS_label )
        .add( Meta::valModified, Q NS_trackModifyDate );

    return map.value( bitValue, Q NS__unknownValue ); // this will be just an unset SPARQL selector (e.g. NULL values)
}

QString
NepomukQueryMakerPrivate::returnFunctionSelector(QueryMaker::ReturnFunction function, qint64 value)
{
    QString valSelector(valueToSelector(value));

    if( valSelector != Q NS__unknownValue )
    {
        switch(function)
        {
            case QueryMaker::Count:
                return QString("COUNT(DISTINCT %1)").arg(valSelector);
            case QueryMaker::Sum:
                return QString("SUM(%1)").arg(valSelector);
            case QueryMaker::Max:
                return QString("MAX(%1)").arg(valSelector);
            case QueryMaker::Min:
                return QString("MIN(%1)").arg(valSelector);
        }
    }
    return Q NS__unknownFunction;
}

void
NepomukQueryMakerPrivate::addFilter( QString expression )
{
    if( filterNeedsConjunction )
        filters += logic.top();
    filters += expression;
    filterNeedsConjunction = true;
}

void
NepomukQueryMakerPrivate::matchNothing()
{
    filters = '0';
    filterNeedsConjunction = true;
}

QString
NepomukQueryMakerPrivate::escape( QString string )
{
    return QString("\"\"\"%1\"\"\"").arg(string); // TODO: use some actual function from Nepomuk API
}

QString
NepomukQueryMakerPrivate::stringOperation(bool matchBegin, bool matchEnd)
{
    // We'll convert matchBegin and matchEnd to an index in the following array:
    //                             <result>                     <matchBegin> <matchEnd>
    static const QString map[] = { "CONTAINS( str(%1), %2 )",   //   0           0
                                   "STRENDS( str(%1), %2 )",    //   0           1
                                   "STRSTARTS( str(%1), %2 )",  //   1           0
                                   "str(%1) = %2" };            //   1           1
    return map[ !!matchBegin * 2 + !!matchEnd ];
}

QString
NepomukQueryMakerPrivate::numberOperator(QueryMaker::NumberComparison oper)
{
    switch( oper )
    {
        case QueryMaker::GreaterThan:
            return ">";
        case QueryMaker::LessThan:
            return "<";
        case QueryMaker::Equals:
            return "=";
    }
    warning() << "unknown number comparison" << oper;
    return "="; // couldn't be worse
}

void
NepomukQueryMakerPrivate::pushLogic( QString oper )
{
    if( filterNeedsConjunction )
        filters += logic.top();
    filters += '(';
    logic.push( QString(" %1 ").arg( oper ) );
    filterNeedsConjunction = false;
}

void
NepomukQueryMakerPrivate::popLogic()
{
    filters += ')';
    logic.pop();
}

NepomukQueryMaker::NepomukQueryMaker( NepomukCollection *collection )
    : d(new NepomukQueryMakerPrivate)
    , myCollection( collection )
{
    Q_ASSERT( collection );

    d->type = QueryType(0);
    d->filterNeedsConjunction = false;
    d->inquirer = 0;
    d->logic.push(" && ");
    d->distinct = true;
}

NepomukQueryMaker::~NepomukQueryMaker()
{
    delete d;
    d = 0;
}

void
NepomukQueryMaker::abortQuery()
{
    // TODO
}

void
NepomukQueryMaker::run()
{
    DEBUG_BLOCK
    debug() << "running the following query" << d->info;

    QString query(d->constructQuery());
    debug() << "translated into" << query;

    std::auto_ptr<NepomukParser> parser;

    switch(d->type)
    {
        case None:
            debug() << "QueryMaker requested to run a None-query";
            break;
        case Track:
            parser.reset( new NepomukTrackParser( myCollection ) );
            connect(parser.get(), SIGNAL(newResultReady(Meta::TrackList)),
                                  SIGNAL(newResultReady(Meta::TrackList)));
            break;
        case Artist:
            parser.reset( new NepomukArtistParser( myCollection ) );
            connect(parser.get(), SIGNAL(newResultReady(Meta::ArtistList)),
                                  SIGNAL(newResultReady(Meta::ArtistList)));
            break;
        case Album:
            parser.reset( new NepomukAlbumParser( myCollection ) );
            connect(parser.get(), SIGNAL(newResultReady(Meta::AlbumList)),
                                  SIGNAL(newResultReady(Meta::AlbumList)));
            break;
        case AlbumArtist:
            parser.reset( new NepomukArtistParser( myCollection ) );
            connect(parser.get(), SIGNAL(newResultReady(Meta::ArtistList)),
                                  SIGNAL(newResultReady(Meta::ArtistList)));
            break;
        case Genre:
            parser.reset( new NepomukGenreParser( myCollection ) );
            connect(parser.get(), SIGNAL(newResultReady(Meta::GenreList)),
                                  SIGNAL(newResultReady(Meta::GenreList)));
            break;
        case Composer:
            parser.reset( new NepomukComposerParser( myCollection ) );
            connect(parser.get(), SIGNAL(newResultReady(Meta::ComposerList)),
                                  SIGNAL(newResultReady(Meta::ComposerList)));
            break;
        case Year:
            parser.reset( new NepomukYearParser( myCollection ) );
            connect(parser.get(), SIGNAL(newResultReady(Meta::YearList)),
                                  SIGNAL(newResultReady(Meta::YearList)));
            break;
        case Custom:
            parser.reset( new NepomukCustomParser( myCollection ) );
            connect(parser.get(), SIGNAL(newResultReady(QStringList)),
                                  SIGNAL(newResultReady(QStringList)));
            break;
        case Label:
            parser.reset( new NepomukLabelParser( myCollection ) );
            connect(parser.get(), SIGNAL(newResultReady(Meta::LabelList)),
                                  SIGNAL(newResultReady(Meta::LabelList)));
            break;
    }

    if( !parser.get() )
    {
        emit queryDone();
        return;
    }

    d->inquirer = new NepomukInquirer(query, parser);
    connect(d->inquirer, SIGNAL(done(ThreadWeaver::JobPointer)), SLOT(inquirerDone()));
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(d->inquirer) );
}

QueryMaker*
NepomukQueryMaker::setQueryType( QueryType type )
{
    d->type = type;
    d->info += QString("[type %1] ").arg(type);
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::TrackPtr &track )
{
    d->info += QString("[match track %1] ").arg(track->prettyName());
    if( track )
        d->addFilter( QString(Q NS_track " = <%1>").arg(track->uidUrl()) );
    else
        d->matchNothing();
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour )
{
    d->info += QString("[match %1 artist %2] ").arg(behaviour).arg(artist->prettyName());
    if( behaviour == TrackArtists || behaviour == AlbumOrTrackArtists )
    {
        if( artist )
        {
            const Meta::NepomukArtist *nartist = dynamic_cast<const Meta::NepomukArtist*>( artist.data() );
            if( nartist )
                d->addFilter( QString("bound(" Q NS_artist ") && " Q NS_artist " = <%1>").arg( nartist->resourceUri().toString() ) );
            else
                d->addFilter( QString("bound(" Q NS_artistName ") && str(" Q NS_artistName ") = %1").arg( d->escape( artist->name() ) ) );
            return this;
        }
        else
            d->addFilter( "!bound(" Q NS_artist ")" );
    }
    else
    {
        if( artist )
            d->matchNothing(); // TODO add album artist matching behavior
    }
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
    d->info += QString("[match album %1] ").arg(album? album->prettyName() : "0");
    if( album )
    {
        const Meta::NepomukAlbum *nalbum = dynamic_cast<const Meta::NepomukAlbum*>( album.data() );
        if( nalbum )
            d->addFilter( QString("bound(" Q NS_album ") && " Q NS_album " = <%1>").arg( nalbum->resourceUri().toString() ) );
        else
            // TODO: fix album matching once album artists are supported by Nepomuk
            d->addFilter( QString("bound(" Q NS_albumTitle ") && str(" Q NS_albumTitle ") = %1").arg( d->escape( album->name() ) ) );
        return this;
    }
    else
        d->addFilter( "!bound(" Q NS_album ")" );
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
    d->info += QString("[match composer %1] ").arg(composer->prettyName());
    if( composer )
    {
        const Meta::NepomukComposer *ncomposer = dynamic_cast<const Meta::NepomukComposer*>( composer.data() );
        if( ncomposer )
            d->addFilter( QString("bound(" Q NS_composer ") && " Q NS_composer " = <%1>").arg( ncomposer->resourceUri().toString() ) );
        else
            d->addFilter( QString("bound(" Q NS_composerName ") && str(" Q NS_composerName ") = %1").arg( d->escape( composer->name() ) ) );
        return this;
    }
    else
        d->addFilter( "!bound(" Q NS_composer ")" );
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    d->info += QString("[match genre %1] ").arg(genre->prettyName());
    if( genre )
        d->addFilter( QString( "bound(" Q NS_genre ") && str(" Q NS_genre ") = %1" ).arg( d->escape( genre->name() ) ) );
    else
        d->addFilter( "!bound(" Q NS_genre ")" );
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::YearPtr &year )
{
    d->info += QString("[match year %1] ").arg(year->prettyName());
    if( year->year() )
        d->addFilter( QString( "bound(" Q NS_date ") && year(" Q NS_date ") = %1" ).arg( year->year() ) );
    else
        d->addFilter( "!bound(" Q NS_date ")" );
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::LabelPtr &label )
{
    d->info += QString("[match label %1] ").arg(label->prettyName());
    if( label )
        d->addFilter( QString( "EXISTS { " Q NS_track " nao:hasTag " Q NS_tag " . " Q NS_tag " nao:prefLabel %1 . }" ).arg( label->name() ) );
    else
        d->addFilter( "NOT EXISTS { " Q NS_track " nao:hasTag " Q NS_tag " . }" );
    return this;
}

QueryMaker*
NepomukQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    d->info += QString("[filter %1 %2 begin(%3) end(%4)] ").arg(value).arg(filter).arg(matchBegin).arg(matchEnd);
    d->addFilter( d->stringOperation( matchBegin, matchEnd ).arg( d->valueToSelector( value ),
                                                                  d->escape( filter ) ) );
    return this;
}

QueryMaker*
NepomukQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    d->info += QString("[exclude %1 %2 begin(%3) end(%4)] ").arg(value).arg(filter).arg(matchBegin).arg(matchEnd);
    d->addFilter( QString("!(%1)").arg( d->stringOperation( matchBegin, matchEnd ).arg( d->valueToSelector( value ),
                                                                                        d->escape( filter ) ) ) );
    return this;
}

QueryMaker*
NepomukQueryMaker::addNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
    d->info += QString("[filter %1 %2 (%3)] ").arg(value).arg(filter).arg(compare);
    d->addFilter( QString("%1 %2 %3").arg( d->valueToSelector( value ) )
                                     .arg( d->numberOperator( compare ) )
                                     .arg( filter ) );
    return this;
}

QueryMaker*
NepomukQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
    d->info += QString("[exclude %1 %2 (%3)] ").arg(value).arg(filter).arg(compare);
    d->addFilter( QString("!( %1 %2 %3 )").arg( d->valueToSelector( value ) )
                                          .arg( d->numberOperator( compare ) )
                                          .arg( filter ) );
    return this;
}

QueryMaker*
NepomukQueryMaker::addReturnValue( qint64 value )
{
    d->info += QString("[return %1] ").arg(value);
    d->customSelectors << d->valueToSelector(value);
    return this;
}

QueryMaker*
NepomukQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    d->info += QString("[return %1(%2)] ").arg(function).arg(value);
    d->customSelectors << d->returnFunctionSelector(function, value);
    d->distinct = false;
    return this;
}

QueryMaker*
NepomukQueryMaker::orderBy( qint64 value, bool descending )
{
    d->info += QString("[order %1(%2)] ").arg(value).arg(descending);
    d->extra += QString(" ORDER BY %1").arg( d->valueToSelector( value ) );
    return this;
}

QueryMaker*
NepomukQueryMaker::limitMaxResultSize( int size )
{
    d->info += QString("[limit %1] ").arg(size);
    d->extra += QString(" LIMIT %1").arg( size );
    return this;
}

QueryMaker*
NepomukQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    // TODO
    d->info += QString("[album mode %1] ").arg(mode);
    return this;
}

QueryMaker*
NepomukQueryMaker::setLabelQueryMode( LabelQueryMode mode )
{
    // TODO
    d->info += QString("[label mode %1] ").arg(mode);
    return this;
}

QueryMaker*
NepomukQueryMaker::beginAnd()
{
    d->info += QString("(AND ");
    d->pushLogic("&&");
    return this;
}

QueryMaker*
NepomukQueryMaker::beginOr()
{
    d->info += QString("(OR ");
    d->pushLogic("||");
    return this;
}

QueryMaker*
NepomukQueryMaker::endAndOr()
{
    d->info += QString(") ");
    d->popLogic();
    return this;
}

void
NepomukQueryMaker::inquirerDone()
{
    d->inquirer->deleteLater();
    emit queryDone();
}

} //namespace Collections
