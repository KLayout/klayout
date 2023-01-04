
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


#ifndef _HDR_tlScriptError
#define _HDR_tlScriptError

#include "tlException.h"

#include <string>
#include <vector>

namespace tl
{

/**
 *  @brief A piece of backtrace information
 */
struct TL_PUBLIC BacktraceElement
{
  /**  
   *  @brief Constructor: create an element with a file and a line information
   */
  BacktraceElement(const std::string &_file, int _line);

  /**  
   *  @brief Constructor: create an element with a file, a line information and more information
   */
  BacktraceElement(const std::string &_file, int _line, const std::string _more_info);

  /**
   *  @brief Default constructor
   */
  BacktraceElement();

  /**
   *  @brief Convert the information to a string
   */
  std::string to_string() const;

  std::string file;
  int line;
  std::string more_info;

private:
  void translate_includes ();
};

/**
 *  @brief A basic exception class
 */
class TL_PUBLIC ScriptError
  : public tl::Exception 
{
public:
  ScriptError (const char *msg, const char *cls, const std::vector <BacktraceElement> &backtrace);

  ScriptError (const char *msg, const char *sourcefile, int line, const char *cls, const std::vector <BacktraceElement> &backtrace);

  ScriptError (const ScriptError &d);

  virtual ~ScriptError ()
  { }

  const std::string &sourcefile () const 
  { 
    return m_sourcefile; 
  }

  void set_sourcefile (const std::string &sourcefile)
  {
    m_sourcefile = sourcefile;
  }

  int line () const
  { 
    return m_line; 
  }

  void set_line (int line)
  {
    m_line = line;
  }

  const std::string &cls () const
  { 
    return m_cls; 
  }

  void set_cls (const std::string &cls)
  {
    m_cls = cls;
  }

  const std::string &context () const
  { 
    return m_context; 
  }

  void set_context (const std::string &context)
  {
    m_context = context;
  }

  const std::vector<BacktraceElement> &backtrace () const
  { 
    return m_backtrace; 
  }

  virtual std::string msg () const;

private:
  std::string m_sourcefile;
  int m_line;
  std::string m_cls;
  std::string m_context;
  std::vector<BacktraceElement> m_backtrace;

  void translate_includes ();
};

/**
 *  @brief An exception class indicating an exit  
 *
 *  This exception can be thrown by the C++ client code and is translated into the same exception 
 *  on the user side.
 */
class TL_PUBLIC ExitException
  : public tl::Exception
{
public:
  ExitException ()
    : tl::Exception ("exit"), m_status (1)
  {
    //  do not catch in debugger
    set_first_chance (false);
  }

  ExitException (int status)
    : tl::Exception ("exit"), m_status (status)
  {
    //  do not catch in debugger
    set_first_chance (false);
  }

  int status() const { return m_status; }

private:
  int m_status;
};

}

#endif

