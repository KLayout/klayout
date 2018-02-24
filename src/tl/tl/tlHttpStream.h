
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#include "tlStream.h"
#include "tlEvents.h"

#include <QObject>
#include <QBuffer>
#include <QByteArray>
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkProxy;
class QAuthenticator;

namespace tl
{

class TL_PUBLIC HttpErrorException
  : public tl::Exception
{
public:
  HttpErrorException (const std::string &f, int en, const std::string &url)
    : tl::Exception (tl::to_string (QObject::tr ("Error %d: %s, fetching %s")), en, f, url)
  { }
};

class AuthenticationHandler
  : public QObject
{
Q_OBJECT

public:
  AuthenticationHandler ();

public slots:
  void authenticationRequired (QNetworkReply *, QAuthenticator *);
  void proxyAuthenticationRequired (const QNetworkProxy &, QAuthenticator *);
  void reset ();

private:
  int m_retry, m_proxy_retry;
};

/**
 *  @brief A http input delegate for tl::InputStream
 *
 *  Implements the reader from a server using the HTTP protocol
 */
class TL_PUBLIC InputHttpStream
  : public QObject, public InputStreamBase
{
Q_OBJECT

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
   *  Connect to this event for the asynchroneous interface.
   */
  tl::Event &ready ()
  {
    return m_ready;
  }

  /**
   *  @brief Gets a value indicating whether data is available
   */
  bool data_available ()
  {
    return mp_reply != 0;
  }

  virtual void reset ();  

  virtual std::string source () const
  {
    return m_url;
  }

  virtual std::string absolute_path () const
  {
    return m_url;
  }

  virtual std::string filename () const;

private slots:
  void finished (QNetworkReply *);

private:
  std::string m_url;
  QNetworkReply *mp_reply;
  std::auto_ptr<QNetworkReply> mp_active_reply;
  QByteArray m_request;
  QByteArray m_data;
  QBuffer *mp_buffer;
  std::map<std::string, std::string> m_headers;
  tl::Event m_ready;

  void issue_request (const QUrl &url);
};

}

#endif

