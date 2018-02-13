
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


#include "tlHttpStream.h"
#include "tlLog.h"
#include "tlStaticObjects.h"
#include "tlDeferredExecution.h"

#include "ui_PasswordDialog.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QAuthenticator>
#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QDialog>

namespace tl
{

// ---------------------------------------------------------------
//  PasswordDialog definition and implementation

class PasswordDialog
  : public QDialog, private Ui::PasswordDialog
{
public:
  PasswordDialog (QWidget *parent)
    : QDialog (parent)
  {
    setupUi (this);
  }
  
  bool exec_auth (bool proxy, int attempt, const QString &where, QAuthenticator *auth)
  {
    realm_label->setText (tr ("<b>Realm:</b> ") + auth->realm ());
    if (proxy) {
      where_label->setText (tr ("<b>Proxy:</b> ") + where);
    } else {
      where_label->setText (tr ("<b>URL:</b> ") + where);
    }

    if (attempt > 1) {
      attempt_label->setText (tr ("Authentication failed - please try again"));
      attempt_label->show ();
    } else {
      attempt_label->hide ();
    }

    if (QDialog::exec ()) {
      auth->setPassword (password_le->text ());
      auth->setUser (user_le->text ());
      return true;
    } else {
      return false;
    }

  }
};

// ---------------------------------------------------------------
//  AuthenticationHandler implementation

AuthenticationHandler::AuthenticationHandler ()
  : QObject (0), m_retry (0), m_proxy_retry (0)
{
  //  .. nothing yet ..
}

void
AuthenticationHandler::reset ()
{
  m_retry = 0;
  m_proxy_retry = 0;
}

void
AuthenticationHandler::authenticationRequired (QNetworkReply *reply, QAuthenticator *auth)
{
  PasswordDialog pw_dialog (0 /*no parent*/);
  pw_dialog.exec_auth (false, ++m_retry, reply->url ().toString (), auth);
}

void
AuthenticationHandler::proxyAuthenticationRequired (const QNetworkProxy &proxy, QAuthenticator *auth)
{
  PasswordDialog pw_dialog (0 /*no parent*/);
  pw_dialog.exec_auth (true, ++m_proxy_retry, proxy.hostName (), auth);
}

// ---------------------------------------------------------------
//  InputHttpFile implementation

static QNetworkAccessManager *s_network_manager (0);
static AuthenticationHandler *s_auth_handler (0);

InputHttpStream::InputHttpStream (const std::string &url)
  : m_url (url), mp_reply (0), m_request ("GET"), mp_buffer (0)
{
  if (! s_network_manager) {

    s_network_manager = new QNetworkAccessManager (0);
    s_auth_handler = new AuthenticationHandler ();
    connect (s_network_manager, SIGNAL (authenticationRequired (QNetworkReply *, QAuthenticator *)), s_auth_handler, SLOT (authenticationRequired (QNetworkReply *, QAuthenticator *)));
    connect (s_network_manager, SIGNAL (proxyAuthenticationRequired (const QNetworkProxy &, QAuthenticator *)), s_auth_handler, SLOT (proxyAuthenticationRequired (const QNetworkProxy &, QAuthenticator *)));

    tl::StaticObjects::reg (&s_network_manager);
    tl::StaticObjects::reg (&s_auth_handler);

  }

  connect (s_network_manager, SIGNAL (finished (QNetworkReply *)), this, SLOT (finished (QNetworkReply *)));
}

InputHttpStream::~InputHttpStream ()
{
  // .. nothing yet ..
}

void
InputHttpStream::set_request (const char *r)
{
  m_request = QByteArray (r);
}

void
InputHttpStream::set_data (const char *data)
{
  m_data = QByteArray (data);
}

void
InputHttpStream::set_data (const char *data, size_t n)
{
  m_data = QByteArray (data, int (n));
}

void
InputHttpStream::add_header (const std::string &name, const std::string &value)
{
  m_headers.insert (std::make_pair (name, value));
}

void 
InputHttpStream::finished (QNetworkReply *reply)
{
  if (reply != mp_active_reply.get ()) {
    return;
  }

  QVariant redirect_target = reply->attribute (QNetworkRequest::RedirectionTargetAttribute);
  if (reply->error () == QNetworkReply::NoError && ! redirect_target.isNull ()) {
    m_url = tl::to_string (redirect_target.toString ());
    if (tl::verbosity() >= 30) {
      tl::info << "HTTP redirect to: " << m_url;
    }
    issue_request (QUrl (redirect_target.toString ()));
  } else {
    mp_reply = reply;
    m_ready ();
  }
}

void
InputHttpStream::issue_request (const QUrl &url)
{
  delete mp_buffer;
  mp_buffer = 0;

  //  reset the retry counters -> this way we can detect authentication failures
  s_auth_handler->reset ();

  QNetworkRequest request (url);
  if (tl::verbosity() >= 30) {
    tl::info << "HTTP request URL: " << url.toString ().toUtf8 ().constData ();
  }
  for (std::map<std::string, std::string>::const_iterator h = m_headers.begin (); h != m_headers.end (); ++h) {
    if (tl::verbosity() >= 40) {
      tl::info << "HTTP request header: " << h->first << ": " << h->second;
    }
    request.setRawHeader (QByteArray (h->first.c_str ()), QByteArray (h->second.c_str ()));
  }

#if QT_VERSION < 0x40700
  if (m_request == "GET" && m_data.isEmpty ()) {
    mp_active_reply.reset (s_network_manager->get (request));
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("Custom HTTP requests are not supported in this build (verb is %1)").arg (QString::fromUtf8 (m_request))));
  }
#else
  if (m_data.isEmpty ()) {
    mp_active_reply.reset (s_network_manager->sendCustomRequest (request, m_request));
  } else {
    if (tl::verbosity() >= 40) {
      tl::info << "HTTP request data: " << m_data.constData ();
    }
    mp_buffer = new QBuffer (&m_data);
    mp_active_reply.reset (s_network_manager->sendCustomRequest (request, m_request, mp_buffer));
  }
#endif
}

void
InputHttpStream::send ()
{
  if (mp_reply == 0) {
    issue_request (QUrl (tl::to_qstring (m_url)));
  }
}

size_t 
InputHttpStream::read (char *b, size_t n)
{
  //  Prevents deferred methods to be executed during the processEvents below (undesired side effects)
  tl::NoDeferredMethods silent;

  if (mp_reply == 0) {
    issue_request (QUrl (tl::to_qstring (m_url)));
  }

  //  TODO: progress, timeout
  while (mp_reply == 0) {
    QCoreApplication::processEvents (QEventLoop::ExcludeUserInputEvents);
  }

  if (mp_reply->error () != QNetworkReply::NoError) {

    //  throw an error 
    std::string em = tl::to_string (mp_reply->attribute (QNetworkRequest::HttpReasonPhraseAttribute).toString ());
    if (tl::verbosity() >= 30) {
      tl::info << "HTTP response error: " << em;
    }

    int ec = mp_reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
    if (ec == 0) {
      switch (mp_reply->error ()) {
      case QNetworkReply::ConnectionRefusedError:
        em = tl::to_string (QObject::tr ("Connection refused"));
        break;
      case QNetworkReply::RemoteHostClosedError:
        em = tl::to_string (QObject::tr ("Remote host closed connection"));
        break;
      case QNetworkReply::HostNotFoundError:
        em = tl::to_string (QObject::tr ("Host not found"));
        break;
      case QNetworkReply::TimeoutError:
        em = tl::to_string (QObject::tr ("Timeout"));
        break;
      case QNetworkReply::ContentAccessDenied:
        em = tl::to_string (QObject::tr ("Access denied"));
        break;
      case QNetworkReply::ContentNotFoundError:
        em = tl::to_string (QObject::tr ("Content not found"));
        break;
      default:
        em = tl::to_string (QObject::tr ("Network API error"));
      }
      ec = int (mp_reply->error ());
    }

    throw HttpErrorException (em, ec, m_url);

  }

  QByteArray data = mp_reply->read (n);
  memcpy (b, data.constData (), data.size ());
  if (tl::verbosity() >= 40) {
    tl::info << "HTTP reponse data read: " << data.constData ();
  }
  return data.size ();
}

void 
InputHttpStream::reset ()
{
  throw tl::Exception (tl::to_string (QObject::tr ("'reset' is not supported on HTTP input streams")));
}

std::string 
InputHttpStream::filename () const
{
  return tl::to_string (QFileInfo (QUrl (tl::to_qstring (m_url)).path ()).fileName ());
}

}
