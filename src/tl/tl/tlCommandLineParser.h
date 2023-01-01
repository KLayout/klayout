
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

#if !defined(HDR_tlCommandLineParser_h)
#define HDR_tlCommandLineParser_h

#include "tlCommon.h"
#include "tlLog.h"

namespace tl
{

class CommandLineOptions;

/**
 *  @brief A base class for argument getters
 *
 *  The argument getters are both declarations (delivering names and doc)
 *  as well as providers for the translation methods. Argument getters
 *  are attached to the CommandLineOptions object to form the
 *  command line declaration.
 */
class TL_PUBLIC ArgBase
{
public:
  /**
   *  @brief A parsed version of the option string
   */
  struct ParsedOption
  {
    /**
     *  @brief Constructor
     *  This constructor populates the optional flag and
     *  the long_option, short_option and name values.
     */
    ParsedOption (const std::string &option);

    bool optional, inverted, advanced, non_advanced, repeated;
    std::string long_option, short_option, name;
    std::string group;
  };

  /**
   *  @brief Constructor
   *
   *  @param option The option descriptor
   *  @param brief_doc A short documentation
   *  @param long_doc A long documentation (or empty to skip this)
   *
   *  The option descriptor is either
   *    "name"                    - A mandatory input parameter with name "name"
   *    "?name"                   - An optional input parameter with name "name"
   *    "-o"                      - A short option without a parameter (boolean)
   *    "!-o"                     - Sets the value to false if present
   *    "-o=value"                - A short option with a value named "value"
   *    "--long-option"           - A long option
   *    "--long-option=value"     - A long option with a value
   *    "-o|--long-option"        - A short/long option
   *    "-o|--long-option=value"  - A short/long option with a value
   *    "[group]..."              - List the option under this group (group = group title)
   *    "#..."                    - Advanced option - listed with --help-all only
   *    "/..."                    - Non-advanced option - listed with -h|--help only
   *    "*..."                    - Multiple occurrences allowed - values needs to be
   *                                an array and values are accumulated. Without *, the
   *                                value string is evaluated to a comma-separated list.
   *                                "*" means one occurrence at least unless combined with "?".
   */
  ArgBase (const std::string &option, const std::string &brief_doc, const std::string &long_doc);

  /**
   *  @brief Destructor
   */
  virtual ~ArgBase ();

  /**
   *  @brief The option descriptor (see constructor)
   */
  const ParsedOption &option () const
  {
    return m_option;
  }

  /**
   *  @brief The short documentation
   */
  const std::string &brief_doc () const
  {
    return m_brief_doc;
  }

  /**
   *  @brief The long documentation
   */
  const std::string &long_doc () const
  {
    return m_long_doc;
  }

  /**
   *  @brief Returns true, if the argument is an option
   */
  bool is_option () const;

  /**
   *  @brief Returns an option description string
   *  This string is typically "-x|--xlong".
   */
  std::string option_desc () const;

  /**
   *  @brief Gets a value from the extractor into the target of the argument
   */
  virtual void take_value (tl::Extractor & /*ex*/)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Marks an option to be present (for boolean options)
   */
  virtual void mark_present ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Inverts the presence of an option (for boolean options)
   */
  virtual void invert_present ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Clones this method
   */
  virtual ArgBase *clone () const
  {
    return new ArgBase (*this);
  }

  /**
   *  @brief Returns a value indicating whether the argument wants a value
   *  This flag indicates that a value shall be assigned. For boolean types
   *  no value needs to be supplied. For other types a value is required.
   */
  virtual bool wants_value () const
  {
    return false;
  }

  /**
   *  @brief Performs the action this argument is associated with
   *  This method is used to implement "active" arguments such as "-h" or "--version".
   */
  virtual void action (CommandLineOptions *) const
  {
    //  The default implementation does nothing
  }

private:
  ParsedOption m_option;
  std::string m_brief_doc;
  std::string m_long_doc;
};

/**
 *  @brief A helper for extracting values by type
 *
 *  @param repeated_hint Is true, if the value is supposed to be repeated
 *  @param enumerated Is true, if the value parser shall stop at a comma to allow for more entries in the next iteration
 */
template <class T>
inline void extract (tl::Extractor &ex, T &t, bool /*repeated_hint*/, bool enumerated = false)
{
  ex.read (t);
  if (! enumerated) {
    ex.expect_end ();
  }
}

/**
 *  @brief A specialization for the string type
 */
inline void extract (tl::Extractor &ex, std::string &t, bool /*repeated_hint*/, bool enumerated = false)
{
  if (*ex == '"' || *ex == '\'') {
    ex.read_quoted (t);
    if (! enumerated) {
      ex.expect_end ();
    }
  } else if (enumerated) {
    ex.read (t, ",");
  } else {
    t = ex.get ();
    //  TODO: there should be a tl::Extractor method either to
    //  read all remaining text or to move the pointer to the end.
    while (! ex.at_end ()) {
      ++ex;
    }
  }
}

/**
 *  @brief A specialization for a list of any type (vector)
 */
template <class T>
inline void extract (tl::Extractor &ex, std::vector<T> &t, bool repeated_hint, bool /*enumerated*/ = false)
{
  while (! ex.at_end ()) {
    t.push_back (T ());
    extract (ex, t.back (), false, ! repeated_hint);
    if (! repeated_hint && ! ex.test (",")) {
      ex.expect_end ();
    }
  }
}

/**
 *  @brief A helper to mark "presence"
 */
template <class T>
inline void mark_presence (T &)
{
  //  .. the default implementation does nothing ..
}

inline void mark_presence (bool &t)
{
  t = true;
}

/**
 *  @brief A helper to invert "presence"
 */
template <class T>
inline void invert_presence (T &)
{
  //  .. the default implementation does nothing ..
}

inline void invert_presence (bool &t)
{
  t = !t;
}

/**
 *  @brief A helper template to extract the actual type from (T) or (const T &)
 */
template <class T>
struct type_without_const_ref
{
  typedef T inner_type;
};

template <class T>
struct type_without_const_ref<const T &>
{
  typedef T inner_type;
};

/**
 *  @brief A helper template to determine types that don't need a value
 */
template <class T>
struct wants_value_traits
{
  bool operator() () const { return true; }
};

template <>
struct wants_value_traits<bool>
{
  bool operator() () const { return false; }
};

/**
 *  @brief A specialization of the ArgBase class for a setter with direct access to the value
 */
template <class T>
class arg_direct_setter
  : public ArgBase
{
public:
  arg_direct_setter (const std::string &option, T *value, const std::string &brief_doc, const std::string &long_doc)
    : ArgBase (option, brief_doc, long_doc), mp_value (value)
  {
    //  .. nothing yet ..
  }

  virtual void take_value (tl::Extractor &ex)
  {
    extract (ex, *mp_value, option ().repeated);
  }

  virtual void mark_present ()
  {
    mark_presence (*mp_value);
  }

  virtual void invert_present ()
  {
    invert_presence (*mp_value);
  }

  virtual ArgBase *clone () const
  {
    return new arg_direct_setter<T> (*this);
  }

  virtual bool wants_value () const
  {
    return wants_value_traits<T> () ();
  }

private:
  T *mp_value;
};

/**
 *  @brief A specialization of the ArgBase class for a setter that is a method of a class
 */
template <class C, class T>
class arg_method_setter
  : public ArgBase
{
public:
  typedef typename type_without_const_ref<T>::inner_type inner_type;

  arg_method_setter (const std::string &option, C *object, void (C::*setter)(T), const std::string &brief_doc, const std::string &long_doc)
    : ArgBase (option, brief_doc, long_doc), m_value (), mp_object (object), mp_setter (setter)
  {
    //  .. nothing yet ..
  }

  virtual void take_value (tl::Extractor &ex)
  {
    extract (ex, m_value, option ().repeated);
    (mp_object->*mp_setter) (m_value);
  }

  virtual void mark_present ()
  {
    mark_presence (m_value);
    (mp_object->*mp_setter) (m_value);
  }

  virtual void invert_present ()
  {
    invert_presence (m_value);
    (mp_object->*mp_setter) (m_value);
  }

  virtual ArgBase *clone () const
  {
    return new arg_method_setter<C, T> (*this);
  }

  virtual bool wants_value () const
  {
    return wants_value_traits<T> () ();
  }

private:
  inner_type m_value;
  C *mp_object;
  void (C::*mp_setter)(T);
};

/**
 *  @brief Polymorphic production methods for the argument getters
 */
template <class C, class T>
arg_method_setter<C, T> arg (const std::string &option, C *object, void (C::*setter)(T), const std::string &brief_doc, const std::string &long_doc = "")
{
  return arg_method_setter<C, T> (option, object, setter, brief_doc, long_doc);
}

template <class T>
arg_direct_setter<T> arg (const std::string &option, T *value, const std::string &brief_doc, const std::string &long_doc = "")
{
  return arg_direct_setter<T> (option, value, brief_doc, long_doc);
}

/**
 *  @brief The command line parser class
 *
 *  To establish a command line parser use code like this:
 *
 *  @code
 *  int
 *  main (int argc, char *argv [])
 *  {
 *    bool has_x = false;
 *    int int_value = 0;
 *
 *    CommandLineOptions cmd;
 *    cmd << arg("-x|--long-option", &has_x, "X Option", "This is a long documentation for X option")
 *        << arg("int_value", &int_value, "A mandatory integer value");
 *      ;
 *
 *    try {
 *      cmd.parse (argc, argv);
 *      ... use has_x, int_value ..
 *    } catch (tl::CancelException &ex) {
 *      return 1;
 *    } catch (std::exception &ex) {
 *      tl::error << ex.what ();
 *      return 1;
 *    } catch (tl::Exception &ex) {
 *      tl::error << ex.msg ();
 *      return 1;
 *    } catch (...) {
 *      tl::error << "ERROR: unspecific error";
 *    }
 *  }
 *  @endcode
 */
class TL_PUBLIC CommandLineOptions
{
public:
  /**
   *  @brief Constructor
   */
  CommandLineOptions ();

  /**
   *  @brief Destructor
   */
  ~CommandLineOptions ();

  /**
   *  @brief Adds the argument declaration & getter
   */
  CommandLineOptions &operator<< (const ArgBase &a);

  /**
   *  @brief Adds the brief documentation
   */
  void brief (const std::string &text)
  {
    m_brief = text;
  }

  /**
   *  @brief Parses the command line and executes the getters
   *
   *  This method will throw a tl::CancelException if the help function was used.
   *  It will throw other exceptions on syntax errors or if mandatory arguments are
   *  missing.
   */
  void parse (int argc, char *argv[]);

  /**
   *  @brief Produces the help text
   */
  void produce_help (const std::string &program_name, bool advanced);

  /**
   *  @brief Produces the version text
   */
  void produce_version ();

  /**
   *  @brief Produces the license text
   */
  void produce_license ();

  /**
   *  @brief Gets the program name
   *  This name is set by the "parse" method.
   */
  const std::string &program_name () const
  {
    return m_program_name;
  }

  /**
   *  @brief Sets the version text
   *
   *  This text is printed with the --version option.
   */
  static void set_version (const std::string &text)
  {
    m_version = text;
  }

  /**
   *  @brief Sets the license text
   *
   *  This text is printed with the --license option.
   */
  static void set_license (const std::string &text)
  {
    m_license = text;
  }

private:
  std::string m_brief;
  std::vector<ArgBase *> m_args;
  std::string m_program_name;
  static std::string m_version;
  static std::string m_license;
};

}

#endif
