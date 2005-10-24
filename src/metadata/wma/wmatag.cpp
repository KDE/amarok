#include <tag.h>
#include "wmatag.h"


using namespace TagLib;

WMA::WMATag::WMATag() : Tag::Tag() {
    m_title = String::null;
    m_artist = String::null;
    m_album = String::null;
    m_comment = String::null;
    m_genre = String::null;
    m_year = 0;
    m_track = 0;
}

WMA::WMATag::~WMATag() {
}

bool WMA::WMATag::isEmpty() const {
    return  m_title == String::null &&
        m_artist == String::null &&
        m_album == String::null && 
        m_comment == String::null &&
        m_genre == String::null &&
        m_year == 0 &&
        m_track == 0;
}

void WMA::WMATag::duplicate(const Tag *source, Tag *target, bool overwrite) {
    // No nonstandard information stored yet
    Tag::duplicate(source, target, overwrite);
}

