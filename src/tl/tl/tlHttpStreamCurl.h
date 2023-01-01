
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

#ifndef HDR_tlHttpStreamCurl
#define HDR_tlHttpStreamCurl

#include "tlStream.h"
#include "tlEvents.h"
#include "tlObject.h"
#include "tlProgress.h"

#include <memory>

namespace tl
{

class CurlConnection;
class HttpCredentialProvider;

/**
 *  @brief A http input delegate for tl::InputStream
 *
 *  Implements the reader from a server using the HTTP protocol
 */
class TL_PUBLIC InputHttpStreamPrivateData
  : public tl::Object
{
public:
  /**
   *  @brief Open a stream with the given URL
   */
  InputHttpStreamPrivateData (InputHttpStream *stream, const std::string &url);

  /**
   *  @brief Close the file
   *
   *  The destructor will automatically close the connection.
   */
  virtual ~InputHttpStreamPrivateData ();

  /**
   *  @brief Sends the request for data
   *  To ensure prompt delivery of data, this method can be used prior to
   *  "read" to trigger the download from the given URL.
   *  This method will return immediately. When the reply is available,
   *  the "ready" event will be triggered. "read" can then be used to
   *  read the data or - in case of an error - throw an exception.
   *  If "send" is not used before "read", "read" will block until data
   *  is available.
   *  If a request has already been sent, this method will do nothing.
   */
  void send ();

  /**
   *  @brief Sets the request verb
   *  The default verb is "GET"
   */
  void set_request (const char *r);

  /**
   *  @brief Sets data to be sent with the request
   *  If data is given, it is sent along with the request.
   *  This version takes a null-terminated string.
   */
  void set_data (const char *data);

  /**
   *  @brief Sets data to be sent with the request
   *  If data is given, it is sent along with the request.
   *  This version takes a data plus length.
   */
  void set_data (const char *data, size_t n);

  /**
   *  @brief Sets a header field
   */
  void add_header (const std::string &name, const std::string &value);

  /**
   *  @brief Read from the stream
   *  Implements the basic read method.
   */
  virtual size_t read (char *b, size_t n);

  /**
   *  @brief Gets the "ready" event
   *  Connect to this event for the asynchronous interface.
   *  This event is fired when the request has finished.
   */
  tl::Event &ready ()
  {
    return m_ready_event;
  }

  /**
   *  @brief Checks for errors
   *  This method can be used after the ready event to check for errors.
   *  It will throw an exception if errors occurred.
   *  read() will do the same.
   */
  void check ();

  /**
   *  @brief Gets the "data available" event
   *  Connect to this event for the asynchronous interface.
   *  This event is fired when data becomes available for read.
   *  It is just fired once.
   */
  tl::Event &data_ready ()
  {
    return m_data_ready_event;
  }

  /**
   *  @brief Gets a value indicating whether data is available
   */
  bool data_available ();

  /**
   *  @brief Sets the timeout in seconds
   */
  void set_timeout (double to);

  /**
   *  @brief Gets the timeout in seconds
   */
  double timeout () const;

  //  Basic interface
  virtual void reset ();
  virtual void close ();
  virtual std::string source () const;
  virtual std::string absolute_path () const;
  virtual std::string filename () const;

private:
  std::unique_ptr<CurlConnection> m_connection;
  tl::Event m_ready_event;
  tl::Event m_data_ready_event;
  bool m_sent;
  bool m_ready;
  std::unique_ptr<tl::AbsoluteProgress> m_progress;
  double m_timeout;
  InputHttpStream *mp_stream;

  void on_data_available ();
  void on_finished ();
};

}

#endif

