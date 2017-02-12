
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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

#include <QObject>

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
   *  @brief Read from the stream 
   *
   *  Implements the basic read method. 
   */
  virtual size_t read (char *b, size_t n);

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

private:
  std::string m_url;
  QNetworkReply *mp_reply;

private slots:
  void finished (QNetworkReply *);
  void authenticationRequired (QNetworkReply *, QAuthenticator *);
  void proxyAuthenticationRequired (const QNetworkProxy &, QAuthenticator *);
};

}

#endif

