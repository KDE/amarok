/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#include "TagHelper.h"

#include <config.h>

#include <QImage>
#include <QRegularExpression>
#include <QStringList>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include <fileref.h>
#include <aifffile.h>
#include <asffile.h>
#include <flacfile.h>
#include <mp4file.h>
#include <mpcfile.h>
#include <mpegfile.h>
#include <oggfile.h>
#ifdef TAGLIB_OPUS_FOUND
#include <opusfile.h>
#endif
#include <oggflacfile.h>
#include <rifffile.h>
#include <speexfile.h>
#include <trueaudiofile.h>
#include <vorbisfile.h>
#include <wavfile.h>
#include <wavpackfile.h>
#ifdef TAGLIB_MOD_FOUND
#include <modfile.h>
#include <s3mfile.h>
#include <itfile.h>
#include <xmfile.h>
#endif
#pragma GCC diagnostic pop

#include "APETagHelper.h"
#include "ASFTagHelper.h"
#include "ID3v2TagHelper.h"
#include "MP4TagHelper.h"
#include "VorbisCommentTagHelper.h"

#include "StringHelper.h"

using namespace Meta::Tag;

TagHelper::TagHelper( TagLib::Tag *tag, Amarok::FileType fileType )
         : m_tag( tag )
         , m_fileType( fileType )
{
}

TagHelper::TagHelper( TagLib::ID3v1::Tag *tag, Amarok::FileType fileType )
         : m_tag( tag )
         , m_fileType( fileType )
{
}

TagHelper::~TagHelper()
{
}

Meta::FieldHash
TagHelper::tags() const
{
    Meta::FieldHash data;

    data.insert( Meta::valTitle,   TStringToQString( m_tag->title() ) );
    data.insert( Meta::valArtist,  TStringToQString( m_tag->artist() ) );
    data.insert( Meta::valAlbum,   TStringToQString( m_tag->album() ) );
    data.insert( Meta::valTrackNr, m_tag->track() );
    data.insert( Meta::valYear,    m_tag->year() );
    data.insert( Meta::valGenre,   TStringToQString( m_tag->genre() ) );
    data.insert( Meta::valComment, TStringToQString( m_tag->comment() ) );

    return data;
}

bool
TagHelper::setTags( const Meta::FieldHash &changes )
{
    bool modified = false;

    if( changes.contains( Meta::valTitle ) )
    {
        m_tag->setTitle( Qt4QStringToTString( changes.value( Meta::valTitle ).toString() ) );
        modified = true;
    }
    if( changes.contains( Meta::valArtist ) )
    {
        m_tag->setArtist( Qt4QStringToTString( changes.value( Meta::valArtist ).toString() ) );
        modified = true;
    }
    if( changes.contains( Meta::valAlbum ) )
    {
        m_tag->setAlbum( Qt4QStringToTString( changes.value( Meta::valAlbum ).toString() ) );
        modified = true;
    }
    if( changes.contains( Meta::valTrackNr ) )
    {
        m_tag->setTrack( changes.value( Meta::valTrackNr ).toUInt() );
        modified = true;
    }
    if( changes.contains( Meta::valYear ) )
    {
        m_tag->setYear( changes.value( Meta::valYear ).toUInt() );
        modified = true;
    }
    if( changes.contains( Meta::valGenre ) )
    {
        m_tag->setGenre( Qt4QStringToTString( changes.value( Meta::valGenre ).toString() ) );
        modified = true;
    }
    if( changes.contains( Meta::valComment ) )
    {
        m_tag->setComment( Qt4QStringToTString( changes.value( Meta::valComment ).toString() ) );
        modified = true;
    }

    return modified;
}

TagLib::ByteVector
TagHelper::render() const
{
    QByteArray byteArray;
    QDataStream stream( &byteArray, QIODevice::WriteOnly );
    stream << tags();
    return TagLib::ByteVector( byteArray.constData(), byteArray.size() );
}

bool
TagHelper::hasEmbeddedCover() const
{
    return false;
}

QImage
TagHelper::embeddedCover() const
{
    return QImage();
}

bool
TagHelper::setEmbeddedCover( const QImage &cover )
{
    Q_UNUSED( cover )
    return false;
}

TagLib::String
TagHelper::fieldName( const qint64 field ) const
{
    return m_fieldMap.value( field );
}

qint64
TagHelper::fieldName( const TagLib::String &field ) const
{
    return m_fieldMap.key( field );
}

QPair< TagHelper::UIDType, QString >
TagHelper::splitUID( const QString &uidUrl ) const
{
    TagHelper::UIDType type = UIDInvalid;
    QString uid = uidUrl;

    if( uid.startsWith( QLatin1String("amarok-") ) )
        uid = uid.remove( QRegularExpression( QStringLiteral("^(amarok-\\w+://).+$") ) );

    if( isValidUID( uid, UIDAFT ) )
        type = UIDAFT;

    return qMakePair( type, uid );
}

QPair< int, int >
TagHelper::splitDiscNr( const QString &value ) const
{
    int disc;
    int count = 0;
    if( value.indexOf( QLatin1Char('/') ) != -1 )
    {
        QStringList list = value.split( QLatin1Char('/'), Qt::SkipEmptyParts );
        disc = list.value( 0 ).toInt();
        count = list.value( 1 ).toInt();
    }
    else if( value.indexOf( QLatin1Char(':') ) != -1 )
    {
        QStringList list = value.split( QLatin1Char(':'), Qt::SkipEmptyParts );
        disc = list.value( 0 ).toInt();
        count = list.value( 1 ).toInt();
    }
    else
        disc = value.toInt();

    return qMakePair( disc, count );
}

bool
TagHelper::isValidUID( const QString &uid, const TagHelper::UIDType type ) const
{
    if( uid.length() >= 127 ) // the database can't handle longer uids
        return false;

    QRegularExpression regexp( QStringLiteral("^$") );

    if( type == UIDAFT )
        regexp.setPattern( QStringLiteral("^[0-9a-fA-F]{32}$") );

    return QRegularExpression(QRegularExpression::anchoredPattern(regexp.pattern())).match( uid ).hasMatch();
}

TagLib::String
TagHelper::uidFieldName( const TagHelper::UIDType type ) const
{
    return m_uidFieldMap.value( type );
}

TagLib::String
TagHelper::fmpsFieldName( const TagHelper::FMPS field ) const
{
    return m_fmpsFieldMap.value( field );
}

Amarok::FileType
TagHelper::fileType() const
{
    return m_fileType;
}

QByteArray
TagHelper::testString() const
{
    TagLib::String string = m_tag->album() + m_tag->artist() + m_tag->comment() +
                            m_tag->genre() + m_tag->title();

    return QByteArray( string.toCString( true ) );
}


Meta::Tag::TagHelper *
Meta::Tag::selectHelper( const TagLib::FileRef &fileref, bool forceCreation )
{
    TagHelper *tagHelper = nullptr;

    if( TagLib::MPEG::File *file = dynamic_cast< TagLib::MPEG::File * >( fileref.file() ) )
    {
        if( file->ID3v2Tag( forceCreation ) )
            tagHelper = new ID3v2TagHelper( fileref.tag(), file->ID3v2Tag(), Amarok::Mp3 );
        else if( file->APETag() )
            tagHelper = new APETagHelper( fileref.tag(), file->APETag(), Amarok::Mp3 );
        else if( file->ID3v1Tag() )
            tagHelper = new TagHelper( fileref.tag(), Amarok::Mp3 );
    }
    else if( TagLib::Ogg::Vorbis::File *file = dynamic_cast< TagLib::Ogg::Vorbis::File * >( fileref.file() ) )
    {
        if( file->tag() )
            tagHelper = new VorbisCommentTagHelper( fileref.tag(), file->tag(), Amarok::Ogg );
    }
    else if( TagLib::Ogg::FLAC::File *file = dynamic_cast< TagLib::Ogg::FLAC::File * >( fileref.file() ) )
    {
        if( file->tag() )
            tagHelper = new VorbisCommentTagHelper( fileref.tag(), file->tag(), Amarok::Flac );
    }
    else if( TagLib::Ogg::Speex::File *file = dynamic_cast< TagLib::Ogg::Speex::File * >( fileref.file() ) )
    {
        if( file->tag() )
            tagHelper = new VorbisCommentTagHelper( fileref.tag(), file->tag(), Amarok::Speex );
    }
#ifdef TAGLIB_OPUS_FOUND
    else if( TagLib::Ogg::Opus::File *file = dynamic_cast< TagLib::Ogg::Opus::File * >( fileref.file() ) )
    {
        if( file->tag() )
            tagHelper = new VorbisCommentTagHelper( fileref.tag(), file->tag(), Amarok::Opus );
    }
#endif
    else if( TagLib::FLAC::File *file = dynamic_cast< TagLib::FLAC::File * >( fileref.file() ) )
    {
        if( file->xiphComment() )
            tagHelper = new VorbisCommentTagHelper( fileref.tag(), file->xiphComment(), Amarok::Flac, file );
        else if( file->ID3v2Tag() )
            tagHelper = new ID3v2TagHelper( fileref.tag(), file->ID3v2Tag(), Amarok::Flac );
        else if( file->ID3v1Tag() )
            tagHelper = new TagHelper( fileref.tag(), Amarok::Flac );
    }
    else if( TagLib::MP4::File *file = dynamic_cast< TagLib::MP4::File * >( fileref.file() ) )
    {
        TagLib::MP4::Tag *tag = dynamic_cast< TagLib::MP4::Tag * >( file->tag() );
        if( tag )
        {
            Amarok::FileType specificType = Amarok::Mp4;
            QString filename = QString::fromLatin1( fileref.file()->name() );
            foreach( Amarok::FileType type, QList<Amarok::FileType>() << Amarok::M4a << Amarok::M4v )
            {
                if( filename.endsWith( Amarok::FileTypeSupport::toString( type ), Qt::CaseInsensitive ) )
                    specificType = type;
            }
            tagHelper = new MP4TagHelper( fileref.tag(), tag, specificType );
        }
    }
    else if( TagLib::MPC::File *file = dynamic_cast< TagLib::MPC::File * >( fileref.file() ) )
    {
        if( file->APETag( forceCreation ) )
            tagHelper = new APETagHelper( fileref.tag(), file->APETag(), Amarok::Mpc );
        else if( file->ID3v1Tag() )
            tagHelper = new TagHelper( fileref.tag(), Amarok::Mpc );
    }
    else if( TagLib::RIFF::AIFF::File *file = dynamic_cast< TagLib::RIFF::AIFF::File * >( fileref.file() ) )
    {
        if( file->tag() )
            tagHelper = new ID3v2TagHelper( fileref.tag(), file->tag(), Amarok::Aiff );
    }
    else if( TagLib::RIFF::WAV::File *file = dynamic_cast< TagLib::RIFF::WAV::File * >( fileref.file() ) )
    {
        if( file->tag() )
            tagHelper = new ID3v2TagHelper( fileref.tag(), file->ID3v2Tag(), Amarok::Wav );
    }
    else if( TagLib::ASF::File *file = dynamic_cast< TagLib::ASF::File * >( fileref.file() ) )
    {
        TagLib::ASF::Tag *tag = dynamic_cast< TagLib::ASF::Tag * >( file->tag() );
        if( tag )
            tagHelper = new ASFTagHelper( fileref.tag(), tag, Amarok::Wma );
    }
    else if( TagLib::TrueAudio::File *file = dynamic_cast< TagLib::TrueAudio::File * >( fileref.file() ) )
    {
        if( file->ID3v2Tag( forceCreation ) )
            tagHelper = new ID3v2TagHelper( fileref.tag(), file->ID3v2Tag(), Amarok::TrueAudio );
        else if( file->ID3v1Tag() )
            tagHelper = new TagHelper( fileref.tag(), Amarok::TrueAudio );
    }
    else if( TagLib::WavPack::File *file = dynamic_cast< TagLib::WavPack::File * >( fileref.file() ) )
    {
        if( file->APETag( forceCreation ) )
            tagHelper = new APETagHelper( fileref.tag(), file->APETag(), Amarok::WavPack );
        else if( file->ID3v1Tag() )
            tagHelper = new TagHelper( fileref.tag(), Amarok::WavPack );
    }
#ifdef TAGLIB_MOD_FOUND
    else if( TagLib::Mod::File *file = dynamic_cast< TagLib::Mod::File * >( fileref.file() ) )
    {
        if( file->tag() )
            tagHelper = new TagHelper( fileref.tag(), Amarok::Mod );
    }
    else if( TagLib::S3M::File *file = dynamic_cast< TagLib::S3M::File * >( fileref.file() ) )
    {
        if( file->tag() )
            tagHelper = new TagHelper( fileref.tag(), Amarok::S3M );
    }
    else if( TagLib::IT::File *file = dynamic_cast< TagLib::IT::File * >( fileref.file() ) )
    {
        if( file->tag() )
            tagHelper = new TagHelper( fileref.tag(), Amarok::IT );
    }
    else if( TagLib::XM::File *file = dynamic_cast< TagLib::XM::File * >( fileref.file() ) )
    {
        if( file->tag() )
            tagHelper = new TagHelper( fileref.tag(), Amarok::XM );
    }
#endif

    return tagHelper;
}

