
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


#include "dbLoadLayoutOptions.h"
#include "dbStream.h"
#include "tlClassRegistry.h"
#include "tlStream.h"
#include "tlExpression.h"

namespace db
{
  LoadLayoutOptions::LoadLayoutOptions ()
    : m_warn_level (1)
  {
    // .. nothing yet ..
  }

  LoadLayoutOptions::LoadLayoutOptions (const LoadLayoutOptions &d)
  {
    operator= (d);
  }

  LoadLayoutOptions &
  LoadLayoutOptions::operator= (const LoadLayoutOptions &d)
  {
    if (&d != this) {

      m_warn_level = d.m_warn_level;

      release ();
      for (std::map <std::string, FormatSpecificReaderOptions *>::const_iterator o = d.m_options.begin (); o != d.m_options.end (); ++o) {
        m_options.insert (std::make_pair (o->first, o->second->clone ()));
      }

    }
    return *this;
  }

  LoadLayoutOptions::~LoadLayoutOptions ()
  {
    release (); 
  }

  void 
  LoadLayoutOptions::release ()
  {
    for (std::map <std::string, FormatSpecificReaderOptions *>::const_iterator o = m_options.begin (); o != m_options.end (); ++o) {
      delete o->second;
    }
    m_options.clear ();
  }

  void 
  LoadLayoutOptions::set_options (const FormatSpecificReaderOptions &options)
  {
    set_options (options.clone ());
  }

  void 
  LoadLayoutOptions::set_options (FormatSpecificReaderOptions *options)
  {
    std::map <std::string, FormatSpecificReaderOptions *>::iterator o = m_options.find (options->format_name ());
    if (o != m_options.end ()) {
      delete o->second;
      m_options.erase (o);
    }

    m_options.insert (std::make_pair (options->format_name (), options));
  }

  const FormatSpecificReaderOptions *
  LoadLayoutOptions::get_options (const std::string &format) const
  {
    std::map <std::string, FormatSpecificReaderOptions *>::const_iterator o = m_options.find (format);
    if (o != m_options.end ()) {
      return o->second;
    } else {
      return 0;
    }
  }

  FormatSpecificReaderOptions *
  LoadLayoutOptions::get_options (const std::string &format)
  {
    std::map <std::string, FormatSpecificReaderOptions *>::const_iterator o = m_options.find (format);
    if (o != m_options.end ()) {
      return o->second;
    } else {
      return 0;
    }
  }

  void
  LoadLayoutOptions::set_option_by_method (const std::string &method, const tl::Variant &value)
  {
    //  Utilizes the GSI binding to set the values
    tl::Variant ref = tl::Variant::make_variant_ref (this);

    tl::Extractor ex (method.c_str ());

    while (! ex.at_end ()) {

      std::string m;
      ex.read_word (m, "_=");
      if (! ex.at_end ()) {
        ex.expect (".");
      }

      tl::Variant out;

      std::vector<tl::Variant> args;
      if (ex.at_end ()) {
        args.push_back (value);
      }
      tl::ExpressionParserContext context;
      ref.user_cls ()->eval_cls ()->execute (context, out, ref, m, args);

      ref = out;

    }
  }

  tl::Variant
  LoadLayoutOptions::get_option_by_method (const std::string &method)
  {
    //  Utilizes the GSI binding to set the values
    tl::Variant ref = tl::Variant::make_variant_ref (this);

    tl::Extractor ex (method.c_str ());

    while (! ex.at_end ()) {

      std::string m;
      ex.read_word (m, "_=");
      if (! ex.at_end ()) {
        ex.expect (".");
      }

      tl::Variant out;

      std::vector<tl::Variant> args;
      tl::ExpressionParserContext context;
      ref.user_cls ()->eval_cls ()->execute (context, out, ref, m, args);

      ref = out;

    }

    return ref;
  }

  void
  LoadLayoutOptions::set_option_by_name (const std::string &method, const tl::Variant &value)
  {
    return set_option_by_method (method + "=", value);
  }

  tl::Variant
  LoadLayoutOptions::get_option_by_name (const std::string &method)
  {
    return get_option_by_method (method);
  }
}

