
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


#ifndef HDR_tlHttpStreamQt
#define HDR_tlHttpStreamQt

#include "tlStream.h"
#include "tlEvents.h"

#include <QObject>
#include <QBuffer>
#include <QByteArray>
#include <QTimer>
#include <QNetworkAccessManager>
#include <memory>

class QNetworkReply;
class QNetworkProxy;
class QAuthenticator;

namespace tl
{

class HttpCredentialProvider;
class InputHttpStream;

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

class InputHttpStreamPrivateData
  : public QObject
{
Q_OBJECT

public:
  InputHttpStreamPrivateData (InputHttpStream *stream, const std::string &url);

  virtual ~InputHttpStreamPrivateData ();

  void send ();
  void close ();
  void set_request (const char *r);
  void set_data (const char *data);
  void set_data (const char *data, size_t n);
  void add_header (const std::string &name, const std::string &value);
  void set_timeout (double to);
  double timeout () const;

  tl::Event &ready ()
  {
    return m_ready;
  }

  bool data_available ()
  {
    return mp_reply != 0;
  }

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

private slots:
  void finished (QNetworkReply *);
  void resend ();
#if !defined(QT_NO_SSL)
  void sslErrors (QNetworkReply *reply, const QList<QSslError> &errors);
#endif

private:
  std::string m_url;
  QNetworkReply *mp_reply;
  std::unique_ptr<QNetworkReply> mp_active_reply;
  QByteArray m_request;
  QByteArray m_data;
  QBuffer *mp_buffer;
  std::map<std::string, std::string> m_headers;
  tl::Event m_ready;
  QTimer *mp_resend_timer;
  std::string m_ssl_errors;
  double m_timeout;
  InputHttpStream *mp_stream;

  void issue_request (const QUrl &url);
};

}

#endif

