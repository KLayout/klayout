
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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


#ifndef HDR_tlStream
#define HDR_tlStream

#include "tlCommon.h"

#include "tlException.h"
#include "tlString.h"

#include <string>
#include <sstream>
#include <cstdio>
#include <cstring>


namespace tl
{

class InflateFilter;
class DeflateFilter;
class OutputStream;

// ---------------------------------------------------------------------------------

/**
 *  @brief An utility method to match a file name against a given format
 */
extern TL_PUBLIC bool match_filename_to_format (const std::string &fn, const std::string &fmt);

// ---------------------------------------------------------------------------------

/**
 *  @brief The input stream delegate base class
 *
 *  This class provides the basic input stream functionality.
 *  The actual implementation is provided through InputFile, InputPipe and InputZLibFile.
 */

class TL_PUBLIC InputStreamBase
{
public:
  InputStreamBase () { }
  virtual ~InputStreamBase () { }

  /** 
   *  @brief Read a block of n bytes
   *
   *  Read the requested number of bytes or less.
   *  May throw an exception if a read error occurs.
   *  
   *  @param b The buffer where to write to
   *  @param n The number of bytes to read (or less)
   *
   *  @return The number of bytes read. Should report 0 on EOF
   */
  virtual size_t read (char *b, size_t n) = 0;

  /**
   *  @brief Seek to the beginning
   */
  virtual void reset () = 0;

  /**
   *  @brief Closes the channel
   */
  virtual void close () = 0;

  /**
   *  @brief Get the source specification (i.e. the file name)
   */
  virtual std::string source () const = 0;

  /**
   *  @brief Gets the absolute path of the source
   */
  virtual std::string absolute_path () const = 0;

  /**
   *  @brief Gets the filename part of the source
   */
  virtual std::string filename () const = 0;
};

// ---------------------------------------------------------------------------------

class ZLibFilePrivate;

// ---------------------------------------------------------------------------------
//  InputStreamBase implementations

/**
 *  @brief An in-memory input file delegate
 */
class TL_PUBLIC InputMemoryStream
  : public InputStreamBase
{
public:
  /**
   *  @brief Create a stream reading from the given memory block
   *
   *  @param data The memory block where to read from
   *  @param length The length of the block
   *  @param owns_data If true, the memory becomes owned by the stream
   */
  InputMemoryStream (const char *data, size_t length, bool owns_data = false)
    : mp_data (data), m_length (length), m_owns_data (owns_data), m_pos (0)
  {
    //  .. nothing yet ..
  }

  ~InputMemoryStream ()
  {
    if (m_owns_data) {
      delete [] const_cast<char *> (mp_data);
    }
    mp_data = 0;
  }

  virtual size_t read (char *b, size_t n)
  {
    if (m_pos + n > m_length) {
      n = m_length - m_pos;
    }
    memcpy (b, mp_data + m_pos, n);
    m_pos += n;
    return n;
  }

  virtual void reset ()
  {
    m_pos = 0;
  }

  virtual void close ()
  {
    //  .. nothing yet ..
  }

  virtual std::string source () const
  {
    return "data";
  }

  virtual std::string absolute_path () const
  {
    return "data";
  }

  virtual std::string filename () const
  {
    return "data";
  }

private:
  //  no copying
  InputMemoryStream (const InputMemoryStream &);
  InputMemoryStream &operator= (const InputMemoryStream &);

  const char *mp_data;
  size_t m_length;
  bool m_owns_data;
  size_t m_pos;
};

/**
 *  @brief A zlib input file delegate
 *
 *  Implements the reader for a zlib stream
 */
class TL_PUBLIC InputZLibFile
  : public InputStreamBase
{
public:
  /**
   *  @brief Open a file with the given path
   *
   *  Opening a file is a prerequisite for reading from the
   *  object. open() will throw a FileOpenErrorException if
   *  an error occurs.
   *
   *  @param path The (relative) path of the file to open
   */
  InputZLibFile (const std::string &path);

  /**
   *  @brief Close the file
   *
   *  The destructor will automatically close the file.
   */
  virtual ~InputZLibFile ();

  /**
   *  @brief Read from a file
   *
   *  Implements the basic read method.
   *  Will throw a ZLibReadErrorException if an error occurs.
   */
  virtual size_t read (char *b, size_t n);

  virtual void reset ();

  virtual void close ();

  virtual std::string source () const
  {
    return m_source;
  }

  virtual std::string absolute_path () const;

  virtual std::string filename () const;

private:
  //  no copying
  InputZLibFile (const InputZLibFile &);
  InputZLibFile &operator= (const InputZLibFile &);

  std::string m_source;
  ZLibFilePrivate *mp_d;
};

/**
 *  @brief A simple input file delegate
 *
 *  Implements the reader for ordinary files.
 */
class TL_PUBLIC InputFile
  : public InputStreamBase
{
public:
  /**
   *  @brief Open a file with the given path
   *
   *  Opening a file is a prerequisite for reading from the
   *  object. open() will throw a FileOpenErrorException if
   *  an error occurs.
   *
   *  @param path The (relative) path of the file to open
   *  @param read True, if the file should be read, false on write.
   */
  InputFile (const std::string &path);

  /**
   *  @brief Close the file
   *
   *  The destructor will automatically close the file.
   */
  virtual ~InputFile ();

  virtual size_t read (char *b, size_t n);

  virtual void reset ();

  virtual void close ();

  virtual std::string source () const
  {
    return m_source;
  }

  virtual std::string absolute_path () const;

  virtual std::string filename () const;

private:
  //  no copying
  InputFile (const InputFile &d);
  InputFile &operator= (const InputFile &d);

  std::string m_source;
  int m_fd;
};

/**
 *  @brief A simple pipe input delegate
 *
 *  Implements the reader for pipe streams
 */
class TL_PUBLIC InputPipe
  : public InputStreamBase
{
public:
  /**
   *  @brief Open a stream by connecting with the stdout of a given command
   *
   *  Opening a pipe is a prerequisite for reading from the
   *  object. open() will throw a FilePOpenErrorException if
   *  an error occurs - commonly if the command cannot be executed.
   *  This implementation is based on popen ().
   *
   *  @param cmd The command to execute
   *  @param read True, if the file should be read, false on write.
   */
  InputPipe (const std::string &path);

  /**
   *  @brief Close the pipe
   *
   *  The destructor will automatically close the pipe.
   */
  virtual ~InputPipe ();

  /**
   *  @brief Read from the pipe
   *
   *  Implements the basic read method.
   *  Will throw a FilePReadErrorException if an error occurs.
   */
  virtual size_t read (char *b, size_t n);

  /**
   *  @brief Reset to the beginning of the file
   */
  virtual void reset ();

  /**
   *  @brief Closes the pipe
   *  This method will wait for the child process to terminate.
   */
  virtual void close ();

  /**
   *  @brief Get the source specification (the file name)
   *
   *  Returns an empty string if no file name is available.
   */
  virtual std::string source () const
  {
    //  No source (in the sense of a file name) is available ..
    return std::string ();
  }

  virtual std::string absolute_path () const
  {
    //  No source (in the sense of a file name) is available ..
    return std::string ();
  }

  virtual std::string filename () const
  {
    //  No source (in the sense of a file name) is available ..
    return std::string ();
  }

  /**
   *  @brief Closes the pipe and returns the exit code of the child process
   */
  int wait ();

private:
  //  No copying
  InputPipe (const InputPipe &);
  InputPipe &operator= (const InputPipe &);

  FILE *m_file;
  std::string m_source;
};

// ---------------------------------------------------------------------------------

/**
 *  @brief An input stream abstraction class
 *
 *  The basic objective of this interface is to provide
 *  the capability to read a block of n bytes into a buffer.
 *  This object provides unget capabilities and buffering.
 *  The actual stream access is delegated to another object.
 */

class TL_PUBLIC InputStream
{
public:
  /**
   *  @brief Default constructor
   *  This constructor takes a delegate object, but does not take ownership.
   */
  InputStream (InputStreamBase &delegate);

  /**
   *  @brief Default constructor
   *  This constructor takes a delegate object, and takes ownership.
   */
  InputStream (InputStreamBase *delegate);

  /**
   *  @brief Opens a stream from a abstract path
   *
   *  This will automatically create the appropriate delegate and 
   *  delete it later.
   *
   *  The abstract path
   */
  InputStream (const std::string &abstract_path);

  /**
   *  @brief Destructor
   */
  virtual ~InputStream ();

  /** 
   *  @brief This is the outer write method to call
   *  
   *  This implementation writes data through the 
   *  protected write call.
   */
  void put (const char *b, size_t n);

  /** 
   *  @brief This is the outer read method to call
   *  
   *  This implementation obtains data through the 
   *  protected read call and buffers the data accordingly so
   *  a contigious memory block can be returned.
   *  If inline deflating is enabled, the method will return
   *  inflate data unless "bypass_inflate" is set to true.
   *
   *  @return 0 if not enough data can be obtained
   */
  const char *get (size_t n, bool bypass_inflate = false);

  /** 
   *  @brief Undo a previous get call
   *  
   *  This call puts back the bytes read by a previous get call.
   *  Only one call can be made undone.
   */
  void unget (size_t n);

  /**
   *  @brief Reads all remaining bytes into the string
   */
  std::string read_all ();

  /**
   *  @brief Reads all remaining bytes into the string 
   *
   *  This function reads all remaining of max_count bytes.
   */
  std::string read_all (size_t max_count);

  /**
   *  @brief Copies the content of the stream to the output stream
   *  Throws an exception on error.
   */
  void copy_to (tl::OutputStream &os);

  /**
   *  @brief Enable uncompression of the following DEFLATE-compressed block
   *
   *  This call will enable uncompression of the next block
   *  of DEFLATE (RFC1951) compressed data. Subsequence get() calls will deliver
   *  the uncompressed data rather than the raw data, until the
   *  compressed block is finished.
   *  The stream must not be in inflate state yet.
   */
  void inflate ();

  /**
   *  @brief Enables "inflate" right from the beginning
   *
   *  Contrary to "inflate" (which is temporary), this version enables
   *  decompression right from the beginning of the file. It does a "reset"
   *  implicitly.
   */
  void inflate_always ();

  /**
   *  @brief Obtain the current file position
   */
  size_t pos () const 
  {
    return m_pos;
  }

  /**
   *  @brief Obtain the available number of bytes
   *
   *  This is the number of bytes that can be delivered on the next get, not
   *  the total file size.
   */
  size_t blen () const
  {
    return m_blen;
  }

  /**
   *  @brief Get the source specification (the file name)
   *
   *  Returns an empty string if no file name is available.
   */
  std::string source () const
  {
    return mp_delegate->source ();
  }

  /**
   *  @brief Get the filename specification (the file name part of the path)
   *
   *  Returns an empty string if no file name is available.
   */
  std::string filename () const
  {
    return mp_delegate->filename ();
  }

  /**
   *  @brief Get the absolute path 
   *
   *  Returns an empty string if no absolute path is available.
   */
  std::string absolute_path () const
  {
    return mp_delegate->absolute_path ();
  }

  /**
   *  @brief Reset to the initial position
   */
  virtual void reset ();

  /**
   *  @brief Closes the reader
   *  This method will finish reading and free resources
   *  associated with it. HTTP connections will be closed.
   */
  void close ();

  /**
   *  @brief Gets the absolute path for a given URL
   */
  static std::string absolute_path (const std::string &path);

  /**
   *  @brief Gets the base reader (delegate)
   */
  InputStreamBase *base ()
  {
    return mp_delegate;
  }

protected:
  void reset_pos ()
  {
    m_pos = 0;
  }

private:
  size_t m_pos;
  char *mp_buffer;
  size_t m_bcap;
  size_t m_blen;
  char *mp_bptr;
  InputStreamBase *mp_delegate;
  bool m_owns_delegate;

  //  inflate support 
  InflateFilter *mp_inflate;
  bool m_inflate_always;

  //  No copying currently
  InputStream (const InputStream &);
  InputStream &operator= (const InputStream &);
};

// ---------------------------------------------------------------------------------

/**
 *  @brief A text input stream (UTF8 encoded)
 *
 *  This class is put in front of a InputStream to format the input as text input stream.
 */
class TL_PUBLIC TextInputStream 
{
public:
  /**
   *  @brief Default constructor
   *
   *  This constructor takes a delegate object. 
   */
  TextInputStream (InputStream &stream);

  /**
   *  @brief Gets the raw stream
   */
  InputStream &raw_stream ()
  {
    return m_stream;
  }

  /**
   *  @brief Get a single line (presumably UTF8 encoded)
   */
  const std::string &get_line ();

  /**
   *  @brief Reads all remaining bytes into the string
   */
  std::string read_all ();

  /**
   *  @brief Reads all remaining bytes into the string
   *
   *  This function reads all remaining of max_count bytes.
   */
  std::string read_all (size_t max_count);

  /**
   *  @brief Get a single character
   */
  char get_char ();

  /**
   *  @brief Peek a single character
   */
  char peek_char ();

  /**
   *  @brief Skip blanks, newlines etc.
   *
   *  Returns the following character without getting it.
   */
  char skip ();

  /**
   *  @brief Get the source specification
   */
  std::string source () const
  {
    return m_stream.source ();
  }

  /**
   *  @brief Get the current line number
   */
  size_t line_number ()
  {
    return m_line;
  }

  /**
   *  @brief Return false, if no more characters can be obtained
   */
  bool at_end () const 
  {
    return m_at_end;
  }

  /**
   *  @brief Reset to the initial position
   */
  void reset ();

private:
  size_t m_line, m_next_line;
  bool m_at_end;
  std::string m_line_buffer;
  InputStream &m_stream;

  //  no copying
  TextInputStream (const TextInputStream &);
  TextInputStream &operator= (const TextInputStream &);
};

// ---------------------------------------------------------------------------------

/**
 *  @brief The output stream delegate base class
 *
 *  This class provides the basic output stream functionality.
 *  The actual implementation is provided through OutputFile, OutputPipe and OutputZLibFile.
 */

class TL_PUBLIC OutputStreamBase
{
public:
  OutputStreamBase () { }
  virtual ~OutputStreamBase () { }

  /**
   *  @brief Write a block a n bytes
   *
   *  May throw an exception if a write error occurs.
   *
   *  @param b What to write
   *  @param n The number of bytes to write 
   */
  virtual void write (const char *b, size_t n) = 0;

  /**
   *  @brief Seek to the specified position 
   *
   *  Writing continues at that position after a seek.
   */
  virtual void seek (size_t /*s*/) 
  {
    //  .. the default implementation does nothing ..
  }

  /**
   *  @brief Returns a value indicating whether that stream supports seek
   */
  virtual bool supports_seek () 
  {
    return false;
  }

  /**
   *  @brief Returns a value indicating whether that stream is compressing
   */
  virtual bool is_compressing () const
  {
    return false;
  }

  /**
   *  @brief Rejects the output - for delegates supporting unrolling, this means the original file is restored
   */
  virtual void reject ()
  {
    //  ... the default implementation does not support this feature ..
  }

private:
  //  No copying
  OutputStreamBase (const OutputStreamBase &);
  OutputStreamBase &operator= (const OutputStreamBase &);
};

/**
 *  @brief A string output delegate
 *
 *  Implements the writer to a memory buffer
 */
class TL_PUBLIC OutputMemoryStream
  : public OutputStreamBase
{
public:
  /**
   *  @brief Create a string writer
   */
  OutputMemoryStream ()
  {
    m_buffer.reserve (65336);
  }

  /**
   *  @brief Write to a string 
   *
   *  Implements the basic write method. 
   */
  virtual void write (const char *b, size_t n)
  {
    m_buffer.insert (m_buffer.end (), b, b + n);
  }

  /**
   *  @brief Get the address pointing to the data
   */
  const char *data () const
  {
    return & m_buffer.front ();
  }

  /**
   *  @brief Gets the size of the data stored
   */
  size_t size () const
  {
    return m_buffer.size ();
  }

  /**
   *  @brief Clears the buffer
   */
  void clear ()
  {
    m_buffer.clear ();
  }

private:
  //  No copying
  OutputMemoryStream (const OutputMemoryStream &);
  OutputMemoryStream &operator= (const OutputMemoryStream &);

  std::vector<char> m_buffer;
};

/**
 *  @brief A string output delegate
 *
 *  Implements the writer to a string
 */
class TL_PUBLIC OutputStringStream
  : public OutputStreamBase
{
public:
  /**
   *  @brief Create a string writer
   */
  OutputStringStream ()
  {
    m_stream.imbue (std::locale ("C"));
  }

  /**
   *  @brief Write to a string 
   *
   *  Implements the basic write method. 
   */
  virtual void write (const char *b, size_t n)
  {
    m_stream.write (b, n);
  }

  /**
   *  @brief Seek to the specified position 
   *
   *  Writing continues at that position after a seek.
   */
  void seek (size_t s)
  {
    m_stream.seekp (s);
  }

  /**
   *  @brief Returns a value indicating whether that stream supports seek
   */
  bool supports_seek () 
  {
    return true;
  }

  /**
   *  @brief Get the content as a STL string
   *
   *  This method will return an char pointer containing the data written. 
   */
  std::string string () 
  {
    return m_stream.str ();
  }

private:
  //  No copying
  OutputStringStream (const OutputStringStream &);
  OutputStringStream &operator= (const OutputStringStream &);

  std::ostringstream m_stream;
};

/**
 *  @brief A base for file writer delegates
 *
 *  This class mainly provides safety services for the file writer.
 *  When writing a file, it will keep a backup until the file actually
 *  has been written. This way, a network or disk full error will not
 *  compromise the file's content. The backup file name is created
 *  by appending ".~backup" to the original file path.
 *
 *  In addition, a specified number or persistent backup files can
 *  be kept. The first backup will be called like the original file
 *  with ".1" appended. The second will be called ".2" etc.
 *  The backups will be shuffled, so ".1" is always the most recent
 *  one while older ones get bigger numbers.
 *
 *  The number of backups can be specified with "keep_backups". If this
 *  count is zero, no backups will be kept. If the count is larger than
 *  zero, the specified maximum number of backups is kept. If less than
 *  zero, an infinite number of backups is made. But beware: shuffling
 *  will become increasingly expensive.
 */
class TL_PUBLIC OutputFileBase
  : public OutputStreamBase
{
public:
  /**
   *  @brief Constructor
   *
   *  @param path The (relative) path of the file to write
   *  @param keep_backups The number of backups to keep (0: none, -1: infinite)
   */
  OutputFileBase (const std::string &path, int keep_backups);

  /**
   *  @brief Destructor
   */
  virtual ~OutputFileBase ();

  /**
   *  @brief Seek to the specified position
   *
   *  Writing continues at that position after a seek.
   */
  virtual void seek (size_t s);

  /**
   *  @brief Write to a file
   *
   *  Implements the basic write method.
   *  Will throw a FileWriteErrorException if an error occurs.
   */
  virtual void write (const char *b, size_t n);

  /**
   *  @brief Unrolls the output
   */
  virtual void reject ();

  /**
   *  @brief Gets the actual path
   */
  const std::string &path () const
  {
    return m_path;
  }

  /**
   *  @brief Gets the path of the backup file
   */
  const std::string &backup_path () const
  {
    return m_backup_path;
  }

protected:
  virtual void seek_file (size_t s) = 0;
  virtual void write_file (const char *b, size_t n) = 0;

private:
  int m_keep_backups;
  std::string m_backup_path, m_path;
  bool m_has_error;
};

/**
 *  @brief A zlib output file delegate
 *
 *  Implements the writer for a zlib stream
 */
class TL_PUBLIC OutputZLibFile
  : public OutputFileBase
{
public:
  /**
   *  @brief Open a file with the given path
   *
   *  Opening a file is a prerequisite for reading from the
   *  object. open() will throw a FileOpenErrorException if
   *  an error occurs.
   *
   *  @param path The (relative) path of the file to write
   *  @param keep_backups The number of backups to keep (0: none, -1: infinite)
   */
  OutputZLibFile (const std::string &path, int keep_backups);

  /**
   *  @brief Close the file
   *
   *  The destructor will automatically close the file.
   */
  virtual ~OutputZLibFile ();

protected:
  /**
   *  @brief Write to a file
   *
   *  Implements the basic write method.
   *  Will throw a ZLibWriteErrorException if an error occurs.
   */
  virtual void write_file (const char *b, size_t n);

  /**
   *  @brief The seek operation isn't implemented for zlib files
   */
  virtual void seek_file (size_t /*s*/) { }

  /**
   *  @brief Returns a value indicating whether this steam is compressing
   */
  virtual bool is_compressing () const { return true; }

private:
  //  No copying
  OutputZLibFile (const OutputZLibFile &);
  OutputZLibFile &operator= (const OutputZLibFile &);

  ZLibFilePrivate *mp_d;
};

/**
 *  @brief A simple output file delegate
 *
 *  Implements the writer for ordinary files.
 */
class TL_PUBLIC OutputFile
  : public OutputFileBase
{
public:
  /**
   *  @brief Open a file with the given path
   *
   *  Opening a file is a prerequisite for reading from the
   *  object. open() will throw a FileOpenErrorException if
   *  an error occurs.
   *
   *  @param path The (relative) path of the file to write
   *  @param keep_backups The number of backups to keep (0: none, -1: infinite)
   */
  OutputFile (const std::string &path, int keep_backups = 0);

  /**
   *  @brief Close the file
   *
   *  The destructor will automatically close the file.
   */
  virtual ~OutputFile ();

  /**
   *  @brief Returns a value indicating whether that stream supports seek
   */
  virtual bool supports_seek ()
  {
    return true;
  }

protected:
  /**
   *  @brief Seek to the specified position
   *
   *  Writing continues at that position after a seek.
   */
  virtual void seek_file (size_t s);

  /**
   *  @brief Write to a file
   *
   *  Implements the basic write method.
   *  Will throw a FileWriteErrorException if an error occurs.
   */
  virtual void write_file (const char *b, size_t n);

private:
  //  No copying
  OutputFile (const OutputFile &);
  OutputFile &operator= (const OutputFile &);

  int m_fd;
};

/**
 *  @brief A simple pipe output delegate
 *
 *  Implements the writer for pipe streams
 */
class TL_PUBLIC OutputPipe
  : public OutputStreamBase
{
public:
  /**
   *  @brief Open a stream by connecting with the stdout of a given command
   *
   *  Opening a pipe is a prerequisite for reading from the
   *  object. open() will throw a FilePOpenErrorException if
   *  an error occurs - commonly if the command cannot be executed.
   *  This implementation is based on popen ().
   *
   *  @param cmd The command to execute
   *  @param read True, if the file should be read, false on write.
   */
  OutputPipe (const std::string &path);

  /**
   *  @brief Close the pipe
   *
   *  The destructor will automatically close the pipe.
   */
  virtual ~OutputPipe ();

  /**
   *  @brief Write to a file
   *
   *  Implements the basic write method.
   *  Will throw a FilePWriteErrorException if an error occurs.
   */
  virtual void write (const char *b, size_t n);

private:
  //  No copying
  OutputPipe (const OutputPipe &);
  OutputPipe &operator= (const OutputPipe &);

  FILE *m_file;
  std::string m_source;
};

// ---------------------------------------------------------------------------------

/**
 *  @brief An output stream abstraction class
 *
 *  The basic objective of this interface is to provide
 *  the capability to write a block of n bytes into a buffer.
 *  The actual stream access is delegated to another object.
 */

class TL_PUBLIC OutputStream
{
public:
  /**
   *  @brief Definitions of the output options
   */
  enum OutputStreamMode
  {
    /**
     *  @brief Without compression
     */
    OM_Plain = 0,

    /**
     *  @brief With zlib compression
     */
    OM_Zlib = 1,

    /**
     *  @brief Determine from path (.gz -> zlib compression)
     */
    OM_Auto = 2
  };

  /**
   *  @brief Determine the output mode from the filename and a given mode
   *
   *  This method will replace OM_Auto by the appropriate mode given by the 
   *  file name.
   */
  static OutputStreamMode output_mode_from_filename (const std::string &abstract_path, OutputStreamMode om = OM_Auto);

  /**
   *  @brief Default constructor
   *
   *  This constructor takes a delegate object. 
   */
  OutputStream (OutputStreamBase &delegate, bool as_text = false);

  /**
   *  @brief Default constructor
   *
   *  This constructor takes a delegate object. The stream will own the delegate.
   */
  OutputStream (OutputStreamBase *delegate, bool as_text = false);

  /**
   *  @brief Open an output stream with the given path and stream mode
   *
   *  This will automatically create a delegate object and delete it later.
   *  If "as_text" is true, the output will be formatted with the system's line separator.
   */
  OutputStream (const std::string &abstract_path, OutputStreamMode om = OM_Auto, bool as_text = false, int keep_backups = 0);

  /**
   *  @brief Destructor
   *
   *  This will delete the delegate object passed in the constructor.
   */
  virtual ~OutputStream ();

  /**
   *  @brief Closes the stream - after closing, the stream can't be accessed anymore
   */
  void close ();

  /** 
   *  @brief This is the outer write method to call
   *  
   *  This implementation writes data through the 
   *  protected write call.
   */
  void put (const char *b, size_t n);

  /**
   *  @brief Puts a C string (UTF-8) to the output
   */
  void put (const char *s)
  {
    put (s, strlen (s));
  }

  /**
   *  @brief Puts a STL string (UTF-8) to the output
   */
  void put (const std::string &s)
  {
    put (s.c_str (), s.size ());
  }

  /**
   *  @brief << operator
   */
  OutputStream &operator<< (char s)
  {
    put (&s, 1);
    return *this;
  }

  /**
   *  @brief << operator
   */
  OutputStream &operator<< (unsigned char s)
  {
    put ((const char *) &s, 1);
    return *this;
  }

  /**
   *  @brief << operator
   */
  OutputStream &operator<< (const char *s)
  {
    put (s);
    return *this;
  }

  /**
   *  @brief << operator
   */
  OutputStream &operator<< (const std::string &s)
  {
    put (s);
    return *this;
  }

  /**
   *  @brief << operator
   */
  template <class T>
  OutputStream &operator<< (const T &t)
  {
    put (tl::to_string (t));
    return *this;
  }

  /**
   *  @brief Rejects the output - for delegates which support backup, this means the original file is restored
   */
  void reject () const
  {
    if (mp_delegate) {
      mp_delegate->reject ();
    }
  }

  /**
   *  @brief Returns a value indicating whether that stream is compressing
   */
  bool is_compressing () const
  {
    return mp_delegate != 0 && mp_delegate->is_compressing ();
  }

  /**
   *  @brief Returns a value indicating whether that stream supports seek
   */
  bool supports_seek () const
  {
    return mp_delegate != 0 && mp_delegate->supports_seek ();
  }

  /**
   *  @brief Seek to the specified position 
   *
   *  Writing continues at that position after a seek.
   *  Seek is not supported while in deflate mode.
   */
  void seek (size_t s);

  /**
   *  @brief Obtain the current file position
   */
  size_t pos () const 
  {
    return m_pos;
  }
    
  /**
   *  @brief Flush buffered data
   */
  void flush ();

  /**
   *  @brief Gets the path that was specified in the constructor
   */
  const std::string &path () const
  {
    return m_path;
  }

  /**
   *  @brief Configures the stream for text output
   */
  void set_as_text (bool f);

protected:
  void reset_pos ()
  {
    m_pos = 0;
  }

private:
  size_t m_pos;
  OutputStreamBase *mp_delegate;
  bool m_owns_delegate;
  bool m_as_text;
  char *mp_buffer;
  size_t m_buffer_capacity, m_buffer_pos;
  std::string m_path;

  void put_raw (const char *b, size_t n);

  //  No copying currently
  OutputStream (const OutputStream &);
  OutputStream &operator= (const OutputStream &);
};

}

#endif

