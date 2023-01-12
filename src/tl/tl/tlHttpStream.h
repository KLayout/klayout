
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

#ifndef HDR_tlHttpStream
#define HDR_tlHttpStream

#include "tlObject.h"
#include "tlException.h"
#include "tlStream.h"
#include "tlEvents.h"

class QNetworkReply;

namespace tl
{

/**
 *  @brief A callback interface to provide the authentication data
 */
class TL_PUBLIC HttpCredentialProvider
  : public tl::Object
{
public:
  HttpCredentialProvider () { }
  virtual ~HttpCredentialProvider () { }

  /**
   *  @brief Gets the user name and password for the given URL and authentication realm
   */
  virtual bool user_password (const std::string &url, const std::string &realm, bool proxy, int attempt, std::string &user, std::string &passwd) = 0;
};

/**
 *  @brief An exception class for HTTP errors
 */
class TL_PUBLIC HttpErrorException
  : public tl::Exception
{
public:
  HttpErrorException (const std::string &f, int ec, const std::string &url, const std::string &body = std::string ())
    : tl::Exception (format_error (f, ec, url, body))
  { }

  static std::string format_error (const std::string &em, int ec, const std::string &url, const std::string &body);
};

/**
 *  @brief A callback function during waiting for a response
 */
class TL_PUBLIC InputHttpStreamCallback
{
public:
  InputHttpStreamCallback () { }
  virtual ~InputHttpStreamCallback () { }

  virtual void wait_for_input () { }
};

class InputHttpStreamPrivateData;

/**
 *  @brief A http input delegate for tl::InputStream
 *
 *  Implements the reader from a server using the HTTP protocol
 */
class TL_PUBLIC InputHttpStream
  : public InputStreamBase
{
public:
  /**
   *  @brief Open a stream with the given URL
   */
  InputHttpStream (const std::string &url);

  /**
   *  @brief Close the file
   *
   *  The destructor will automatically close the connection.
   */
  virtual ~InputHttpStream ();

  /**
   *  @brief Sets the credential provider
   */
  static void set_credential_provider (HttpCredentialProvider *cp);

  /**
   *  @brief Returns true, if HTTP support is compiled in
   */
  static bool is_available ();

  /**
   *  @brief Polling: call this function regularly to explicitly establish polling
   *  (in the Qt framework, this is done automatically within the event loop)
   *  May throw a tl::CancelException to stop.
   *  Returns true if a message has arrived.
   */
  void tick ();

  /**
   *  @brief Sets a timeout callback
   *  The callback's wait_for_input method is called regularily while the stream
   *  waits for HTTP responses.
   *  The implementation may throw a tl::CancelException to stop the polling.
   */
  void set_callback (tl::InputHttpStreamCallback *callback)
  {
    mp_callback = callback;
  }

  /**
   *  @brief Sets the timeout in seconds
   */
  void set_timeout (double to);

  /**
   *  @brief Gets the timeout in seconds or zero if no timeout is set.
   */
  double timeout () const;

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
   *  @brief Closes the connection
   */
  void close ();

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
   *  @brief Gets the "ready" event
   *  Connect to this event for the asynchronous interface.
   *  This event is fired when data becomes available or the
   *  connection has terminated with an error.
   */
  tl::Event &ready ();

  /**
   *  @brief Gets a value indicating whether data is available
   */
  bool data_available ();

  /**
   *  @brief Read from the stream
   *  Implements the basic read method.
   */
  virtual size_t read (char *b, size_t n);

  virtual void reset ();
  virtual std::string source () const;
  virtual std::string absolute_path () const;
  virtual std::string filename () const;

private:
  InputHttpStreamPrivateData *mp_data;
  InputHttpStreamCallback *mp_callback;
};

}

#endif

