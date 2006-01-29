// (C) 2005 Martin Aumueller <aumuell@reserv.at>
// portions are (C) 2005 Umesh Shankar <ushankar@cs.berkeley.edu>
//          and (C) 2005 Andy Leadbetter <andrew.leadbetter@gmail.com>
//
// See COPYING file for licensing information

#include "audibletag.h"

#include <taglib/tag.h>

#include <netinet/in.h> // ntohl

using namespace TagLib;

Audible::Tag::Tag() : TagLib::Tag::Tag() {
    m_title = String::null;
    m_artist = String::null;
    m_album = String::null;
    m_comment = String::null;
    m_genre = String::null;
    m_year = 0;
    m_track = 0;
    m_userID = 0;
    m_tagsEndOffset = -1;
}

Audible::Tag::~Tag() {
}

bool Audible::Tag::isEmpty() const {
    return  m_title == String::null &&
        m_artist == String::null &&
        m_album == String::null && 
        m_comment == String::null &&
        m_genre == String::null &&
        m_year == 0 &&
        m_track == 0 &&
        m_userID == 0;
}

void Audible::Tag::duplicate(const Tag *source, Tag *target, bool overwrite) {
    // No nonstandard information stored yet
    Tag::duplicate(source, target, overwrite);
}

#define OFF_PRODUCT_ID 197
#define OFF_TAGS 189

void Audible::Tag::readTags( FILE *fp )
{
    char buf[1023];
    fseek(fp, OFF_PRODUCT_ID, SEEK_SET);
    fread(buf, strlen("product_id"), 1, fp);
    if(memcmp(buf, "product_id", strlen("product_id")))
    {
        buf[20]='\0';
        fprintf(stderr, "no valid Audible aa file: %s\n", buf);
        return;
    }

    // Now parse tag.

    fseek(fp, OFF_TAGS, SEEK_SET);
    char *name, *value;

    m_tagsEndOffset = OFF_TAGS;

    bool lasttag = false;
    while(!lasttag)
    {
        lasttag = !readTag(fp, &name, &value);
        if(!strcmp(name, "title"))
        {
            m_title = String(value, String::Latin1);
        }
        else if(!strcmp(name, "author"))
        {
            m_artist = String(value, String::Latin1);
        }
        else if(!strcmp(name, "long_description"))
        {
            m_comment = String(value, String::Latin1);
        }
        else if(!strcmp(name, "description"))
        {
            if( m_comment.isNull() )
                m_comment = String(value, String::Latin1);
        }
        else if(!strcmp(name, "pubdate"))
        {
            m_year = 0;
            char *p = strrchr(value, '-');
            if(p)
                m_year = strtol(p+1, NULL, 10);
        }
        else if(!strcmp(name, "user_id"))
        {
            m_userID = strtol(value, NULL, 10);
        }

        delete[] name;
        delete[] value;
    }

    m_album  =  String("", String::Latin1);
    m_track = 0;
    m_genre = String("Audiobook", String::Latin1);
}

bool Audible::Tag::readTag( FILE *fp, char **name, char **value)
{
    uint32_t nlen;
    fread(&nlen, sizeof(nlen), 1, fp);
    nlen = ntohl(nlen);
    //fprintf(stderr, "tagname len=%x\n", (unsigned)nlen);
    *name = new char[nlen+1];
    (*name)[nlen] = '\0';

    uint32_t vlen;
    fread(&vlen, sizeof(vlen), 1, fp);
    vlen = ntohl(vlen);
    //fprintf(stderr, "tag len=%x\n", (unsigned)vlen);
    *value = new char[vlen+1];
    (*value)[vlen] = '\0';

    fread(*name, nlen, 1, fp);
    fread(*value, vlen, 1, fp);
    char lasttag;
    fread(&lasttag, 1, 1, fp);
    //fprintf(stderr, "%s: \"%s\"\n", *name, *value);

    m_tagsEndOffset += 2 * 4 + nlen + vlen + 1;

    return !lasttag;
}

int Audible::Tag::getTagsEndOffset()
{
    return m_tagsEndOffset;
}
