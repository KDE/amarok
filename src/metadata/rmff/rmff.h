/***************************************************************************
      copyright            : (C) 2005 by Paul Cifarelli
      email                : paulc2@optonline.net
 ***************************************************************************/

/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin St, 5th fl, Boston, MA 02110-1301,      *
 *   USA, or check http://www.fsf.org/about/contact.html                   *
 *                                                                         *
 *   Note that no RealNetworks code appears or is duplicated, copied, or   *
 +   used as a template in this code.  The code was written from scratch   *
 *   using the reference documentation found at:                           *
 *                                                                         *
 *   https://common.helixcommunity.org/nonav/2003/HCS_SDK_r5/helixsdk.htm  *
 *                                                                         *
 ***************************************************************************/
#ifndef _RMFF_H_INCLUDED_
#define _RMFF_H_INCLUDED_

#include <config.h>

#include <string.h>

namespace TagLib
{
   namespace RealMedia
   {
#if SIZEOF_LONG == 4
      typedef unsigned long  UINT32;
#elif SIZEOF_INT == 4
      typedef unsigned int  UINT32;
#else
#error At least 1 builtin type needs to be 4 bytes!!
#endif
      typedef unsigned short UINT16;
      typedef unsigned char  UINT8;

      static const int RMFF_HDR_SIZE = 8;  // packed hdr size

      // some assumptions on these 2 enum defs, based solely on the order they are listed on the website
      enum PROPERTY_TYPES
      {
         MPT_TEXT = 1, // The value is string data.
         MPT_TEXTLIST, // The value is a separated list of strings, 
                       // separator specified as sub-property/type descriptor.
         MPT_FLAG,     // The value is a boolean flag-either 1 byte or 4 bytes, check size value.
         MPT_ULONG,    // The value is a four-byte integer.
         MPT_BINARY,   // The value is a byte stream.
         MPT_URL,      // The value is string data.
         MPT_DATE,     // The value is a string representation of the date in the form: 
                       // YYYYmmDDHHMMSS (m = month, M = minutes).
         MPT_FILENAME, // The value is string data.
         MPT_GROUPING, // This property has subproperties, but its own value is empty.
         MPT_REFERENCE // The value is a large buffer of data, use sub-properties/type 
                       // descriptors to identify mime-type.
      };

      enum PROPERTY_FLAGS
      {
         MPT_READONLY = 1,       // Read only, cannot be modified.
         MPT_PRIVATE = 2,        // Private, do not expose to users.
         MPT_TYPE_DESCRIPTOR = 4 // Type descriptor used to further define type of value.
      };
      
      struct Collectable
      {
         Collectable() : fwd(0) {}
         virtual ~Collectable() {}
         Collectable *fwd;
      };
      
      struct File_Header_Start
      {
         File_Header_Start() : object_id(0), size(0) {} 
         UINT32    object_id;
         UINT32    size;
      };
      
      struct File_Header_v0_v1 : public Collectable
      {
         File_Header_Start s;
         UINT16   object_version;
         
         UINT32   file_version;
         UINT32   num_headers;
      };
      
      struct RMProperties : public Collectable
      {
         File_Header_Start s;
         UINT16   object_version;
         
         UINT32   max_bit_rate;
         UINT32   avg_bit_rate;
         UINT32   max_packet_size;
         UINT32   avg_packet_size;
         UINT32   num_packets;
         UINT32   duration;
         UINT32   preroll;
         UINT32   index_offset;
         UINT32   data_offset;
         UINT16   num_streams;
         UINT16   flags;
      };


      struct NameValueProperty
      {
         NameValueProperty()
           : size(0), object_version(0), name_length(0), name(0), type(0)
           , value_length(0), value_data(0) {}
         virtual ~NameValueProperty() { delete [] name; delete [] value_data; }

         UINT32   size;
         UINT16   object_version;

         UINT8    name_length;
         UINT8   *name;
         UINT32   type;
         UINT16   value_length;
         UINT8   *value_data;
      }; 


      struct LogicalStream
      {
         LogicalStream()
            : size(0), object_version(0), num_physical_streams(0)
            , physical_stream_numbers(0), data_offsets(0), num_rules(0)
            , rule_to_physical_stream_number_map(0), num_properties(0)
            , properties(0) {}
         virtual ~LogicalStream() 
            { delete [] physical_stream_numbers; delete [] data_offsets; 
            delete [] rule_to_physical_stream_number_map; delete [] properties; }

         UINT32  size;
         UINT16  object_version;

         UINT16              num_physical_streams;
         UINT16             *physical_stream_numbers;
         UINT32             *data_offsets;
         UINT16              num_rules;
         UINT16             *rule_to_physical_stream_number_map;
         UINT16              num_properties;
         NameValueProperty  *properties;
      }; 
      
      struct MediaProperties : public Collectable
      {
         MediaProperties()
            : object_version(0), stream_number(0), max_bit_rate(0)
            , avg_bit_rate(0), max_packet_size(0), avg_packet_size(0)
            , start_time(0), preroll(0), duration(0), stream_name_size(0)
            , mime_type_size(0), type_specific_len(0), type_specific_data(0), lstr(0)
         {
            memset(stream_name, 0, sizeof(UINT8) * 256);
            memset(mime_type, 0, sizeof(UINT8) * 256);
         }
         virtual ~MediaProperties() { delete lstr; delete [] type_specific_data;  }
         
         File_Header_Start s;
         UINT16   object_version;
         
         UINT16   stream_number;
         UINT32   max_bit_rate;
         UINT32   avg_bit_rate;
         UINT32   max_packet_size;
         UINT32   avg_packet_size;
         UINT32   start_time;
         UINT32   preroll;
         UINT32   duration;
         UINT8    stream_name_size;
         UINT8    stream_name[256];
         UINT8    mime_type_size;
         UINT8    mime_type[256];
         UINT32   type_specific_len;
         UINT8   *type_specific_data;

         LogicalStream *lstr; // only one of these
      };
      
      
      struct ContentDescription : public Collectable
      {
         ContentDescription()
           : object_version(0), title_len(0), title(0), author_len(0)
           , author(0), copyright_len(0), copyright(0), comment_len(0)
           , comment(0) {}
         virtual ~ContentDescription()
           {
             delete [] title; delete [] author; delete [] copyright;
             delete [] comment;
           }
         
         File_Header_Start s;
         UINT16   object_version;
         
         UINT16   title_len;
         UINT8   *title;
         UINT16   author_len;
         UINT8   *author;
         UINT16   copyright_len;
         UINT8   *copyright;
         UINT16   comment_len;
         UINT8   *comment;
      };
      
      
      struct PropListEntry
      {
         UINT32 offset;
         UINT32 num_props_for_name;
      };
      
      struct MDProperties
      {
         MDProperties()
           : size(0), type(0), flags(0), value_offset(0)
           , subproperties_offset(0), num_subproperties(0), name_length(0)
           , name(0), value_length(0), value(0), subproperties_list(0)
           , subproperties(0) {}
         virtual ~MDProperties() 
            { delete [] name; delete [] value; delete [] subproperties_list; delete [] subproperties; }
         
         UINT32            size;
         UINT32            type;
         UINT32            flags;
         UINT32            value_offset;
         UINT32            subproperties_offset;
         UINT32            num_subproperties;
         UINT32            name_length;
         UINT8            *name;
         UINT32            value_length;
         UINT8            *value;
         PropListEntry    *subproperties_list; // num_subproperties 
         MDProperties     *subproperties;      // num_subproperties
      };
      
      struct MetadataSection : public Collectable
      {
         File_Header_Start s;
         
         UINT32  object_id;
         UINT32  object_version;

         // this is the 1 "unnamed root property"         
         MDProperties properties;
      };
      
      class Tag;
      class File;
      
      // RealMedia File Format contains a normal ID3v1 Tag at the end of the file
      // no sense reinventing the wheel, so this class is just so we can use TagLib
      // to manage it
      class RMFFile : public TagLib::File
      {
      public:
         RMFFile(const char *filename);
         virtual ~RMFFile();
         bool save();
         TagLib::Tag *tag() const { return m_id3tag; }
         TagLib::AudioProperties *audioProperties() const { return 0; }
         
      private:
         TagLib::ID3v1::Tag *m_id3tag;
      };
      
      class TagLib::AudioProperties;
      
      class RealMediaFF
      {
      public:
         RealMediaFF(const char *file, bool readProperties = true, 
                     TagLib::AudioProperties::ReadStyle propertiesStyle = TagLib::AudioProperties::Average);
         RealMediaFF(RealMediaFF &src);
         ~RealMediaFF();
         
         int err() const { return m_err; }
         bool isEmpty() const;
         
         // tag
         TagLib::String   title () const;
         TagLib::String   artist () const;
         TagLib::String   album () const;
         TagLib::String   comment () const;
         TagLib::String   genre () const;
         TagLib::uint     year () const;
         TagLib::uint     track () const;
         // TODO write support
         //void     setTitle (const String &s);
         //void     setArtist (const String &s);
         //void     setAlbum (const String &s);
         //void     setComment (const String &s);
         //void     setGenre (const String &s);
         //void     setYear (uint i);
         //void     setTrack (uint i);
         
         // properties
         int length () const;
         int bitrate () const;
         int sampleRate () const;
         int channels () const;
         
#ifdef TESTING
         std::ostream &operator<<(std::ostream &os);
#endif
         
      private:
         RealMediaFF();
         char               *m_filename;
         Collectable        *m_head;
         Collectable        *m_tail;
         int                 m_fd;
         int                 m_err;
         
         File_Header_v0_v1  *m_hdr;
         RMProperties       *m_props;
         MediaProperties    *media_hdrs;
         ContentDescription *m_contenthdr;
         MetadataSection    *m_md;
         
         char               *m_title;
         char               *m_author;
         char               *m_copyright;
         char               *m_comment;
         
         RMFFile            *m_id3v1tag;

         bool                m_flipYearInMetadataSection;
         bool                m_readProperties;

         int init();
         int initMetadataSection();
         void saveHeader(Collectable *hdr);
         int seekChunk(UINT32 object_id);
         
         int getHdr(unsigned char *buf, size_t sz, UINT32 &fourcc, UINT32 &csz);
         int getChunk(unsigned char *buf, size_t sz, UINT32 &fourcc, UINT32 &csz, UINT32 &consumed);
         int getRealFileHeader(File_Header_v0_v1 *hdr, const unsigned char *buf, UINT32 object_id, int sz);
         int getRealPropertyHeader(RMProperties *props, const unsigned char *buf, UINT32 object_id, int sz);
         int getMediaPropHeader(MediaProperties *mh, const unsigned char *buf, UINT32 object_id, int sz);
         int getContentDescription(ContentDescription *cont, const unsigned char *buf, UINT32 object_id, int sz);
         int getMDProperties(MDProperties *md, const unsigned char *buf);

#ifdef TESTING         
         void printRealFileHeader(std::ostream &os);
         void printRealPropHeader(std::ostream &os);
         void printMediaPropHeaders(std::ostream &os);
         void printContentDescription(std::ostream &os);
         void printID3v1Tag(std::ostream &os);
         void printMetadataSection(std::ostream &os);
         void printMDProperties(std::ostream &os, char *name, MDProperties *props);
#endif
      };
      
   } // namespace RealMedia
} // namespace TagLib

#ifdef TESTING
std::ostream &operator<<(std::ostream &os, TagLib::RealMedia::RealMediaFF &rmff);
#endif

#endif
