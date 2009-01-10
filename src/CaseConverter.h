#ifndef CASECONVERTER_H
#define CASECONVERTER_H

#include <QString>

namespace Amarok
{
/** Case converter for tag formatting.
  *
  * Provides helper functions to achieve sane capitalization of tag
  * information.
  */
class CaseConverter
{
public:
    /** Convert to "title case".
    *
    * Title case tries to conform to the common capitalization of titles,
    * i.e. first letter of each word is capitalized, except for "small"
    * words like "in", "of", etc.
    *
    * This implementation will also leave alone words that already have
    * some kind of capitalization, assuming that those are properly
    * formatted.
    *
    * @param s A string to be converted
    * @return The converted string
    */
    static QString toTitleCase( const QString &s );
    /** Convert to "capitalized case"
    *
    * Capitalizes the initial letter of each word.
    *
    * @param s A string to be converted
    * @return The converted string
    */
    static QString toCapitalizedCase( const QString &s );
private:
    /// regular expression for a word.
    static const QString s_MATCH_A_WORD;
    /** "small words" that ought not be capitalized.
    *
    * This is mostly English only.
    */
    static const QString s_LITTLE_WORDS;
};
}

#endif //CASECONVERTER_H