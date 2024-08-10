
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef HDR_tlProtocolBuffer
#define HDR_tlProtocolBuffer

#include "tlCommon.h"
#include "tlStream.h"

namespace tl
{

//  Representation of VARINT values
typedef uint64_t pb_varint;

enum PBWireType
{
  PB_VARINT = 0,
  PB_I64 = 1,
  PB_LEN = 2,
  PB_SGROUP = 3,
  PB_EGROUP = 4,
  PB_I32 = 5
};

/**
 *  @brief An exception thrown by the ProtocolBuffer reader on a read error
 */
class TL_PUBLIC ProtocolBufferReaderError
  : public tl::Exception
{
public:
  ProtocolBufferReaderError (const std::string &msg, size_t position)
    : tl::Exception (msg), m_position (position)
  { }

  virtual std::string msg () const
  {
    return basic_msg () + tl::to_string (tr (", at position ")) + tl::to_string (m_position);
  }

private:
  size_t m_position;
};

/**
 *  @brief A reader for ProtocolBuffer files and streams
 *
 *  This is a low-level decoder for ProtocolBuffer files.
 *
 *  The following LEN-type related concepts need to be implemented by the client code:
 *  - submessages
 *  - maps
 *  - packed repetitions
 *  - strings
 *
 *  Unknown tags need to be skipped with "skip".
 *
 *  Submessages: if a corresponding tag is encountered with "is_seq()" true, the
 *  reader code needs to call "open" to enter the sequence and read tags until
 *  "at_end" is true. Then, call "close" to leave the sequence.
 *
 *  Packed repetitions: same a submessages, but single values are read
 *  with one of the "read" types.
 *
 *  Maps are read like submessages with key/values as tags 1 and 2.
 *
 *  Strings: if a corresponding tag is encountered, use "read(s)" to read
 *  the string. "is_seq()" is required to be true, i.e. wire type is LEN.
 */
class TL_PUBLIC ProtocolBufferReader
{
public:
  /**
   *  @brief Creates a ProtocolBuffer parser from the given stream
   */
  ProtocolBufferReader (tl::InputStream &input);

  /**
   *  @brief Reads a new tag
   *  This method will also set the current write type.
   *  @returns The message ID
   */
  int read_tag ();

  /**
   *  @brief Gets the current wire type
   */
  PBWireType type () const
  {
    return m_type;
  }

  /**
   *  @brief Returns true, if the current message is a LEN type sequence
   *  Such messages can be read into strings or "open" can be used on them to
   *  open a submessage, map or packed repetition.
   */
  bool is_seq () const
  {
    return m_type == PB_LEN;
  }

  /**
   *  @brief Skips the current tag
   */
  void skip ();

  /**
   *  @brief Reads a floating-point value from the current message
   *  Throws a reader error if the current tag's value is not compatible with a double value.
   */
  void read (double &d);

  /**
   *  @brief Reads a floating-point value from the current message
   *  Throws a reader error if the current tag's value is not compatible with a float value.
   */
  void read (float &f);

  /**
   *  @brief Reads a string from the current message
   *  Throws a reader error if the current tag's value is not compatible with a string.
   */
  void read (std::string &s);

  /**
   *  @brief Reads a uint32_t value from the current message
   *  Throws a reader error if the current tag's value is not compatible with a uint32_t.
   */
  void read (uint32_t &ui32);

  /**
   *  @brief Reads a int32_t value from the current message
   *  Throws a reader error if the current tag's value is not compatible with a int32_t.
   */
  void read (int32_t &i32);

  /**
   *  @brief Reads a uint64_t value from the current message
   *  Throws a reader error if the current tag's value is not compatible with a uint64_t.
   */
  void read (uint64_t &ui64);

  /**
   *  @brief Reads a int64_t value from the current message
   *  Throws a reader error if the current tag's value is not compatible with a int64_t.
   */
  void read (int64_t &i64);

  /**
   *  @brief Reads a boolean value from the current message
   *  Throws a reader error if the current tag's value is not compatible with a bool.
   */
  void read (bool &b);

  /**
   *  @brief Opens a LEN sequence
   *  After calling "open", the parser will continue reading messages, but
   *  "at_end" will report true on the end of the sequence, not at the end of the
   *  file.
   *  This method will throw an exception if not in a message of LEN type.
   */
  void open ();

  /**
   *  @brief Closes the LEN sequence
   *  This method will jump to the end of the sequence and continue reading
   *  messages from the previous block or file.
   */
  void close ();

  /**
   *  @brief Returns true if at the end of the file or end of a block
   */
  bool at_end () const;

private:
  tl::InputStream *mp_stream;
  PBWireType m_type;
  size_t m_pos;
  size_t m_pos_before;
  std::vector<size_t> m_seq_counts;

  pb_varint read_varint ();
  void skip_bytes (size_t n);
  void error (const std::string &msg);
  const char *get (size_t n);
};

/**
 *  @brief The Protocol buffer writer
 *
 *  Scalar types are easy to write: just use the "write" methods.
 *  This includes strings.
 *
 *  Submessages and packed sequences are special as byte counting is
 *  required. The writer uses a two-pass approach. This means:
 *
 *  1. On writing a submessage or packed repetition, call "begin_seq"
 *     with the message tag and "counting" set to true.
 *  2. Write the elements using "write" or the submessage writing
 *     scheme recursively.
 *  3. At the end of the sequence, call "end_seq".
 *  4. if "is_counting()" is false, repeat steps 1 to 3 with
 *     "counting" set to false on "begin_seq".
 */
class TL_PUBLIC ProtocolBufferWriter
{
public:
  /**
   *  @brief Creates a writer for the given stream
   */
  ProtocolBufferWriter (tl::OutputStream &stream);

  /**
   *  @brief Writes a scalar tag with the given value
   */
  void write (int tag, float v);
  void write (int tag, double v);
  void write (int tag, uint32_t v, bool fixed = false);
  void write (int tag, int32_t v, bool fixed = false);
  void write (int tag, uint64_t v, bool fixed = false);
  void write (int tag, int64_t v, bool fixed = false);
  void write (int tag, bool b);
  void write (int tag, const std::string &s);

  /**
   *  @brief Returns true if the writer is in counting mode
   */
  bool is_counting () const;

  /**
   *  @brief Initiates a new sequence. See class documentation for details.
   */
  void begin_seq (int tag, bool counting);

  /**
   *  @brief Ends a sequence. See class documentation for details.
   */
  void end_seq ();

private:
  void write_varint (pb_varint v);

  tl::OutputStream *mp_stream;
  size_t m_bytes_counted;
  std::vector<size_t> m_byte_counter_stack;
};

}

#endif
