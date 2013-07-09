#ifndef AUDIOCDMETACLASS_H
#define AUDIOCDMETACLASS_H

#include "core/meta/Meta.h"

using namespace Meta;

template<typename T>
class AudioCdMetaClass : public T
{
    public:
        AudioCdMetaClass( const QString &name )
            : m_name( name )
        {
        }
        virtual ~AudioCdMetaClass()
        {
        }

        virtual QString name() const
        {
            return m_name;
        }

        virtual TrackList tracks()
        {
            return Meta::TrackList();
        }
    private:
        QString m_name;
};

typedef AudioCdMetaClass<Meta::Artist> AudioCdArtist;
typedef KSharedPtr<AudioCdArtist> AudioCdArtistPtr;

typedef AudioCdMetaClass<Meta::Composer> AudioCdComposer;
typedef KSharedPtr<AudioCdComposer> AudioCdComposerPtr;

typedef AudioCdMetaClass<Meta::Genre> AudioCdGenre;
typedef KSharedPtr<AudioCdArtist> AudioCdGenrePtr;

typedef AudioCdMetaClass<Meta::Year> AudioCdYear;
typedef KSharedPtr<AudioCdYear> AudioCdYearPtr;

#endif
