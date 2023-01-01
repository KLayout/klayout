
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


#include "gsiDecl.h"

#include <cctype>

namespace gsi
{

// --------------------------------------------------------------------------------
//  Implementation of MethodBase

MethodBase::MethodBase (const std::string &name, const std::string &doc, bool c, bool s)
  : m_doc (doc), m_const (c), m_static (s), m_protected (false), m_argsize (0)
{ 
  reset_called ();
  parse_name (name);
}

MethodBase::MethodBase (const std::string &name, const std::string &doc)
  : m_doc (doc), m_const (false), m_static (false), m_protected (false), m_argsize (0)
{ 
  reset_called ();
  parse_name (name);
}

bool MethodBase::compatible_with_num_args (unsigned int nargs) const
{
  if (nargs > (unsigned int) (std::distance (begin_arguments (), end_arguments ()))) {
    return false;
  } else {
    for (MethodBase::argument_iterator a = begin_arguments (); a != end_arguments () && (! a->spec () || ! a->spec ()->has_default ()); ++a) {
      if (nargs == 0) {
        return false;
      }
      --nargs;
    }
    return true;
  }
}

void MethodBase::check_no_args () const
{
  if (end_arguments () != begin_arguments ()) {
    throw NoArgumentsAllowedException ();
  }
}

void MethodBase::check_num_args (unsigned int num) const
{
  if (! compatible_with_num_args (num)) {
    throw NeedsArgumentsException (num, std::distance (begin_arguments (), end_arguments ()));
  }
}

void MethodBase::check_return_type (const ArgType &a) const
{
  if (m_ret_type != a) {
    throw IncompatibleReturnTypeException (a, m_ret_type);
  }
}

void MethodBase::parse_name (const std::string &name)
{
  const char *n = name.c_str ();

  if (*n == '*' && n[1] && n[1] != '*' && n[1] != '!' && n[1] != '=') {
    m_protected = true;
    ++n;
  }

  while (*n) {

    m_method_synonyms.push_back (MethodSynonym ());

    if ((*n == '#' || *n == ':') && n[1]) {
      if (*n == '#') {
        m_method_synonyms.back ().deprecated = true;
      } else {
        m_method_synonyms.back ().is_getter = true;
      }
      ++n;
    }

    bool any = false;
    while (*n && (*n != '|' || !any)) {
      if (*n == '\\' && n[1]) {
        ++n;
      }
      m_method_synonyms.back ().name += *n;
      any = true;
      if (isalnum (*n) || *n == '_') {
        ++n;
        if (*n == '?' && (n[1] == '|' || !n[1])) {
          ++n;
          m_method_synonyms.back ().is_predicate = true;
        } else if (*n == '=' && (n[1] == '|' || !n[1])) {
          ++n;
          m_method_synonyms.back ().is_setter = true;
        }
      } else {
        ++n;
      }
    }

    if (*n == '|') {
      ++n;
    }

  }
}

std::string
MethodBase::to_string () const
{
  std::string res;

  if (is_static ()) {
    res += "static ";
  }

  res += ret_type ().to_string ();
  res += " ";

  if (m_method_synonyms.size () == 1) {
    res += names ();
  } else {
    res += "{" + names() + "}";
  }

  res += "(";
  for (argument_iterator a = begin_arguments (); a != end_arguments (); ++a) {
    if (a != begin_arguments ()) {
      res += ", ";
    }
    res += a->to_string ();
  }

  res += ")";

  if (is_const ()) {
    res += " const";
  }

  return res;
}

std::string
MethodBase::names () const
{
  std::string res;

  for (synonym_iterator s = m_method_synonyms.begin (); s != m_method_synonyms.end (); ++s) {
    if (s != m_method_synonyms.begin ()) {
      res += "|";
    }
    res += s->name;
    if (s->is_setter) {
      res += "=";
    } else if (s->is_predicate) {
      res += "?";
    }
  }

  return res;
}

std::string
MethodBase::combined_name () const
{
  std::string res;
  if (m_protected) {
    res += "*";
  }

  for (synonym_iterator s = m_method_synonyms.begin (); s != m_method_synonyms.end (); ++s) {
    if (s != m_method_synonyms.begin ()) {
      res += "|";
    }
    if (s->is_getter) {
      res += ":";
    }
    if (s->deprecated) {
      res += "#";
    }
    for (const char *n = s->name.c_str (); *n; ++n) {
      if (*n == '*' || *n == '#' || *n == '\\' || *n == '|' || *n == ':' || *n == '=' || *n == '?') {
        res += "\\";
      }
      res += *n;
    }
    if (s->is_setter) {
      res += "=";
    } else if (s->is_predicate) {
      res += "?";
    }
  }

  return res;
}

const std::string &
MethodBase::primary_name () const
{
  if (m_method_synonyms.empty ()) {
    static const std::string empty_name;
    return empty_name;
  } else {
    return m_method_synonyms.front ().name;
  }
}

}

