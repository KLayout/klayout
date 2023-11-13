
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

#define NOMINMAX   //  for windows.h -> min/max not defined

#include "tlHttpStream.h"
#include "tlHttpStreamCurl.h"
#include "tlLog.h"
#include "tlStaticObjects.h"
#include "tlDeferredExecution.h"
#include "tlEvents.h"
#include "tlAssert.h"
#include "tlStaticObjects.h"
#include "tlProgress.h"
#include "tlFileUtils.h"
#include "tlUri.h"

#if !defined(_MSC_VER)
# include <sys/time.h>
# include <unistd.h>
#else
# include <WinSock2.h>
#endif

#include <curl/curl.h>

#include <map>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <memory>
#include <algorithm>

// #define DEBUG_CURL 1


namespace tl
{

// ---------------------------------------------------------------
//  Utilities

std::string server_from_url (const std::string &url)
{
  tl::URI uri (url);
  return uri.scheme () + "://" + uri.authority ();
}

std::string parse_realm (const std::string &header)
{
  std::vector<std::string> lines = tl::split (header, "\n");
  for (std::vector<std::string>::const_iterator l = lines.begin (); l != lines.end (); ++l) {

    tl::Extractor ex (l->c_str ());
    std::string header;
    if (! ex.try_read_word (header, "_.$-") || ! ex.test (":")) {
      continue;
    }

    header = tl::to_lower_case (header);
    if (header != "www-authenticate" && header != "proxy-authenticate") {
      continue;
    }

    std::string auth_type;
    if (! ex.try_read_word (auth_type)) {
      continue;
    }

    while (! ex.at_end ()) {
      std::string key, value;
      if (! ex.try_read_word (key) || ! ex.test ("=") || ! ex.try_read_word_or_quoted (value)) {
        break;
      }
      key = tl::to_lower_case (key);
      if (key == "realm") {
        return value;
      }
    }

  }

  return std::string ();
}



// ---------------------------------------------------------------
//  CurlCredentialManager definition and implementation

/**
 *  @brief A cache for keeping the credentials
 */
class CurlCredentialManager
{
public:

  enum Mode {
    UseAsIs,
    Inquire,
    ForceInquire
  };

  CurlCredentialManager (bool proxy)
    : m_proxy (proxy), mp_provider (0)
  {
    //  .. nothing yet ..
  }

  const std::pair<std::string, std::string> *user_password (const std::string &url, const std::string &realm, int attempt, Mode mode)
  {
    std::string server = server_from_url (url);

    if (mode != ForceInquire) {

      std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string> >::const_iterator c = m_credentials.find (std::make_pair (server, realm));
      if (c != m_credentials.end ()) {
        return &c->second;
      }

    }

    if (mode != UseAsIs && mp_provider.get ()) {

      std::string user, password;
      if (! mp_provider->user_password (url, realm, m_proxy, attempt, user, password)) {
        throw tl::CancelException ();
      }

      set_credentials (server, realm, user, password);
      return user_password (url, realm, attempt, CurlCredentialManager::UseAsIs);

    } else {
      return 0;
    }
  }

  void set_credentials (const std::string &url, const std::string &realm, const std::string &user, const std::string &passwd)
  {
    m_credentials[std::make_pair (url, realm)] = std::make_pair (user, passwd);
  }

  void set_provider (HttpCredentialProvider *provider)
  {
    mp_provider.reset (provider);
  }

  HttpCredentialProvider *provider ()
  {
    return mp_provider.get ();
  }

private:
  std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string> > m_credentials;
  bool m_proxy;
  tl::weak_ptr<HttpCredentialProvider> mp_provider;
};

// ---------------------------------------------------------------
//  ChunkedBuffer definition and implementation

/**
 *  @brief A stream buffer
 *  The stream buffer can push data in chunks and
 *  deliver data in chunks of different size.
 *  Internally, the data is kept in the original chunks
 *  and is combined on output.
 */
class ChunkedBuffer
{
private:
  struct ChunkInfo
  {
    ChunkInfo ()
      : pos (0), start (0), size (0)
    {
      //  .. nothing yet ..
    }

    ChunkInfo (const ChunkInfo &other)
      : pos (0), start (0), size (0)
    {
      operator= (other);
    }

    ChunkInfo &operator= (const ChunkInfo &other)
    {
      if (this != &other) {
        set (other.start, other.size);
        pos = start + (other.pos - other.start);
      }
      return *this;
    }

    ~ChunkInfo ()
    {
      set (0, 0);
    }

    void set (const char *data, size_t n)
    {
      if (start) {
        delete [] start;
        start = pos = 0;
      }

      size = n;
      if (n > 0) {
        char *data_copy = new char [n];
        memcpy (data_copy, data, n);
        pos = start = data_copy;
      }
    }

    size_t fetch (char *data, size_t bytes)
    {
      size_t n = std::min (bytes, available ());
      if (n > 0) {
        memcpy (data, pos, n);
        pos += n;
      }
      return n;
    }

    bool empty () const
    {
      return available () == 0;
    }

    size_t available () const
    {
      return size - (pos - start);
    }

    char *pos, *start;
    size_t size;
  };

public:
  ChunkedBuffer ()
    : m_current_chunk (m_chunks.end ())
  {
    //  .. nothing yet ..
  }

  void clear ()
  {
    m_chunks.clear ();
    m_current_chunk = m_chunks.end ();
  }

  void push (const char *data, size_t bytes)
  {
    if (bytes > 0) {
      m_chunks.push_back (ChunkInfo ());
      m_chunks.back ().set (data, bytes);
      if (m_current_chunk == m_chunks.end ()) {
        --m_current_chunk;
      }
    }
  }

  size_t fetch (char *data, size_t bytes)
  {
    char *start = data;

    while (bytes > 0 && m_current_chunk != m_chunks.end ()) {

      size_t n = m_current_chunk->fetch (data, bytes);
      data += n;
      bytes -= n;
      if (m_current_chunk->empty ()) {
        ++m_current_chunk;
      }

    }

    return data - start;
  }

  std::string to_string () const
  {
    std::string s;
    s.reserve (size ());
    for (std::list<ChunkInfo>::const_iterator c = m_chunks.begin (); c != m_chunks.end (); ++c) {
      s += std::string (c->pos, c->available ());
    }
    return s;
  }

  size_t size () const
  {
    size_t bytes = 0;
    for (std::list<ChunkInfo>::const_iterator c = m_chunks.begin (); c != m_chunks.end (); ++c) {
      bytes += c->size;
    }
    return bytes;
  }

  bool empty () const
  {
    for (std::list<ChunkInfo>::const_iterator c = m_chunks.begin (); c != m_chunks.end (); ++c) {
      if (! c->empty ()) {
        return false;
      }
    }
    return true;
  }

  size_t pos () const
  {
    size_t p = 0;
    for (std::list<ChunkInfo>::const_iterator c = m_chunks.begin (); c != m_current_chunk; ++c) {
      p += c->size;
    }
    if (m_current_chunk != m_chunks.end ()) {
      p += (m_current_chunk->pos - m_current_chunk->start);
    }
    return p;
  }

  void seek (size_t pos)
  {
    for (std::list<ChunkInfo>::iterator c = m_chunks.begin (); c != m_chunks.end (); ++c) {
      c->pos = c->start;
    }

    m_current_chunk = m_chunks.end ();
    for (std::list<ChunkInfo>::iterator c = m_chunks.begin (); c != m_chunks.end (); ++c) {
      if (pos < c->size) {
        m_current_chunk = c;
        c->pos = c->start + pos;
        break;
      } else {
        pos -= c->size;
      }
    }
  }

  std::list<ChunkInfo> m_chunks;
  std::list<ChunkInfo>::iterator m_current_chunk;
};

// ---------------------------------------------------------------
//  CurlConnection definition

/**
 *  @brief Represents a connection to a server
 *
 *  Objects of this class are created by the CurlNetworkManager and must
 *  be deleted by the caller when the connection is no longer required.
 *
 *  @code
 *    CurlNetworkManager mgr;
 *    std::unique_ptr<CurlConnection> conn (mgr.create_connection ());
 *    conn->set_url ("http://www.example.com");
 *    conn->send ();
 *    while (mgr.tick () > 0) {
 *      //  continue;
 *    }
 *  @endcode
 */
class CurlConnection
{
public:
  /**
   *  @brief Destructor
   */
  ~CurlConnection ();

  /**
   *  @brief Sets the URL to use for the request
   *  Use "send" to initiate the request.
   */
  void set_url (const char *url);

  /**
   *  @brief Gets the URL
   */
  const std::string &url () const
  {
    return m_url;
  }

  /**
   *  @brief Sets the custom request to use
   *  This will override the "GET" request used normally.
   */
  void set_request (const char *request);

  /**
   *  @brief Sets a custom header field to use for the request
   */
  void add_header (const char *header, const char *value);

  /**
   *  @brief Sets the request data (0 terminated string)
   */
  void set_data (const char *data);

  /**
   *  @brief Sets the request data
   */
  void set_data (const char *data, size_t n);

  /**
   *  @brief Gets the byte count of the data block read
   */
  size_t read_available () const;

  /**
   *  @brief Fetches a block of read data
   */
  size_t fetch_read_data (char *buffer, size_t nbytes);

  /**
   *  @brief Gets the read data as a string
   */
  std::string read_data_to_string () const;

  /**
   *  @brief Sends the request
   *  Note: the connection object that sends the request will receive the data.
   */
  void send ();

  /**
   *  @brief Closes this connection
   */
  void close ();

  /**
   *  @brief Gets the HTTP status after the finished_event has been triggered
   */
  int http_status () const
  {
    return m_http_status;
  }

  /**
   *  @brief Gets a value indicating whether the request has finished
   */
  bool finished () const
  {
    return m_finished;
  }

  /**
   *  @brief Checks the response once finished is true
   *  This method will throw an exception if an error occurred
   */
  void check () const;

  /**
   *  @brief This event is triggered when the transmission has finished
   */
  tl::Event finished_event;

  /**
   *  @brief This event is triggered when new data is available
   */
  tl::Event data_available_event;

private:
  //  no copying
  CurlConnection (const CurlConnection &);
  CurlConnection &operator= (const CurlConnection &);

  friend class CurlNetworkManager;
  friend size_t read_func (char *buffer, size_t size, size_t nitems, void *userdata);
  friend size_t seek_func (void *userdata, curl_off_t offset, int origin);
  friend size_t write_func (char *ptr, size_t size, size_t nmemb, void *userdata);
  friend size_t write_header_func (char *ptr, size_t size, size_t nmemb, void *userdata);

  void add_read_data (const char *data, size_t n);
  void add_header_data (const char *data, size_t n);
  size_t fetch_data (char *buffer, size_t nbytes);
  int seek (curl_off_t offset, int origin);

  void finished (int status);
  void init ();

  CurlConnection (CURL *handle);

  CURL *mp_handle;
  ChunkedBuffer m_data, m_read_data, m_header_data;
  char m_error_msg [CURL_ERROR_SIZE];
  std::string m_url, m_request;
  int m_authenticated;
  std::string m_user, m_password;
  struct curl_slist *mp_headers;
  int m_http_status;
  bool m_finished;
  int m_status;
};

// ---------------------------------------------------------------
//  CurlNetworkManager definition

/**
 *  @brief A singleton instance to manage the connections
 */
class CurlNetworkManager
{
public:
  /**
   *  @brief Constructor
   *  There must be one instance, not more.
   */
  CurlNetworkManager ();

  /**
   *  @brief Destructor
   */
  ~CurlNetworkManager ();

  /**
   *  @brief Creates a connection object
   *  Connection objects are the basic keys for implementing connections via curl.
   *  Use the returned object to send and receive data.
   *  The returned object must be deleted by the caller.
   */
  CurlConnection *create_connection ();

  /**
   *  @brief The credentials manager object
   */
  CurlCredentialManager &credentials ()
  {
    return m_credentials;
  }

  /**
   *  @brief The proxy_credentials manager object
   */
  CurlCredentialManager &proxy_credentials ()
  {
    return m_proxy_credentials;
  }

  /**
   *  @brief Must be called in regular intervals to update the status
   *  Returns the number of open connections.
   */
  void tick ();

  /**
   *  @brief Returns true if a reply has arrived
   */
  bool has_reply () const;

  /**
   *  @brief The singleton instance
   */
  static CurlNetworkManager *instance ();

private:
  //  TODO: mutex for thread locking
  friend class CurlConnection;

  void add_connection (CurlConnection *connection);
  void release_connection (CurlConnection *connection);
  void start (CurlConnection *connection);
  void on_tick ();

  tl::DeferredMethod<CurlNetworkManager> dm_tick;
  CURLM *mp_multi_handle;
  int m_still_running;
  std::map<CURL *, int> m_handle_refcount;
  std::map<CURL *, CurlConnection *> m_handle2connection;
  CurlCredentialManager m_credentials;
  CurlCredentialManager m_proxy_credentials;
  static CurlNetworkManager *ms_instance;
};

// ---------------------------------------------------------------
//  InputHttpStream implementation

InputHttpStream::InputHttpStream (const std::string &url)
{
  mp_data = new InputHttpStreamPrivateData (this, url);
  mp_callback = 0;
}

InputHttpStream::~InputHttpStream ()
{
  delete mp_data;
  mp_data = 0;
}

void
InputHttpStream::set_credential_provider (HttpCredentialProvider *cp)
{
  CurlNetworkManager::instance ()->credentials ().set_provider (cp);
  CurlNetworkManager::instance ()->proxy_credentials ().set_provider (cp);
}

void
InputHttpStream::send ()
{
  mp_data->send ();
}

void
InputHttpStream::close ()
{
  mp_data->close ();
}

void
InputHttpStream::set_request (const char *r)
{
  mp_data->set_request (r);
}

void
InputHttpStream::set_data (const char *data)
{
  mp_data->set_data (data);
}

void
InputHttpStream::set_data (const char *data, size_t n)
{
  mp_data->set_data (data, n);
}

void
InputHttpStream::add_header (const std::string &name, const std::string &value)
{
  mp_data->add_header (name, value);
}

tl::Event &
InputHttpStream::ready ()
{
  return mp_data->ready ();
}

bool
InputHttpStream::data_available ()
{
  return mp_data->data_available ();
}

size_t
InputHttpStream::read (char *b, size_t n)
{
  return mp_data->read (b, n);
}

void
InputHttpStream::reset ()
{
  mp_data->reset ();
}

std::string
InputHttpStream::source () const
{
  return mp_data->source ();
}

std::string
InputHttpStream::absolute_path () const
{
  return mp_data->absolute_path ();
}

std::string
InputHttpStream::filename () const
{
  return mp_data->filename ();
}

bool
InputHttpStream::is_available ()
{
  return true;
}

void
InputHttpStream::tick ()
{
  if (mp_callback) {
    mp_callback->wait_for_input ();
  }
  CurlNetworkManager::instance ()->tick ();
}

void
InputHttpStream::set_timeout (double to)
{
  mp_data->set_timeout (to);
}

double
InputHttpStream::timeout () const
{
  return mp_data->timeout ();
}


// ----------------------------------------------------------------------
//  CurlConnection implementation

size_t read_func (char *buffer, size_t size, size_t nitems, void *userdata);
size_t seek_func (void *userdata, curl_off_t offset, int origin);
size_t write_func (char *ptr, size_t size, size_t nmemb, void *userdata);
size_t write_header_func (char *ptr, size_t size, size_t nmemb, void *userdata);

CurlConnection::CurlConnection (CURL *handle)
  : mp_handle (handle)
{
#if defined(DEBUG_CURL)
  std::cerr << "CurlConnection(" << (void *)handle << ")" << std::endl;
#endif
  init ();
}

CurlConnection::CurlConnection (const CurlConnection &other)
  : mp_handle (other.mp_handle)
{
#if defined(DEBUG_CURL)
  std::cerr << "CurlConnection(" << (void *)mp_handle << ")" << std::endl;
#endif
  init ();
}

void CurlConnection::close ()
{
  CurlNetworkManager::instance ()->release_connection (this);
  curl_slist_free_all (mp_headers);

  mp_handle = 0;
  m_http_status = 0;
  m_finished = false;
  m_status = 0;
  mp_headers = 0;
  m_authenticated = 0;
}

void CurlConnection::init ()
{
  m_http_status = 0;
  m_finished = false;
  m_status = 0;
  mp_headers = 0;
  m_authenticated = 0;

  CurlNetworkManager::instance ()->add_connection (this);
}

CurlConnection::~CurlConnection ()
{
#if defined(DEBUG_CURL)
  std::cerr << "~CurlConnection(" << (void *)mp_handle << ")" << std::endl;
#endif
  if (mp_handle) {
    CurlNetworkManager::instance ()->release_connection (this);
    curl_slist_free_all (mp_headers);
  }
}

void CurlConnection::set_url (const char *url)
{
  m_url = url;
}

void CurlConnection::set_request (const char *request)
{
  m_request = request;
}

void CurlConnection::add_header (const char *header, const char *value)
{
#if defined(DEBUG_CURL)
  std::cerr << "CurlConnection::set_header(" << header << ", " << value << ")" << std::endl;
#endif
  if (! value) {
    mp_headers = curl_slist_append (mp_headers, (std::string (header) + ";").c_str ());
  } else {
    mp_headers = curl_slist_append (mp_headers, (std::string (header) + ": " + std::string (value)).c_str ());
  }
}

void CurlConnection::set_data (const char *data)
{
#if defined(DEBUG_CURL)
  std::cerr << "CurlConnection::set_data(...)" << std::endl;
#endif
  m_data.push (data, strlen (data));
}

void CurlConnection::set_data (const char *data, size_t n)
{
#if defined(DEBUG_CURL)
  std::cerr << "CurlConnection::set_data(" << n << " bytes)" << std::endl;
#endif
  m_data.push (data, n);
}

size_t CurlConnection::read_available () const
{
  return m_read_data.size ();
}

std::string CurlConnection::read_data_to_string () const
{
  return m_read_data.to_string ();
}

size_t CurlConnection::fetch_read_data (char *buffer, size_t nbytes)
{
  return m_read_data.fetch (buffer, nbytes);
}

size_t CurlConnection::fetch_data (char *buffer, size_t nbytes)
{
  return m_data.fetch (buffer, nbytes);
}

int CurlConnection::seek (curl_off_t offset, int origin)
{
  if (origin == SEEK_CUR) {
    m_data.seek (size_t (curl_off_t (m_data.pos ()) + offset));
  } else if (origin == SEEK_END) {
    m_data.seek (size_t (curl_off_t (m_data.size ()) + offset));
  } else {
    m_data.seek (size_t (offset));
  }
  return CURL_SEEKFUNC_OK;
}

void CurlConnection::send ()
{
  tl_assert (mp_handle != 0);

  m_http_status = 0;
  m_status = 0;
  m_finished = false;
  m_read_data.clear ();
  m_header_data.clear ();

  if (tl::verbosity() >= 30) {
    tl::info << "HTTP request URL: " << m_url;
    if (tl::verbosity() >= 40) {
      curl_slist *hl = mp_headers;
      tl::info << "HTTP request header: ";
      while (hl) {
        tl::info << "   " << hl->data;
        hl = hl->next;
      }
      tl::info << "HTTP request data: " << m_data.to_string ();
    }
  }

  curl_easy_setopt (mp_handle, CURLOPT_URL, m_url.c_str ());
  if (! m_request.empty ()) {
    curl_easy_setopt (mp_handle, CURLOPT_CUSTOMREQUEST, m_request.c_str ());
  }

#if defined(DEBUG_CURL)
  curl_easy_setopt (mp_handle, CURLOPT_VERBOSE, 1L);
#endif

  curl_easy_setopt (mp_handle, CURLOPT_ERRORBUFFER, &m_error_msg);

  curl_easy_setopt (mp_handle, CURLOPT_READFUNCTION, &read_func);
  curl_easy_setopt (mp_handle, CURLOPT_READDATA, (void *) this);
  curl_easy_setopt (mp_handle, CURLOPT_SEEKFUNCTION, &seek_func);
  curl_easy_setopt (mp_handle, CURLOPT_SEEKDATA, (void *) this);
  curl_easy_setopt (mp_handle, CURLOPT_WRITEFUNCTION, &write_func);
  curl_easy_setopt (mp_handle, CURLOPT_WRITEDATA, (void *) this);
  curl_easy_setopt (mp_handle, CURLOPT_HEADERFUNCTION, &write_header_func);
  curl_easy_setopt (mp_handle, CURLOPT_HEADERDATA, (void *) this);

  if (! m_data.empty ()) {
    curl_easy_setopt (mp_handle, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt (mp_handle, CURLOPT_INFILESIZE, (long) m_data.size ());
  } else {
    curl_easy_setopt (mp_handle, CURLOPT_UPLOAD, 0L);
  }

  curl_easy_setopt (mp_handle, CURLOPT_HTTPHEADER, mp_headers);

  if (m_authenticated > 0) {
    curl_easy_setopt (mp_handle, CURLOPT_PASSWORD, m_password.c_str ());
    curl_easy_setopt (mp_handle, CURLOPT_USERNAME, m_user.c_str ());
  }

  //  always resolve redirects
  curl_easy_setopt (mp_handle, CURLOPT_FOLLOWLOCATION, 1L);

  CurlNetworkManager::instance ()->start (this);
}

void CurlConnection::add_read_data (const char *data, size_t n)
{
#if defined(DEBUG_CURL)
  std::cerr << "CurlConnection::add_read_data(" << n << " bytes)" << std::endl;
#endif
  m_read_data.push (data, n);
  data_available_event ();
}

void CurlConnection::add_header_data (const char *data, size_t n)
{
#if defined(DEBUG_CURL)
  std::cerr << "CurlConnection::add_header_data(" << n << " bytes)" << std::endl;
#endif
  m_header_data.push (data, n);
}

void CurlConnection::check () const
{
  if (m_status < 0) {

    throw tl::CancelException ();

  } else if (m_status != 0) {

    throw tl::HttpErrorException (tl::sprintf (tl::to_string (tr ("Connection error (%s)")), m_error_msg), m_status, m_url);

  } else if (m_http_status < 200 || m_http_status >= 300) {

    //  translate some known errors
    const char *error_text = 0;
    if (m_http_status == 400) {
      error_text = "Bad Request";
    } else if (m_http_status == 401) {
      error_text = "Unauthorized";
    } else if (m_http_status == 403) {
      error_text = "Forbidden";
    } else if (m_http_status == 404) {
      error_text = "Not Found";
    } else if (m_http_status == 405) {
      error_text = "Method Not Allowed";
    } else if (m_http_status == 406) {
      error_text = "Not Acceptable";
    } else if (m_http_status == 407) {
      error_text = "Proxy Authentication Required";
    } else if (m_http_status == 408) {
      error_text = "Request Timeout";
    }
    if (error_text) {
      throw tl::HttpErrorException (error_text, m_http_status, m_url);
    } else {
      throw tl::HttpErrorException (tl::to_string (tr ("HTTP error")), m_http_status, m_url);
    }

  }
}

void CurlConnection::finished (int status)
{
  tl_assert (mp_handle != 0);

  if (status != 0) {
    m_status = status;
    m_finished = true;
    finished_event ();
    return;
  }

  long http_code = -1;
  curl_easy_getinfo (mp_handle, CURLINFO_RESPONSE_CODE, &http_code);

  if (tl::verbosity() >= 30) {
    tl::info << "HTTP response code: " << http_code;
    if (tl::verbosity() >= 40) {
      tl::info << "HTTP response header: " << m_header_data.to_string ();
    }
  }

  if ((http_code == 401 || http_code == 407)) {

    bool proxy_auth = (http_code == 407);

#if defined(DEBUG_CURL)
    std::cerr << "CurlConnection::authentication required" << std::endl;
#endif

    //  Authentication required: analyse header for realm information
    std::string realm = parse_realm (m_header_data.to_string ());

    const std::pair<std::string, std::string> *pwd;
    try {

      //  Note: on the second attempt use ForceInquire, so we don't reuse the wrong credentials
      CurlCredentialManager::Mode mode = m_authenticated == 0 ? CurlCredentialManager::Inquire : CurlCredentialManager::ForceInquire;

      if (proxy_auth) {
        pwd = CurlNetworkManager::instance ()->proxy_credentials ().user_password (m_url, realm, m_authenticated + 1, mode);
      } else {
        pwd = CurlNetworkManager::instance ()->credentials ().user_password (m_url, realm, m_authenticated + 1, mode);
      }

    } catch (tl::CancelException &) {

      m_finished = true;
      m_status = -1;  // cancelled
      finished_event ();
      return;

    }

    if (pwd) {

      //  restart - this time with authentication

      m_user = pwd->first;
      m_password = pwd->second;
      m_authenticated += 1;

#if defined(DEBUG_CURL)
      std::cerr << "CurlConnection::finished: resend with authentication data" << std::endl;
#endif

      curl_easy_reset (mp_handle);

      send ();
      return;

    }

  }

  m_http_status = http_code;
  m_finished = true;
  finished_event ();
}

size_t read_func (char *buffer, size_t size, size_t nitems, void *userdata)
{
  CurlConnection *connection = (CurlConnection *) userdata;
  return connection->fetch_data (buffer, size * nitems);
}

size_t seek_func (void *userdata, curl_off_t offset, int origin)
{
  CurlConnection *connection = (CurlConnection *) userdata;
  return connection->seek (offset, origin);
}

size_t write_func (char *buffer, size_t size, size_t nitems, void *userdata)
{
  CurlConnection *connection = (CurlConnection *) userdata;
  connection->add_read_data (buffer, size * nitems);
  return size * nitems;
}

size_t write_header_func (char *buffer, size_t size, size_t nitems, void *userdata)
{
  CurlConnection *connection = (CurlConnection *) userdata;
  connection->add_header_data (buffer, size * nitems);
  return size * nitems;
}


// ----------------------------------------------------------------------
//  CurlNetworkManager implementation

CurlNetworkManager *CurlNetworkManager::ms_instance = 0;

CurlNetworkManager::CurlNetworkManager ()
  : dm_tick (this, &CurlNetworkManager::on_tick),
    m_still_running (0), m_credentials (false), m_proxy_credentials (true)
{
#if defined(DEBUG_CURL)
  std::cerr << "CurlNetworkManager()" << std::endl;
#endif
  tl_assert (ms_instance == 0);
  mp_multi_handle = curl_multi_init ();
  ms_instance = this;

  //  will register the object in the static object repo - it's cleaned up properly
  //  when the application terminates.
  tl::StaticObjects::reg (&ms_instance);
}

CurlNetworkManager::~CurlNetworkManager ()
{
#if defined(DEBUG_CURL)
  std::cerr << "~CurlNetworkManager()" << std::endl;
#endif
  if (ms_instance == this) {
    ms_instance = 0;
  }
  curl_multi_cleanup (mp_multi_handle);
}

CurlNetworkManager *CurlNetworkManager::instance ()
{
  if (! ms_instance) {
    new CurlNetworkManager ();
  }
  return ms_instance;
}

CurlConnection *CurlNetworkManager::create_connection ()
{
  CURL *handle = curl_easy_init();
  return new CurlConnection (handle);
}

void CurlNetworkManager::start (CurlConnection *connection)
{
#if defined(DEBUG_CURL)
  std::cerr << "CurlNetworkManager::start(" << (void*)connection->mp_handle << ")" << std::endl;
#endif
  curl_multi_add_handle (mp_multi_handle, connection->mp_handle);
  curl_multi_perform(mp_multi_handle, &m_still_running);
  m_handle2connection[connection->mp_handle] = connection;

  dm_tick ();
}

void CurlNetworkManager::add_connection (CurlConnection *connection)
{
  ++m_handle_refcount[connection->mp_handle];
}

void CurlNetworkManager::release_connection (CurlConnection *connection)
{
  --m_handle_refcount[connection->mp_handle];
  if (m_handle_refcount[connection->mp_handle] == 0) {

#if defined(DEBUG_CURL)
    std::cerr << "CurlNetworkManager::release_connection(" << (void*)connection->mp_handle << ")" << std::endl;
#endif
    curl_easy_cleanup(connection->mp_handle);
    m_handle_refcount.erase (connection->mp_handle);

    std::map<CURL *, CurlConnection *>::iterator h = m_handle2connection.find (connection->mp_handle);
    if (h != m_handle2connection.end ()) {
      m_handle2connection.erase (h);
    }

  }
}

void CurlNetworkManager::on_tick ()
{
  tick ();
  if (! has_reply ()) {
    //  NOTE: don't reschedule if there is no DM scheduler. This will cause deep
    //  recursion.
    if (tl::DeferredMethodScheduler::instance ()) {
      dm_tick ();
    }
  }
}

bool CurlNetworkManager::has_reply () const
{
  return m_still_running <= 0;
}

void CurlNetworkManager::tick ()
{
#if defined(DEBUG_CURL)
  std::cerr << "CurlNetworkManager::tick()" << std::endl;
#endif
  if (m_still_running <= 0) {
    return;
  }

  struct timeval timeout;
  int rc;                   // select() return code
  CURLMcode mc;             // curl_multi_fdset() return code

  fd_set fdread;
  fd_set fdwrite;
  fd_set fdexcep;
  int maxfd = -1;

  long curl_timeo = -1;

  FD_ZERO (&fdread);
  FD_ZERO (&fdwrite);
  FD_ZERO (&fdexcep);

  // set a suitable timeout to play around with
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  curl_multi_timeout (mp_multi_handle, &curl_timeo);
  if (curl_timeo >= 0) {
    timeout.tv_sec = curl_timeo / 1000;
    if (timeout.tv_sec > 1) {
      timeout.tv_sec = 1;
    } else {
      timeout.tv_usec = (curl_timeo % 1000) * 1000;
    }
  }

  // get file descriptors from the transfers
  mc = curl_multi_fdset (mp_multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

  if (mc != CURLM_OK) {
    throw tl::HttpErrorException (tl::to_string (tr ("Connection error (curl_multi_fdset() failed)")), mc, std::string ());
  }

  /* On success the value of maxfd is guaranteed to be >= -1. We call
     select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
     no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
     to sleep 100ms, which is the minimum suggested value in the
     curl_multi_fdset() doc. */
  if (maxfd == -1) {
#ifdef _WIN32
    Sleep (100);
    rc = 0;
#else
    // Portable sleep for platforms other than Windows.
    struct timeval wait = { 0, 10 * 1000 }; /* 10ms */
    rc = select (0, NULL, NULL, NULL, &wait);
#endif
  }
  else {
    /* Note that on some platforms 'timeout' may be modified by select().
       If you need access to the original value save a copy beforehand. */
    rc = select (maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
  }

  int sr = m_still_running;

  switch (rc) {
  case -1:
    // select error
    break;
  case 0:   // timeout
  default:  // action
    curl_multi_perform (mp_multi_handle, &m_still_running);
    break;
  }

  if (m_still_running < sr) {

    //  less running connections: analyse and finish
    CURLMsg *msg;
    int msgs_left;

    while ((msg = curl_multi_info_read (mp_multi_handle, &msgs_left))) {

      if (msg->msg != CURLMSG_DONE) {
        continue;
      }

      std::map<CURL *, CurlConnection *>::iterator h = m_handle2connection.find (msg->easy_handle);
      if (h != m_handle2connection.end ()) {
        curl_multi_remove_handle (mp_multi_handle, msg->easy_handle);
        h->second->finished (msg->data.result);
      }

    }

  }
}

// ---------------------------------------------------------------
//  InputHttpStreamPrivateData implementation

InputHttpStreamPrivateData::InputHttpStreamPrivateData (InputHttpStream *stream, const std::string &url)
  : m_timeout (10.0), mp_stream (stream)
{
  m_sent = false;
  m_ready = false;

  m_connection.reset (CurlNetworkManager::instance ()->create_connection ());
  m_connection->set_url (url.c_str ());
  m_connection->data_available_event.add (this, &InputHttpStreamPrivateData::on_data_available);
  m_connection->finished_event.add (this, &InputHttpStreamPrivateData::on_finished);
}

InputHttpStreamPrivateData::~InputHttpStreamPrivateData ()
{
  // .. nothing yet ..
}

void
InputHttpStreamPrivateData::set_timeout (double to)
{
  m_timeout = to;
}

double
InputHttpStreamPrivateData::timeout () const
{
  return m_timeout;
}

bool
InputHttpStreamPrivateData::data_available ()
{
  return m_connection->read_available () > 0;
}

void
InputHttpStreamPrivateData::set_request (const char *r)
{
  m_connection->set_request (r);
}

void
InputHttpStreamPrivateData::set_data (const char *data)
{
  m_connection->set_data (data);
}

void
InputHttpStreamPrivateData::set_data (const char *data, size_t n)
{
  m_connection->set_data (data, n);
}

void
InputHttpStreamPrivateData::add_header (const std::string &name, const std::string &value)
{
  m_connection->add_header (name.c_str (), value.c_str ());
}

void
InputHttpStreamPrivateData::on_finished ()
{
  m_progress.reset (0);
  m_ready_event ();
}

void
InputHttpStreamPrivateData::on_data_available ()
{
  //  send the ready event just once
  if (! m_ready) {
    m_data_ready_event ();
    m_ready = true;
  }
}

void
InputHttpStreamPrivateData::send ()
{
  m_ready = false;
  m_progress.reset (0);
  m_connection->send ();
  m_sent = true;
}

void
InputHttpStreamPrivateData::check ()
{
  if (m_connection->finished ()) {
    m_connection->check ();
  }
}

size_t
InputHttpStreamPrivateData::read (char *b, size_t n)
{
  if (! m_sent) {
    send ();
  }

  //  block until enough data is available
  {
    //  Prevents deferred methods to be executed during the processEvents below (undesired side effects)
    tl::NoDeferredMethods silent;

    if (! m_progress.get ()) {
      m_progress.reset (new tl::AbsoluteProgress (tl::to_string (tr ("Downloading")) + " " + m_connection->url (), 1));
    }

    tl::Clock start_time = tl::Clock::current ();
    while (n > m_connection->read_available () && ! m_connection->finished () && (m_timeout <= 0.0 || (tl::Clock::current() - start_time).seconds () < m_timeout) && ! tl::CurlNetworkManager::instance ()->has_reply ()) {
      mp_stream->tick ();
      if (m_progress.get ()) {  //  might have been reset by tick()
        ++*m_progress;
      }
    }
  }

  if (m_connection->finished ()) {
    m_connection->check ();
  } else if (tl::verbosity() >= 40) {
    tl::info << "HTTP response data read: " << m_connection->read_data_to_string ();
  }

  return m_connection->fetch_read_data (b, n);
}

void
InputHttpStreamPrivateData::close ()
{
  m_progress.reset (0);
  if (m_connection.get ()) {
    m_connection->close ();
  }
  m_sent = m_ready = false;
}

void
InputHttpStreamPrivateData::reset ()
{
  throw tl::Exception (tl::to_string (tr ("'reset' is not supported on HTTP input streams")));
}

std::string
InputHttpStreamPrivateData::filename () const
{
  return tl::filename (tl::URI (m_connection->url ()).path ());
}

std::string
InputHttpStreamPrivateData::source () const
{
  return m_connection->url ();
}

std::string
InputHttpStreamPrivateData::absolute_path () const
{
  return m_connection->url ();
}

}
