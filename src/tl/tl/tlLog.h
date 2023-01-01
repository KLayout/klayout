
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


#ifndef HDR_tlLog
#define HDR_tlLog

#include "tlCommon.h"

#include "tlString.h"
#include "tlThreads.h"
#include "tlObjectCollection.h"

namespace tl
{

/**
 *  @brief Set the verbosity level
 *
 *  Predefined levels are:
 *    0: none
 *    10: basic
 *    11: basic timing
 *    20: detailed 
 *    21: detailed timing
 *    30: verbose
 *    31: verbose timing
 *    100+: very verbose 
 */
TL_PUBLIC void verbosity (int level);

/**
 *  @brief Get the verbosity level
 */
TL_PUBLIC int verbosity ();

/**
 *  @brief A "endl" tag class
 *
 *  This class is supposed to issue a end-of-line.
 */

struct TL_PUBLIC ChannelEndl 
{
  //  just a "tag"
};

extern TL_PUBLIC ChannelEndl endl;

/**
 *  @brief A "noendl" tag class
 *
 *  This class is supposed to suppress the implicit end-of-line.
 */

struct TL_PUBLIC ChannelNoendl 
{
  //  just a "tag"
};

extern TL_PUBLIC ChannelNoendl noendl;


class TL_PUBLIC ChannelProxy;

/**
 *  @brief A basic channel
 *
 *  Channels are supposed to be derived by subclasses providing 
 *  a special implementation for the channels.
 */

class TL_PUBLIC Channel
  : public tl::Object
{
public:
  /**
   *  @brief Construct a channel 
   */
  Channel ();

  /**
   *  @brief Destructor
   */
  virtual ~Channel ();

  /**
   *  @brief Output "something"
   *  
   *  A proxy object to the original channel is returned that does
   *  locking of the channel and reference counting such that the
   *  channel is freed again once it is no longer used.
   */
  template <class T> 
  ChannelProxy operator<< (const T &t);

  /**
   *  @brief Output a const char *
   *
   *  For the return object see the generic operator<<
   */
  ChannelProxy operator<< (const char *s);

  /**
   *  @brief A end-of-line output
   */
  ChannelProxy operator<< (ChannelEndl);
  
  /**
   *  @brief Suppress the implicit end of line at the end
   */
  ChannelProxy operator<< (ChannelNoendl);
  
protected:
  //  this is the interface implemented by the subclasses
  virtual void puts (const char *s) = 0;
  virtual void endl () = 0;
  virtual void end () = 0;
  virtual void begin () = 0;
  virtual void yield () = 0;

  tl::Mutex m_lock;

private:
  friend class ChannelProxy;
  friend class LogTee;

  ChannelProxy issue_proxy ();
  void release_proxy ();

  void noendl ()
  {
    m_no_endl = true;
  }

  bool m_no_endl;
  bool m_active;
  bool m_in_yield;
};

/**
 *  @brief A channel proxy
 *
 *  The proxy objects are used to control when the channel is to
 *  be released.
 */

class TL_PUBLIC ChannelProxy
{
public:
  /**
   *  @brief Construct a channel proxy to a channel
   */
  ChannelProxy (Channel *channel);

  /**
   *  @brief Destructor
   */
  ~ChannelProxy ();

  /**
   *  @brief Output "something"
   */
  template <class T> 
  ChannelProxy &operator<< (const T &t)
  {
    mp_channel->puts (tl::to_string (t).c_str ());
    return *this;
  }

  /**
   *  @brief Output a const char *
   */
  ChannelProxy &operator<< (const char *s)
  {
    mp_channel->puts (s);
    return *this;
  }

  /**
   *  @brief A end-of-line output
   */
  ChannelProxy &operator<< (ChannelEndl)
  {
    mp_channel->endl ();
    return *this;
  }
  
  /**
   *  @brief A end-of-line output
   */
  ChannelProxy &operator<< (ChannelNoendl)
  {
    mp_channel->noendl ();
    return *this;
  }
  
private:
  Channel *mp_channel; 

  //  copying only by the Channel class. This one knows what it does
  friend class Channel;
  ChannelProxy &operator= (const ChannelProxy &);
  ChannelProxy (const ChannelProxy &);
};

template <class T> 
inline ChannelProxy 
Channel::operator<< (const T &t)
{
  ChannelProxy p = issue_proxy ();
  puts (tl::to_string (t).c_str ());
  return p;
}

inline ChannelProxy 
Channel::operator<< (const char *s)
{
  ChannelProxy p = issue_proxy ();
  puts (s);
  return p;
}

inline ChannelProxy 
Channel::operator<< (ChannelEndl)
{
  ChannelProxy p = issue_proxy ();
  endl ();
  return p;
}

/**
 *  @brief A multi-cast log distribution object
 */

class TL_PUBLIC LogTee : public Channel
{
public:
  LogTee ();
  LogTee (Channel *first, bool owned);

  void add (Channel *other, bool owned);
  void prepend (Channel *other, bool owned);
  void clear ();

protected:
  virtual void puts (const char *s);
  virtual void endl ();
  virtual void end ();
  virtual void begin ();
  virtual void yield ();

private:
  tl::weak_collection<tl::Channel> m_channels;
  tl::shared_collection<tl::Channel> m_owned_channels;
};

/// The static instance of the log channel
/// The log channel is identical to the info channel but is silent depending on the verbosity and 
/// the output mode. It should be used for general notifications like the beginning of a operation.
extern TL_PUBLIC LogTee log;

/// The static instance of the info channel
extern TL_PUBLIC LogTee info;

/// The static instance of the warning channel
extern TL_PUBLIC LogTee warn;

/// The static instance of the error channel
extern TL_PUBLIC LogTee error;


} // namespace tl

#endif

