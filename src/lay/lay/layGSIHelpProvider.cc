
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "layGSIHelpProvider.h"
#include "layHelpSource.h"
#include "tlClassRegistry.h"

#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "pya.h"

#include <QUrl> 
#include <QColor>
#include <QPalette>
#include <QApplication>
#include <QRegExp>

#include <cstdio>
#include <cctype>
#include <algorithm>

namespace lay
{

// --------------------------------------------------------------------------------------
//  Some utilities

static std::string to_encoded_class_name (const std::string &s)
{
  return tl::replaced (s, ":", "+");
}

static std::string from_encoded_class_name (const std::string &s)
{
  return tl::replaced (s, "+", ":");
}

static std::string module_doc_url (const std::string &m)
{
  return "/code/module_" + to_encoded_class_name (m) + ".xml";
}

static std::string class_doc_url (const std::string &c)
{
  return "/code/class_" + to_encoded_class_name (c) + ".xml";
}

static std::string class_doc_url (const std::string &c, const std::string &m)
{
  if (c.empty ()) {
    return "#m_" + m;
  } else {
    return "/code/class_" + to_encoded_class_name (c) + ".xml#m_" + m;
  }
}

std::string escape_xml_with_formatting (const std::string &s, bool &in_code)
{
  std::string r;
  r.reserve (s.size ());
  for (tl::Extractor sc (s.c_str ()); *sc; ) {
    if (*sc == '<') {
      r += "&lt;";
      ++sc;
    } else if (*sc == '>') {
      r += "&gt;";
      ++sc;
    } else if (*sc == '&') {
      r += "&amp;";
      ++sc;
    } else if (*sc == '@') {
      if (sc.test ("@<")) {

        //  HTML tag
        r += "<";
        while (*sc && *sc != '>') {
          r += *sc;
          ++sc;
        }
        if (*sc == '>') {
          r += ">";
          ++sc;
        }

      } else if (sc.test ("@li")) {
        r += "<li>";
      } else if (sc.test ("@/li")) {
        r += "</li>";
      } else if (sc.test ("@ul")) {
        r += "<ul>";
      } else if (sc.test ("@/ul")) {
        r += "</ul>";
      } else if (sc.test ("@b")) {
        r += "<b>";
      } else if (sc.test ("@/b")) {
        r += "</b>";
      } else if (sc.test ("@u")) {
        r += "<u>";
      } else if (sc.test ("@/u")) {
        r += "</u>";
      } else if (sc.test ("@tt")) {
        r += "<tt>";
      } else if (sc.test ("@/tt")) {
        r += "</tt>";
      } else if (sc.test ("@i")) {
        r += "<i>";
      } else if (sc.test ("@/i")) {
        r += "</i>";
      } else if (sc.test ("@pre") || sc.test ("@code")) {
        in_code = true;
        r += "<pre>";
      } else if (sc.test ("@/pre") || sc.test ("@/code")) {
        in_code = false;
        r += "</pre>";
      } else if (sc.test ("@@")) {
        r += "@";
      } else {
        r += "@";
        ++sc;
      }
    } else {
      r += *sc;
      ++sc;
    }
  }
  return r;
}

static std::string 
full_name (const gsi::MethodBase::MethodSynonym &syn)
{
  if (syn.is_predicate) {
    return syn.name + "?";
  } else if (syn.is_setter) {
    return syn.name + "=";
  } else if (syn.name == "*!") {
    return "*";
  } else {
    return syn.name;
  }
}

struct DocumentationParser 
{
  DocumentationParser (const gsi::MethodBase *method)
  {
    std::string doc = method->doc ();
    parse_doc (doc);
  }

  DocumentationParser (const gsi::ClassBase *cls)
  {
    parse_doc (cls->doc ());
  }

  void parse_doc (const std::string &formatted_doc) 
  {
    hidden = false;
    qt_class = false;

    tl::Extractor ex (formatted_doc.c_str ());
    while (*ex) {

      if (*ex == '@') {

        if (ex.test ("@hide")) {
          hidden = true;
        } else if (ex.test ("@qt")) {
          qt_class = true;
        } else if (ex.test ("@brief")) {

          ex.read (brief_doc, "\n");

        } else if (ex.test ("@alias")) {

          ex.read (alias, "\n");

        } else if (ex.test ("@return") || ex.test ("@returns")) {

          ex.read  (ret_val, "\n");

        } else if (ex.test ("@args")) {

          std::string a;
          ex.try_read (a, "\n");
          a = tl::trim (a);
          if (! a.empty ()) {
            args = tl::split (a, ",");
            for (std::vector<std::string>::iterator i = args.begin (); i != args.end (); ++i) {
              *i = tl::trim (*i);
            }
          }

        } else if (ex.test ("@param")) {

          std::string n;
          ex.try_read (n);
          params.push_back (std::make_pair (n, std::string ()));
          ex.read (params.back ().second, "\n");

        } else {
          doc += "@";
          ++ex;
        }

      } else {
        doc += *ex;
        ++ex;
      }

    }
  }

  std::string doc_html ()
  {
    std::string r;
    r = "<p>";

    size_t p = 0;
    size_t pe = std::string::npos;
    bool in_code = false;
    while ((pe = doc.find ("\n\n", p)) != std::string::npos) {
      r += escape_xml_with_formatting (std::string (doc, p, pe - p), in_code);
      p = pe;
      if (in_code) {
        while (p < doc.size () && doc [p] == '\n') {
          r += doc [p];
          ++p;
        }
      } else {
        while (p < doc.size () && doc [p] == '\n') {
          ++p;
        }
        if (p < doc.size ()) {
          r += "</p><p>";
        }
      }
    }

    if (p < doc.size ()) {
      r += escape_xml_with_formatting (std::string (doc, p), in_code);
    }

    r += "</p>";
    return r;
  }

  bool hidden, qt_class;
  std::string doc;
  std::string brief_doc;
  std::string alias;
  std::vector<std::string> args;
  std::string ret_val;
  std::vector<std::pair <std::string, std::string> > params;
};

//  A cache for the parsed class documentation

static std::map <const gsi::ClassBase *, DocumentationParser> s_cls_doc;

static DocumentationParser &cls_documentation (const gsi::ClassBase *cls)
{
  if (cls->doc ().empty () && cls->declaration ()) {
    cls = cls->declaration ();
  }

  std::map <const gsi::ClassBase *, DocumentationParser>::iterator cd = s_cls_doc.find (cls);
  if (cd != s_cls_doc.end ()) {
    return cd->second;
  } else {
    return s_cls_doc.insert (std::make_pair (cls, DocumentationParser (cls))).first->second;
  }
}

static std::string make_qualified_name (const gsi::ClassBase *cls)
{
  std::string qname;

  const gsi::ClassBase *p = cls;
  while (p) {

    DocumentationParser &doc = cls_documentation (p);

    std::string n = p->name ();
    if (p->declaration () == p && ! doc.alias.empty ()) {
      n = doc.alias;
    }

    if (qname.empty ()) {
      qname = n;
    } else {
      qname = n + "::" + qname;
    }

    p = p->parent ();

  }

  return qname;
}

static const gsi::ClassBase *
real_class (const gsi::ClassBase *cls)
{
  return cls->declaration () ? cls->declaration () : cls;
}

namespace {

class RecursiveClassIterator
{
public:
  typedef const gsi::ClassBase &reference;
  typedef const gsi::ClassBase *pointer;

  RecursiveClassIterator ()
  {
    if (gsi::ClassBase::begin_classes () != gsi::ClassBase::end_classes ()) {
      m_cls_iter_stack.push_back (std::make_pair (gsi::ClassBase::begin_classes (), gsi::ClassBase::end_classes ()));
    }
  }

  bool at_end () const
  {
    return m_cls_iter_stack.empty ();
  }

  RecursiveClassIterator &operator++ ()
  {
    if (operator* ().begin_child_classes () != operator* ().end_child_classes ()) {
      m_cls_iter_stack.push_back (std::make_pair (operator* ().begin_child_classes (), operator* ().end_child_classes ()));
    } else {
      while (! m_cls_iter_stack.empty () && ++m_cls_iter_stack.back ().first == m_cls_iter_stack.back ().second) {
        m_cls_iter_stack.pop_back ();
      }
    }

    return *this;
  }

  const gsi::ClassBase &operator* () const
  {
    return *m_cls_iter_stack.back ().first;
  }

  const gsi::ClassBase *operator-> () const
  {
    return m_cls_iter_stack.back ().first.operator-> ();
  }

private:
  std::list<std::pair<gsi::ClassBase::class_iterator, gsi::ClassBase::class_iterator> > m_cls_iter_stack;
};

}

static std::string
replace_references (const std::string &t, const gsi::ClassBase *cls_base)
{
  cls_base = real_class (cls_base);
  tl_assert (cls_base);

  //  build a name map for simple name replacement
  std::set<std::string> name_map;
  for (gsi::ClassBase::method_iterator m = cls_base->begin_methods (); m != cls_base->end_methods (); ++m) {
    for (gsi::MethodBase::synonym_iterator syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {
      name_map.insert (full_name (*syn));
    }
  }

  std::string r;

  size_t q = 0;
  size_t p;
  while ((p = t.find ("\\", q)) != std::string::npos) {

    r += std::string (t, q, p - q);

    size_t pp = ++p;
    while (p < t.size () && (t[p] == '_' || t[p] == ':' || isalnum (t [p]))) {
      ++p;
    }
    if (p < t.size () && (t[p] == '?' || t [p] == '=')) {
      ++p;
    }

    std::string id (t, pp, p - pp);

    std::string mid;

    if (p < t.size () && t[p] == '#') {
      pp = ++p;
      while (p < t.size () && (t[p] == '_' || isalnum (t [p]))) {
        ++p;
      }
      if (p < t.size () && (t[p] == '?' || t [p] == '=')) {
        ++p;
      }
      mid = std::string (t, pp, p - pp);
    }

    bool found = false;
    if (name_map.find (id) != name_map.end ()) {
      r += "<a href=\"" + escape_xml (class_doc_url (std::string (), id)) + "\">" + escape_xml (id) + "</a>";
      found = true;
    }

    for (RecursiveClassIterator c; ! c.at_end (); ++c) {
      if (c->qname () == id) {
        r += "<a href=\"";
        if (mid.empty ()) {
          r += escape_xml (class_doc_url (id));
        } else {
          r += escape_xml (class_doc_url (id, mid));
        }
        r += "\">";
        if (mid.empty ()) {
          r += escape_xml (id);
        } else if (id.empty ()) {
          r += escape_xml (mid);
        } else {
          r += escape_xml (id) + "#" + escape_xml (mid);
        }
        r += "</a>";
        found = true;
      }
    }

    if (! found) {
      r += escape_xml (id);
      if (! mid.empty ()) {
        r += "#";
        r += mid;
      }
    }

    q = p;

  }

  r += std::string (t, q);

  return r;
}

// --------------------------------------------------------------------------------------
//  Implementation

GSIHelpProvider::GSIHelpProvider ()
{
  //  .. nothing yet ..
}

std::string 
GSIHelpProvider::folder (lay::HelpSource * /*src*/) const
{
  return "code";
}

std::string 
GSIHelpProvider::title (lay::HelpSource * /*src*/) const
{
  return tl::to_string (QObject::tr ("API Reference"));
}

static 
void produce_toc (const gsi::ClassBase *cls, std::vector <std::string> &toc)
{
  DocumentationParser &doc = cls_documentation (cls);
  if (! doc.hidden) {
    toc.push_back (class_doc_url (make_qualified_name (cls)));
  }
  for (tl::weak_collection<gsi::ClassBase>::const_iterator cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
    produce_toc (cc.operator-> (), toc);
  }
}

void 
GSIHelpProvider::toc (lay::HelpSource * /*src*/, std::vector<std::string> &t)
{
  std::set<std::string> mod_names;
  for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {
    mod_names.insert (c->module ());
    produce_toc (c.operator-> (), t);
  }

  for (std::set<std::string>::const_iterator m = mod_names.begin (); m != mod_names.end (); ++m) {
    t.push_back (module_doc_url (*m));
  }
}

QDomDocument
GSIHelpProvider::get (lay::HelpSource *src, const std::string &path) const
{
  QUrl url = QUrl::fromEncoded (path.c_str ());
  QString url_path = url.path ();
  QRegExp class_doc_url (QString::fromUtf8 ("^/code/class_(.*)\\.xml$"));
  QRegExp module_index_url (QString::fromUtf8 ("^/code/module_(.*)\\.xml$"));

  std::string text;
  if (url_path == QString::fromUtf8 ("/code/index.xml")) {
    text = produce_class_index (src, 0);
  } else if (module_index_url.indexIn (url_path) == 0) {
    text = produce_class_index (src, from_encoded_class_name (tl::to_string (module_index_url.cap (1))).c_str ());
  } else if (class_doc_url.indexIn (url_path) == 0) {
    text = produce_class_doc (from_encoded_class_name (tl::to_string (class_doc_url.cap (1))));
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("Page not found: ")) + path);
  }

  QDomDocument doc;
  QString errorMsg;
  int errorLine = 0;

  if (! doc.setContent (QByteArray (text.c_str (), int (text.size ())), true, &errorMsg, &errorLine)) {

    //  fallback: provide the original text plus the error message
    std::string fallback_text = std::string ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
                                "<!DOCTYPE language SYSTEM \"klayout_doc.dtd\">\n" +
                                "<doc><p>\n" + 
                                "<b>XML Parser Error: </b>" + escape_xml (tl::to_string (errorMsg)) + ", in line " + tl::to_string (errorLine) + " of " + path + "\n" +
                                "</p><pre>\n" + 
                                escape_xml (text) + "\n" +
                                "</pre></doc>";

    doc.setContent (QByteArray (fallback_text.c_str (), int (fallback_text.size ())), true, &errorMsg, &errorLine);

  }
  
  return doc;
}

static
void collect_class_info (const gsi::ClassBase *cls, const std::string &module, std::vector <std::pair <std::string, std::pair<std::string, std::string> > > &class_names, std::vector <std::pair <std::string, std::pair<std::string, std::string> > > &qt_class_names)
{
  DocumentationParser &doc = cls_documentation (cls);
  std::string qname = make_qualified_name (cls);

  //  Only list the name if the class is not hidden, it's a top-level class or the path is an expanded one
  //  (the last criterion avoids generating classes such as A::B_C)
  if (! doc.hidden) {
    if (! doc.qt_class) {
      class_names.push_back (std::make_pair (qname, std::make_pair (module, doc.brief_doc)));
    } else {
      qt_class_names.push_back (std::make_pair (qname, std::make_pair (module, doc.brief_doc)));
    }
  }

  for (tl::weak_collection<gsi::ClassBase>::const_iterator cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
    collect_class_info (cc.operator-> (), module, class_names, qt_class_names);
  }
}

std::string
GSIHelpProvider::produce_class_index (lay::HelpSource *src, const char *module_name) const
{
  bool skip_qt_classes = src->get_option ("skip-qt-classes").to_bool ();
  std::ostringstream os;

  os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
     << "<!DOCTYPE language SYSTEM \"klayout_doc.dtd\">" << std::endl
     << std::endl;

  os << "<doc>";
  if (!module_name) {
    os << "<title>" << tl::to_string (QObject::tr ("Class Index")) << "</title>" << std::endl;
  } else {
    os << "<title>" << tl::to_string (QObject::tr ("Class Index for Module ")) << escape_xml (module_name) << "</title>" << std::endl;
    os << "<keyword name=\"" << escape_xml (module_name) << "\"/>" << std::endl;
  }

  typedef std::vector <std::pair <std::string, std::pair<std::string, std::string> > > class_index_t;

  class_index_t class_names;
  class_index_t qt_class_names;

  for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {
    if (! module_name || c->module () == module_name) {
      collect_class_info (c.operator-> (), c->module (), class_names, qt_class_names);
    }
  }

  if (skip_qt_classes) {
    qt_class_names.clear ();
  }

  if (! module_name) {

    for (class_index_t::const_iterator i = class_names.begin (); i != class_names.end (); ++i) {
      os << "<topic-ref href=\"" << escape_xml (class_doc_url (i->first)) << "\"/>" << std::endl;
    }
    for (class_index_t::const_iterator i = qt_class_names.begin (); i != qt_class_names.end (); ++i) {
      os << "<topic-ref href=\"" << escape_xml (class_doc_url (i->first)) << "\"/>" << std::endl;
    }

    std::set<std::string> mod_names;
    std::set<std::string> qt_mod_names;

    for (class_index_t::iterator i = class_names.begin (); i != class_names.end (); ++i) {
      mod_names.insert (i->second.first);
    }

    for (class_index_t::iterator i = qt_class_names.begin (); i != qt_class_names.end (); ++i) {
      qt_mod_names.insert (i->second.first);
    }

    for (std::set<std::string>::const_iterator m = mod_names.begin (); m != mod_names.end (); ++m) {
      os << "<topic-ref href=\"" << escape_xml (module_doc_url (*m)) << "\"/>" << std::endl;
    }
    for (std::set<std::string>::const_iterator m = qt_mod_names.begin (); m != qt_mod_names.end (); ++m) {
      os << "<topic-ref href=\"" << escape_xml (module_doc_url (*m)) << "\"/>" << std::endl;
    }

    os << "<p>" << tl::to_string (QObject::tr ("Per-Module documentation:")) << "</p>";

    os << "<ul>";
    for (std::set<std::string>::const_iterator m = mod_names.begin (); m != mod_names.end (); ++m) {
      os << "<li><a href=\"" << escape_xml (module_doc_url (*m)) << "\">" << tl::to_string (QObject::tr ("Core Module")) << " " << escape_xml (*m) << "</a></li>";
    }
    for (std::set<std::string>::const_iterator m = qt_mod_names.begin (); m != qt_mod_names.end (); ++m) {
      os << "<li><a href=\"" << escape_xml (module_doc_url (*m)) << "\">" << tl::to_string (QObject::tr ("Qt Module")) << " " << escape_xml (*m) << "</a></li>";
    }
    os << "</ul>";

  }

  if (! qt_class_names.empty ()) {
    os << "<p>" << tl::to_string (QObject::tr ("Find Qt class documentation")) << " <a href=\"#qtclasses\">" << tl::to_string (QObject::tr ("here")) << "</a></p>" << std::endl;
  }

  if (! class_names.empty ()) {
    
    std::sort (class_names.begin (), class_names.end ());

    os << "<h2>KLayout classes</h2>" << std::endl;
    os << "<table>" << std::endl;
    int n = 0;
    for (class_index_t::const_iterator cc = class_names.begin (); cc != class_names.end (); ++cc, ++n) {
      os << "<tr class=\"row" << (n % 2)  << "\">" << std::endl;
      os << "<td><a href=\"" << escape_xml (class_doc_url (cc->first)) << "\">" << escape_xml (cc->first) << "</a></td>";
      if (! module_name) {
        os << "<td><a href=\"" << module_doc_url (cc->second.first) << "\">" << escape_xml (cc->second.first) << "</a></td>";
      }
      os << "<td>" << escape_xml (cc->second.second) << "</td></tr>" << std::endl;
    }
    os << "</table>" << std::endl;

  }

  if (! qt_class_names.empty ()) {
    
    std::sort (qt_class_names.begin (), qt_class_names.end ());

    os << "<a name=\"qtclasses\"/><h2>Qt classes</h2>" << std::endl;
    os << "<table>" << std::endl;
    int n = 0;
    for (class_index_t::const_iterator cc = qt_class_names.begin (); cc != qt_class_names.end (); ++cc, ++n) {
      os << "<tr class=\"row" << (n % 2)  << "\">" << std::endl;
      os << "<td><a href=\"" << escape_xml (class_doc_url (cc->first)) << "\">" << escape_xml (cc->first) << "</a></td>";
      if (! module_name) {
        os << "<td><a href=\"" << module_doc_url (cc->second.first) << "\">" << escape_xml (cc->second.first) << "</a></td>";
      }
      os << "<td>" << escape_xml (cc->second.second) << "</td></tr>" << std::endl;
    }
    os << "</table>" << std::endl;

  }

  os << "</doc>" << std::endl;

  return os.str ();

}

static std::string
type_to_s (const gsi::ArgType &a, bool linked, bool for_return)
{
  std::string s;
  switch (a.type ()) {
  case gsi::T_void_ptr:
    s += "void *"; break;
  case gsi::T_void:
    s += "void"; break;
  case gsi::T_bool:
    s += "bool"; break;
  case gsi::T_char:
    s += "char"; break;
  case gsi::T_schar:
    s += "signed char"; break;
  case gsi::T_uchar:
    s += "unsigned char"; break;
  case gsi::T_short:
    s += "short"; break;
  case gsi::T_ushort:
    s += "unsigned short"; break;
  case gsi::T_int:
    s += "int"; break;
#if defined(HAVE_64BIT_COORD)
  case gsi::T_int128:
    s += "int128"; break;
#endif
  case gsi::T_uint:
    s += "unsigned int"; break;
  case gsi::T_long:
    s += "long"; break;
  case gsi::T_ulong:
    s += "unsigned long"; break;
  case gsi::T_longlong:
    s += "long long"; break;
  case gsi::T_ulonglong:
    s += "unsigned long long"; break;
  case gsi::T_double:
    s += "double"; break;
  case gsi::T_float:
    s += "float"; break;
  case gsi::T_string:
    s += "string"; break;
  case gsi::T_byte_array:
    s += "bytes"; break;
  case gsi::T_var:
    s += "variant"; break;
  case gsi::T_object:
    if (a.is_cptr () || (! for_return && a.is_cref ())) {
      s = "const ";
    }
    if (a.pass_obj ()) {
      s += "new ";
    }
    if (linked) {
      s += "<a href=\"" + escape_xml (class_doc_url (make_qualified_name (a.cls ()))) + "\">" + escape_xml (make_qualified_name (a.cls ())) + "</a>";
    } else {
      s += make_qualified_name (a.cls ());
    }
    break;
  case gsi::T_vector:
    if (a.inner ()) {
      s += type_to_s (*a.inner (), linked, false);
    } 
    s += "[]";
    break;
  case gsi::T_map:
    s += "map&lt;";
    if (a.inner_k ()) {
      s += type_to_s (*a.inner_k (), linked, false);
    } 
    s += ",";
    if (a.inner ()) {
      s += type_to_s (*a.inner (), linked, false);
    } 
    s += "&gt;";
    break;
  }
  if (a.is_cptr () || a.is_ptr ()) {
    s += " ptr";
  }
  return s;
}

static std::string 
method_attributes (const gsi::MethodBase *method, DocumentationParser & /*doc*/, bool without_static = false, bool without_prot = true)
{
  std::string r;
  if (method->is_signal ()) {
    if (! r.empty ()) {
      r += ",";
    }
    r += "signal";
  }
  if (method->is_callback ()) {
    if (! r.empty ()) {
      r += ",";
    }
    r += "virtual";
  }
  if (! without_static && method->is_static ()) {
    if (! r.empty ()) {
      r += ",";
    }
    r += "static";
  }
  if (method->is_const ()) {
    if (! r.empty ()) {
      r += ",";
    }
    r += "const";
  }
  if (method->ret_type ().is_iter ()) {
    if (! r.empty ()) {
      r += ",";
    }
    r += "iter";
  }
  if (! without_prot && method->is_protected ()) {
    if (! r.empty ()) {
      r += ",";
    }
    r += "protected";
  }
  return r;
}

static std::string 
method_return (const gsi::MethodBase *method, DocumentationParser & /*doc*/, bool linked = false)
{
  return type_to_s (method->ret_type (), linked, true);
}

static std::string
method_arguments (const gsi::MethodBase *method, const gsi::ClassBase *cls_obj, DocumentationParser &doc, bool linked = false, const char *sep = "<br/>")
{
  std::string r;
  if (method->begin_arguments () == method->end_arguments ()) {

    if (! doc.args.empty ()) {

      r += "(";

      //  special case of external method - just dump the argument names
      for (size_t i = 0; i < doc.args.size (); ++i) {
        if (i > 0) {
          r += ",";
          r += sep;
        }
        r += escape_xml (doc.args [i]);
      }

      r += ")";

    }

    return r;

  } else {

    r += "(";

    int n = 0;
    for (gsi::MethodBase::argument_iterator a = method->begin_arguments (); a != method->end_arguments (); ++a, ++n) {
      if (n > 0) {
        r += ",";
        r += sep;
      }
      r += type_to_s (*a, linked, false);
      r += " ";
      if (a->spec () && !a->spec ()->name ().empty()) {
        r += escape_xml (a->spec ()->name ());
        if (a->spec ()->has_default ()) {
          r += " = ";
          if (! a->spec ()->init_doc ().empty ()) {
            r += replace_references (escape_xml (a->spec ()->init_doc ()), cls_obj);
          } else {
            try {
              r += escape_xml (a->spec ()->default_value ().to_string ());
            } catch (tl::Exception &ex) {
              tl::error << cls_obj->name () << "#" << method->begin_synonyms ()->name << ": " << ex.msg ();
              r += "?";
            }
          }
        }
      } else if (n < int (doc.args.size ())) {
        r += escape_xml (doc.args [n]);
      } else {
        r += "arg" + tl::to_string (n + 1);
      }
    }

    r += ")";

  }

  return r;
}

const gsi::ClassBase *
find_child_with_declaration (const gsi::ClassBase *pc, const gsi::ClassBase *decl)
{
  while (pc) {

    for (tl::weak_collection<gsi::ClassBase>::const_iterator sc = pc->begin_child_classes (); sc != pc->end_child_classes (); ++sc) {

      if (sc->declaration () == decl) {
        return sc.operator-> ();
      }

      const gsi::ClassBase *cc = find_child_with_declaration (sc.operator-> (), decl);
      if (cc) {
        return cc;
      }

    }

    //  Try the base classes - due to skipping of the hidden classes we might pick a parent 
    //  class initially.
    pc = pc->base ();

  }

  return 0;
}

std::string
GSIHelpProvider::produce_class_doc (const std::string &cls) const
{
  std::ostringstream os;

  const gsi::ClassBase *cls_obj = 0;

  std::vector<std::string> comp = tl::split (cls, "::");
  if (comp.empty ()) {
    return "Invalid class name: " + cls;
  }

  std::vector<std::string> cp = comp;

  for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {
    if (c->name () == cp.front ()) {
      cls_obj = &*c;
      cp.erase (cp.begin ());
      break;
    }
  }

  if (! cls_obj) {
    return "Unknown class: " + cls;
  }

  while (! cp.empty ()) {

    const gsi::ClassBase *pc = cls_obj;

    cls_obj = 0;
    while (pc && !cls_obj) {

      for (tl::weak_collection<gsi::ClassBase>::const_iterator sc = pc->begin_child_classes (); sc != pc->end_child_classes (); ++sc) {
        if (sc->name () == cp.front ()) {
          cls_obj = sc.operator-> ();
          cp.erase (cp.begin ());
          break;
        }
      }

      //  Try the base classes too - since we might have skipped some of the classes in the 
      //  inheritance hierarchy, the child may be in a base class.
      pc = pc->base ();

    }

    if (! cls_obj) {
      return "Unknown class: " + cls;
    }

  }

  const gsi::ClassBase *tl_alias = 0;

  if (cls_obj != cls_obj->declaration ()) {

    //  check if there is an alias for this class (the declaration)
    tl_alias = cls_obj->declaration ();

  } else {

    //  check if there is a top-level alias class for this one the other way around (A::B from A_B)
    for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes () && !tl_alias; ++c) {
      tl_alias = find_child_with_declaration (&*c, cls_obj);
    }

  }

  DocumentationParser &class_doc = cls_documentation (cls_obj);

  os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
     << "<!DOCTYPE language SYSTEM \"klayout_doc.dtd\">" << std::endl
     << std::endl;

  os << "<doc><title>" << tl::to_string (QObject::tr ("API reference - Class")) << " " << escape_xml (cls);
  if (class_doc.hidden) {
    os << " " << tl::to_string (QObject::tr ("[internal]"));
  }
  os << "</title>" << std::endl;
  os << "<property name=\"module\" value=\"" << escape_xml (cls_obj->module ()) << "\"/>" << std::endl;

  os << "<keyword name=\"" << escape_xml (cls) << "\"/>" << std::endl;

  os << "<p>"
     << "<link href=\"/about/rba_notation.xml\"/>"
     << "</p>" << std::endl;

  os << "<p><b>" << tl::to_string (QObject::tr ("Module")) << "</b>: <a href=\"" << escape_xml (module_doc_url (cls_obj->module ())) << "\">" << escape_xml (cls_obj->module ()) << "</a></p>";

  os << "<p><b>" << tl::to_string (QObject::tr ("Description")) << "</b>: " << escape_xml (class_doc.brief_doc) << "</p>" << std::endl;

  const gsi::ClassBase *act_cls_obj = real_class (cls_obj);

  std::vector<const gsi::ClassBase *> classes;
  classes.push_back (act_cls_obj);

  const gsi::ClassBase *base = act_cls_obj->base ();
  if (base) {

    const gsi::ClassBase *last_cls = act_cls_obj;
    bool all_collected = false;

    os << "<p><b>" << tl::to_string (QObject::tr ("Class hierarchy")) << "</b>: " << make_qualified_name (cls_obj);
    while (base) {

      DocumentationParser &bdoc = cls_documentation (base);
      if (! bdoc.alias.empty ()) {
        //  suppress direct base class alias to our class (x_Native for x)
        if (bdoc.alias != last_cls->name ()) {
          os << " &#187; <a href=\"" << escape_xml (class_doc_url (bdoc.alias)) << "\">" << escape_xml (bdoc.alias) << "</a>";
          all_collected = true;
        } else if (! all_collected) {
          classes.push_back (base);
        }
      } else if (! bdoc.hidden) {
        os << " &#187; <a href=\"" << escape_xml (class_doc_url (base->name ())) << "\">" << escape_xml (base->name ()) << "</a>";
        all_collected = true;
      } else if (! all_collected) {
        //  class needs to be mixed into the parent
        os << " &#187; <a href=\"" << escape_xml (class_doc_url (base->name ())) << "\">" << escape_xml (base->name ()) << " " << tl::to_string (QObject::tr ("[internal]")) << "</a>";
        classes.push_back (base);
      }

      last_cls = base;
      base = base->base ();

    }

    os << "</p>" << std::endl;

  }

  if (tl_alias) {
    os << "<p>" << tl::to_string (QObject::tr ("This class is equivalent to the class "));
    std::string n = make_qualified_name (tl_alias);
    os << "<a href=\"" << escape_xml (class_doc_url (n)) << "\">" << escape_xml (n) << "</a>";
    os << "</p>" << std::endl;
  }

  //  Produce child classes

  bool any = false;

  for (std::vector<const gsi::ClassBase *>::const_iterator c = classes.begin (); c != classes.end (); ++c) {

    for (tl::weak_collection<gsi::ClassBase>::const_iterator cc = (*c)->begin_child_classes (); cc != (*c)->end_child_classes (); ++cc) {

      DocumentationParser &cdoc = cls_documentation (cc.operator-> ());

      if (any) {
        os << ", ";
      } else {
        os << "<p><b>" << tl::to_string (QObject::tr ("Child classes")) << "</b>: ";
        any = true;
      }

      os << "<a href=\""
         << escape_xml (class_doc_url (make_qualified_name (cc.operator-> ())))
         << "\">"
         << escape_xml (cc->name ());
      if (cdoc.hidden && cdoc.alias.empty ()) {
        os << " " << tl::to_string (QObject::tr ("[internal]"));
      }
      os << "</a>";

    }

  }

  if (any) {
    os << "</p>" << std::endl;
  }

  //  Produce subclasses (parent classes)

  any = false;

  for (tl::weak_collection<gsi::ClassBase>::const_iterator cc = act_cls_obj->begin_subclasses (); cc != act_cls_obj->end_subclasses (); ++cc) {

    DocumentationParser &cdoc = cls_documentation (cc.operator-> ());

    if (any) {
      os << ", ";
    } else {
      os << "<p><b>" << tl::to_string (QObject::tr ("Subclasses")) << "</b>: ";
      any = true;
    }

    os << "<a href=\""
       << escape_xml (class_doc_url (make_qualified_name (cc.operator-> ())))
       << "\">"
       << escape_xml (cc->name ());
    if (cdoc.hidden && cdoc.alias.empty ()) {
      os << " " << tl::to_string (QObject::tr ("[internal]"));
    }
    os << "</a>";

  }

  if (any) {
    os << "</p>" << std::endl;
  }

  //  Inserts an index

  os << "<h2-index/>" << std::endl;

  //  Produce class doc body

  if (class_doc.hidden && class_doc.alias.empty ()) {
    os << "<p><b>"
       << tl::to_string (QObject::tr ("Note"))
       << "</b>: "
       << tl::to_string (QObject::tr (
                           "This class is an internal class provided for technical reasons - i.e. "
                           "as a placeholder class for argument binding or as an abstract interface. "
                           "You should not instantiate objects of this class directly. "
                           "Instead, use the subclasses listed above. "
                           "Also see there for more documentation and actual incarnations of this class."
                        ))
       << "</p>" << std::endl;
  }

  os << replace_references (class_doc.doc_html (), cls_obj) << std::endl;

  //  collect the methods of the class and their hidden base classes
  //  (in the reverse order so that derived classes override their super classes methods)

  std::multimap <std::string, std::pair<const gsi::MethodBase *, size_t> > mm;

  for (size_t n = classes.size (); n > 0; ) {

    --n;

    //  remove the base classes' definitions if the name matches
    for (gsi::ClassBase::method_iterator m = classes [n]->begin_methods (); m != classes [n]->end_methods (); ++m) {
      for (gsi::MethodBase::synonym_iterator syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {
        std::multimap <std::string, std::pair<const gsi::MethodBase *, size_t> >::iterator im;
        while ((im = mm.find (full_name (*syn))) != mm.end ()) {
          mm.erase (im);
        }
      }
    }

    //  add the current classes' methods
    for (gsi::ClassBase::method_iterator m = classes [n]->begin_methods (); m != classes [n]->end_methods (); ++m) {
      for (gsi::MethodBase::synonym_iterator syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {
        DocumentationParser mdoc (*m);
        if (! mdoc.hidden) {
          mm.insert (std::make_pair (full_name (*syn), std::make_pair (*m, syn - (*m)->begin_synonyms ())));
        }
      }
    }

  }

  if (mm.empty ()) {
    os << "</doc>" << std::endl;
    return os.str ();
  }

  //  Produce methods brief descriptions
  
  int n = 0;
  int row = 0;

  any = false;

  for (std::multimap <std::string, std::pair<const gsi::MethodBase *, size_t> >::const_iterator i = mm.begin (); i != mm.end (); ++i, ++n) {

    const gsi::MethodBase::MethodSynonym &syn = i->second.first->begin_synonyms () [i->second.second];

    if (i->second.first->is_static () && syn.name == "new" && ! syn.deprecated && ! i->second.first->is_protected ()) {

      if (! any) {
        os << "<h2>" << tl::to_string (QObject::tr ("Public constructors")) << "</h2>" << std::endl;
        os << "<table>" << std::endl;
        any = true;
      }

      DocumentationParser method_doc (i->second.first);
      os << "<tr class=\"row" << (row % 2)  << "\">" << std::endl;
      ++row;
      os << "<td>" << method_return (i->second.first, method_doc) << "</td>";
      os << "<td><b><a href=\"#method" << n << "\">" << escape_xml (i->first) << "</a></b></td>";
      os << "<td>" << method_arguments (i->second.first, cls_obj, method_doc) << "</td>";
      os << "<td>" << replace_references (escape_xml (method_doc.brief_doc), cls_obj) << "</td>";
      os << "</tr>" << std::endl;

    }

  }

  if (any) {
    os << "</table>" << std::endl;
  }

  n = 0;
  row = 0;
  any = false;

  for (std::multimap <std::string, std::pair<const gsi::MethodBase *, size_t> >::const_iterator i = mm.begin (); i != mm.end (); ++i, ++n) {

    const gsi::MethodBase::MethodSynonym &syn = i->second.first->begin_synonyms () [i->second.second];

    if (! i->second.first->is_static () && ! syn.deprecated && ! i->second.first->is_protected ()) {

      if (! any) {
        os << "<h2>" << tl::to_string (QObject::tr ("Public methods")) << "</h2>" << std::endl;
        os << "<table>" << std::endl;
        any = true;
      }

      DocumentationParser method_doc (i->second.first);
      os << "<tr class=\"row" << (row % 2)  << "\">" << std::endl;
      ++row;
      std::string attr = method_attributes (i->second.first, method_doc, true /*without static*/);
      if (! attr.empty ()) {
        os << "<td><i>[" << escape_xml (method_attributes (i->second.first, method_doc)) << "]</i></td>";
      } else {
        os << "<td></td>";
      }
      os << "<td>" << method_return (i->second.first, method_doc) << "</td>";
      os << "<td><b><a href=\"#method" << n << "\">" << escape_xml (i->first) << "</a></b></td>";
      os << "<td>" << method_arguments (i->second.first, cls_obj, method_doc) << "</td>";
      os << "<td>" << replace_references (escape_xml (method_doc.brief_doc), cls_obj) << "</td>";
      os << "</tr>" << std::endl;

    }

  }

  if (any) {
    os << "</table>" << std::endl;
  }

  //  Produce static methods brief descriptions

  any = false;

  n = 0;
  row = 0;

  for (std::multimap <std::string, std::pair<const gsi::MethodBase *, size_t> >::const_iterator i = mm.begin (); i != mm.end (); ++i, ++n) {

    const gsi::MethodBase::MethodSynonym &syn = i->second.first->begin_synonyms () [i->second.second];

    if (i->second.first->is_static () && syn.name != "new" && ! syn.deprecated && ! i->second.first->is_protected ()) {

      if (! any) {
        any = true;
        os << "<h2>" << tl::to_string (QObject::tr ("Public static methods and constants")) << "</h2>" << std::endl;
        os << "<table>" << std::endl;
      }

      DocumentationParser method_doc (i->second.first);
      os << "<tr class=\"row" << (row % 2)  << "\">" << std::endl;
      ++row;
      std::string attr = method_attributes (i->second.first, method_doc, true /*without static*/);
      if (! attr.empty ()) {
        os << "<td><i>[" << escape_xml (method_attributes (i->second.first, method_doc)) << "]</i></td>";
      } else {
        os << "<td></td>";
      }
      os << "<td>" << method_return (i->second.first, method_doc) << "</td>";
      os << "<td><b><a href=\"#method" << n << "\">" << escape_xml (i->first) << "</a></b></td>";
      os << "<td>" << method_arguments (i->second.first, cls_obj, method_doc) << "</td>";
      os << "<td>" << replace_references (escape_xml (method_doc.brief_doc), cls_obj) << "</td>";
      os << "</tr>" << std::endl;

    }

  }

  if (any) {
    os << "</table>" << std::endl;
  }

  //  Produce protected methods brief descriptions

  any = false;

  n = 0;
  row = 0;

  for (std::multimap <std::string, std::pair<const gsi::MethodBase *, size_t> >::const_iterator i = mm.begin (); i != mm.end (); ++i, ++n) {

    const gsi::MethodBase::MethodSynonym &syn = i->second.first->begin_synonyms () [i->second.second];

    if (! syn.deprecated && i->second.first->is_protected ()) {

      if (! any) {
        any = true;
        os << "<h2>" << tl::to_string (QObject::tr ("Protected methods (static, non-static and constructors)")) << "</h2>" << std::endl;
        os << "<table>" << std::endl;
      }

      DocumentationParser method_doc (i->second.first);
      os << "<tr class=\"row" << (row % 2)  << "\">" << std::endl;
      ++row;
      std::string attr = method_attributes (i->second.first, method_doc);
      if (! attr.empty ()) {
        os << "<td><i>[" << escape_xml (method_attributes (i->second.first, method_doc)) << "]</i></td>";
      } else {
        os << "<td></td>";
      }
      os << "<td>" << method_return (i->second.first, method_doc) << "</td>";
      os << "<td><b><a href=\"#method" << n << "\">" << escape_xml (i->first) << "</a></b></td>";
      os << "<td>" << method_arguments (i->second.first, cls_obj, method_doc) << "</td>";
      os << "<td>";
      os << "<td>" << replace_references (escape_xml (method_doc.brief_doc), cls_obj) << "</td>";
      os << "</td>";
      os << "</tr>" << std::endl;

    }

  }

  if (any) {
    os << "</table>" << std::endl;
  }

  //  Produce deprecated methods brief descriptions

  any = false;

  n = 0;
  row = 0;

  for (std::multimap <std::string, std::pair<const gsi::MethodBase *, size_t> >::const_iterator i = mm.begin (); i != mm.end (); ++i, ++n) {

    const gsi::MethodBase::MethodSynonym &syn = i->second.first->begin_synonyms () [i->second.second];

    if (syn.deprecated) {

      gsi::MethodBase::synonym_iterator nd_syn = i->second.first->begin_synonyms ();
      while (nd_syn != i->second.first->end_synonyms () && nd_syn->deprecated) {
        ++nd_syn;
      }

      if (! any) {
        any = true;
        os << "<h2>" << tl::to_string (QObject::tr ("Deprecated methods (protected, public, static, non-static and constructors)")) << "</h2>" << std::endl;
        os << "<table>" << std::endl;
      }

      DocumentationParser method_doc (i->second.first);
      os << "<tr class=\"row" << (row % 2)  << "\">" << std::endl;
      ++row;
      std::string attr = method_attributes (i->second.first, method_doc, false, false);
      if (! attr.empty ()) {
        os << "<td><i>[" << escape_xml (method_attributes (i->second.first, method_doc)) << "]</i></td>";
      } else {
        os << "<td></td>";
      }
      os << "<td>" << method_return (i->second.first, method_doc) << "</td>";
      os << "<td><b><a href=\"#method" << n << "\">" << escape_xml (i->first) << "</a></b></td>";
      os << "<td>" << method_arguments (i->second.first, cls_obj, method_doc) << "</td>";
      os << "<td>";
      if (nd_syn != i->second.first->end_synonyms ()) {
        os << tl::to_string (QObject::tr ("Use of this method is deprecated. Use")) << " " << full_name (*nd_syn) << " " << tl::to_string (QObject::tr ("instead"));
      } else {
        os << tl::to_string (QObject::tr ("Use of this method is deprecated"));
      }
      os << "</td>";
      os << "</tr>" << std::endl;

    }

  }

  if (any) {
    os << "</table>" << std::endl;
  }

  //  Produce method details

  n = 0;

  os << "<a name=\"detailed\"/><h2>" << tl::to_string (QObject::tr ("Detailed description")) << "</h2>" << std::endl;

  std::string prev_title;

  os << "<table>";

  int rowindex = -1;
  int sigindex = -1;

  for (std::multimap <std::string, std::pair<const gsi::MethodBase *, size_t> >::const_iterator i = mm.begin (); i != mm.end (); ++i, ++n) {

    const gsi::MethodBase::MethodSynonym &syn = i->second.first->begin_synonyms () [i->second.second];

    DocumentationParser method_doc (i->second.first);
    std::string pydoc = pya::PythonInterpreter::python_doc (i->second.first);

    if (i->first != prev_title) {
      rowindex += 1;
    }
    os << "<tr class=\"bigrow" << (rowindex % 2) << "\">";

    if (i->first != prev_title) {
      int rows = 0;
      for (std::multimap <std::string, std::pair<const gsi::MethodBase *, size_t> >::const_iterator j = i; j != mm.end () && j->first == i->first; ++j) {
        ++rows;
      }
      if (rows > 1) {
        os << "<td rowspan=\"" << rows << "\">";
        sigindex = 0;
      } else {
        os << "<td>";
        sigindex = -1;
      }
      os << "<h3>" << escape_xml (i->first) << "</h3>" << std::endl;
      prev_title = i->first;
      os << "</td>";
    }
    os << "<td style=\"padding-bottom: 16px\">";

    os << "<a name=\"method" << n << "\"/>"
       << "<a name=\"m_" << escape_xml (i->first) << "\"/>"
       << "<keyword title=\"" << tl::to_string (QObject::tr ("API reference - Class")) << " " << escape_xml (cls) << ", " << tl::to_string (QObject::tr ("Method")) << " " << escape_xml (i->first) <<  "\" name=\"" << escape_xml (cls) << "#" << escape_xml (i->first) << "\"/>" << std::endl;

    os << "<p><b>";
    if (sigindex >= 0) {
      ++sigindex;
      os << "(" << sigindex << ") ";
    }
    os << tl::to_string (QObject::tr ("Signature")) << "</b>: ";
    std::string attr = method_attributes (i->second.first, method_doc);
    if (! attr.empty ()) {
      os << "<i>[" << escape_xml (attr) << "] </i>";
    }
    os << method_return (i->second.first, method_doc, true) << " <b> " << escape_xml (i->first) << " </b> " << method_arguments (i->second.first, cls_obj, method_doc, true, " ");
    os << "</p>" << std::endl;

    os << "<p><b>" << tl::to_string (QObject::tr ("Description")) << "</b>: " << replace_references (escape_xml (method_doc.brief_doc), cls_obj) << "</p>" << std::endl;

    if (! method_doc.params.empty () || ! method_doc.ret_val.empty ()) {
      os << "<table class=\"layout-table\">" << std::endl;
      for (std::vector<std::pair <std::string, std::string> >::const_iterator p = method_doc.params.begin (); p != method_doc.params.end (); ++p) {
        os << "<tr><td><b>" << escape_xml (p->first) << "</b>:</td><td>" << replace_references (escape_xml (p->second), cls_obj) << "</td></tr>" << std::endl;
      }
      if (! method_doc.ret_val.empty ()) {
        os << "<tr><td><b>" << tl::to_string (QObject::tr ("Returns")) << "</b>:</td><td>" << replace_references (escape_xml (method_doc.ret_val), cls_obj) << "</td></tr>" << std::endl;
      }
      os << "</table>" << std::endl;
    }

    if (syn.deprecated) {

      gsi::MethodBase::synonym_iterator nd_syn = i->second.first->begin_synonyms ();
      while (nd_syn != i->second.first->end_synonyms () && nd_syn->deprecated) {
        ++nd_syn;
      }

      if (nd_syn != i->second.first->end_synonyms ()) {
        os << "<p>" << tl::to_string (QObject::tr ("Use of this method is deprecated. Use")) << " " << full_name (*nd_syn) << " " << tl::to_string (QObject::tr ("instead")) << "</p>" << std::endl;
      } else {
        os << "<p>" << tl::to_string (QObject::tr ("Use of this method is deprecated")) << "</p>" << std::endl;
      }

    }

    std::string dh = method_doc.doc;
    if (! tl::Extractor (dh.c_str ()).at_end ()) {
      os << "<p>" << replace_references (method_doc.doc_html (), cls_obj) << "</p>" << std::endl;
    }

    if (! pydoc.empty ()) {
      os << "<p><b>";
      os << tl::to_string (QObject::tr ("Python specific notes: "));
      os << "</b><br/>" << tl::replaced (escape_xml (pydoc), "\n\n", "<br/>") << "</p>" << std::endl;
    }

    os << "</td></tr>";

  }

  os << "</table>";

  os << "</doc>" << std::endl;
  return os.str ();

}

static tl::RegisteredClass<lay::HelpProvider> gsi_help_provider (new GSIHelpProvider (), 1000);

}


