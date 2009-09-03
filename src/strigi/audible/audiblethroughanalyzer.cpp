/****************************************************************************************
 * Copyright (c) 2005-2007 Martin Aumueller <aumuell@reserv.at>                         *
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

#ifdef VERBOSE
#include <iostream>
#endif

#include <string>
#include <string.h>
#if 1
#include <strigi/streamthroughanalyzer.h>
#include <strigi/analyzerplugin.h>
#include <strigi/analysisresult.h>
#include <strigi/fieldtypes.h>
#include <strigi/textutils.h>
#else
#include "streamthroughanalyzer.h"
#include "analyzerplugin.h"
#include "analysisresult.h"
#include "fieldtypes.h"
#include "textutils.h"
#endif
#include <QtGlobal>

using namespace Strigi;

class AudibleThroughAnalyzerFactory;

static const Strigi::RegisteredField *sampleRateField = 0,
             *channelsField = 0,
             *audioDurationField = 0,
             *mimeTypeField = 0,
             *audioArtistField = 0,
             *audioNarratorField = 0,
             *audioTitleField = 0,
             *mediaCodecField = 0,
             *contentDescriptionField = 0,
             *contentCopyrightField = 0,
             *contentKeywordField = 0,
             *contentCreationTimeField = 0,
             *contentMaintainerField = 0,
             *contentIdField = 0,
             *audibleUserIdField = 0,
             *audibleUserAliasField = 0;
        

// Analyzer
class STRIGI_EXPORT AudibleThroughAnalyzer
    : public Strigi::StreamThroughAnalyzer {
        friend class AudibleThroughAnalyzerFactory;
private:
    Strigi::AnalysisResult* analysisResult;
    const AudibleThroughAnalyzerFactory* factory;
public:
    AudibleThroughAnalyzer(const AudibleThroughAnalyzerFactory* f)
        : analysisResult(0), factory(f) {}
    ~AudibleThroughAnalyzer() {}
    const char *name() const { return "Audible"; }
    void setIndexable(Strigi::AnalysisResult* i) {
        analysisResult = i;
    }
    bool isReadyWithStream() {
        return true;
    }
    Strigi::InputStream *connectInputStream(Strigi::InputStream *in);
};

InputStream*
AudibleThroughAnalyzer::connectInputStream(InputStream* in) {
    if (in == 0) return in;
    const int32_t nreq = 207;
    const char* buf;
    int32_t nread = in->read(buf, nreq, nreq);
    in->reset(0);

    if (nread < nreq) {
        return in;
    }

    // all the audible file I examined have these 4 bytes starting at an offset of 4
    const char sign[] = { 0x57, 0x90, 0x75, 0x36 };
    if (strncmp(sign, buf+4, 4)) {
        return in;
    }

    enum {
        NoDescription = 0,
        ShortDescription,
        RegularDescription,
        LongDescription
    } descriptionLength = NoDescription;

    std::string description;

    // audible files contain several key/value-pairs which are stored as this:
    // key length (4 bytes)
    // value length (4 bytes)
    // key (key length bytes)
    // value (value length bytes)
    // termination marker (1 byte)
    size_t filepos = 189; // this is where the first key/value-pair is stored
    do
    {
        int32_t nreq = filepos + 2*4;
        nread = in->read(buf, nreq, nreq);
        in->reset(0);
        if (nread < nreq) {
            return in;
        }

        uint32_t keyLength = readBigEndianUInt32(buf+filepos);
        uint32_t valueLength = readBigEndianUInt32(buf+filepos+4);
        if(keyLength > 1000 || valueLength > 100000)
            return in;
        const char *key = buf+filepos+2*4;
        const char *value = buf+filepos+2*4+keyLength;
        nreq = filepos + 2*4 + keyLength + valueLength + 1;
        nread = in->read(buf, nreq, nreq);
        in->reset(0);
        if (nread < nreq) {
            return in;
        }
        filepos = nreq;

        if(!strncmp(key, "codec", keyLength))
            analysisResult->addValue(mediaCodecField, std::string("audible/")+std::string(value, valueLength));
        else if(!strncmp(key, "title", keyLength))
            analysisResult->addValue(audioTitleField, std::string(value, valueLength));
        else if(!strncmp(key, "author", keyLength))
            analysisResult->addValue(audioArtistField, std::string(value, valueLength));
        else if(!strncmp(key, "narrator", keyLength))
            analysisResult->addValue(audioNarratorField, std::string(value, valueLength));
        else if(!strncmp(key, "user_id", keyLength))
            analysisResult->addValue(audibleUserIdField, std::string(value, valueLength));
        else if(!strncmp(key, "user_alias", keyLength))
            analysisResult->addValue(audibleUserAliasField, std::string(value, valueLength));
        else if(!strncmp(key, "copyright", keyLength))
            analysisResult->addValue(contentCopyrightField, std::string(value, valueLength));
        else if(!strncmp(key, "keywords", keyLength))
            analysisResult->addValue(contentKeywordField, std::string(value, valueLength));
        else if(!strncmp(key, "provider", keyLength))
            analysisResult->addValue(contentMaintainerField, std::string(value, valueLength));
        else if(!strncmp(key, "content_id", keyLength))
            analysisResult->addValue(contentIdField, std::string(value, valueLength));
        else if(!strncmp(key, "pubdate", keyLength))
        {
            analysisResult->addValue(contentCreationTimeField, std::string(value, valueLength));
        }
        else if(!strncmp(key, "short_description", keyLength))
        {
            if(descriptionLength <= ShortDescription)
            {
                description = std::string(value, valueLength);
                descriptionLength = ShortDescription;
            }
        }
        else if(!strncmp(key, "description", keyLength))
        {
            if(descriptionLength <= RegularDescription)
            {
                description = std::string(value, valueLength);
                descriptionLength = RegularDescription;
            }
        }
        else if(!strncmp(key, "long_description", keyLength))
        {
            description = std::string(value, valueLength);
            descriptionLength = LongDescription;
        }
#ifdef VERBOSE
        else
        {
            std::string k(key, keyLength);
            std::string v(value, valueLength);
            std::cerr << k<< ": " << v<< std::endl;
        }
#endif
    } while(!buf[filepos-1]);

    if(descriptionLength > NoDescription)
        analysisResult->addValue(contentDescriptionField, description);

    analysisResult->addValue(mimeTypeField, "audio/audible");
    analysisResult->addValue(audioDurationField, readBigEndianUInt32(buf+61));
    analysisResult->addValue(channelsField, 1);

    return in;
}

class AudibleThroughAnalyzerFactory
    : public Strigi::StreamThroughAnalyzerFactory {
friend class AudibleThroughAnalyzer;
private:

    const char* name() const {
        return "AudibleThroughAnalyzer";
    }
    Strigi::StreamThroughAnalyzer* newInstance() const {
        return new AudibleThroughAnalyzer(this);
    }
    void registerFields(Strigi::FieldRegister &reg) {
        mimeTypeField = reg.registerField("content.mime_type", FieldRegister::stringType, 1, 0);
        audioTitleField = reg.registerField("audio.title", FieldRegister::stringType, 1, 0);
        audioArtistField = reg.registerField("audio.artist", FieldRegister::stringType, 1, 0); // = author
        audioNarratorField = reg.registerField("todo.audio.narrator", FieldRegister::stringType, 1, 0); // usually different from the artist, could be mapped to performer
        mediaCodecField = reg.registerField("media.codec", FieldRegister::stringType, 1, 0);
        audibleUserIdField = reg.registerField("todo.audible.user_id", FieldRegister::stringType, 1, 0); // this is necessary in order to transfer the drm'ed file to an ipod
        audibleUserAliasField = reg.registerField("todo.audible.user_alias", FieldRegister::stringType, 1, 0); // just for information
        audioDurationField = reg.registerField("audio.duration", FieldRegister::integerType, 1, 0);
        contentDescriptionField = reg.registerField("content.description", FieldRegister::stringType, 1, 0);
        contentCopyrightField = reg.registerField("content.copyright", FieldRegister::stringType, 1, 0);
        contentKeywordField = reg.registerField("content.keyword", FieldRegister::stringType, 1, 0);
        contentCreationTimeField = reg.registerField("content.creation_time", FieldRegister::datetimeType, 1, 0);
        contentMaintainerField = reg.registerField("content.maintainer", FieldRegister::stringType, 1, 0);
        contentIdField = reg.registerField("content.ID", FieldRegister::stringType, 1, 0);

        channelsField = reg.registerField("audio.channel_count", FieldRegister::integerType, 1, 0);
        Q_UNUSED( sampleRateField );
    }
};

//Factory
class AudibleFactory : public AnalyzerFactoryFactory {
public:
    std::list<StreamThroughAnalyzerFactory*>
    streamThroughAnalyzerFactories() const {
        std::list<StreamThroughAnalyzerFactory*> af;
        af.push_back(new AudibleThroughAnalyzerFactory());
        return af;
    }
};

STRIGI_ANALYZER_FACTORY(AudibleFactory)
