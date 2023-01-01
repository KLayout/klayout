
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


#ifndef HDR_tlXMLParser
#define HDR_tlXMLParser

#include "tlCommon.h"

#include <list>
#include <vector>

#include "tlAssert.h"
#include "tlInternational.h"
#include "tlString.h"
#include "tlStream.h"

class QIODevice;

namespace tl
{

/**
 *  @brief A basic XML parser error exception class
 */

class TL_PUBLIC XMLException : public tl::Exception
{
public: 
  XMLException (const char *msg)
    : Exception (tl::to_string (tr ("XML parser error: %s")).c_str ()),
      m_msg (msg)
  {
    //  .. nothing yet ..
  }

  XMLException (const std::string &msg)
    : Exception (fmt (-1, -1).c_str (), msg.c_str ()),
      m_msg (msg)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Raw (unprefixed) message of the XML parser
   */
  const std::string &
  raw_msg () const
  {
    return m_msg;
  }

protected:
  XMLException (const std::string &msg, int line, int column)
    : Exception (fmt (line, column).c_str (), msg.c_str (), line, column),
      m_msg (msg)
  {
    //  .. nothing yet ..
  }

private:
  std::string m_msg;

  static std::string fmt (int line, int /*column*/)
  {
    if (line < 0) {
      return tl::to_string (tr ("XML parser error: %s")).c_str ();
    } else {
      return tl::to_string (tr ("XML parser error: %s in line %d, column %d")).c_str ();
    }
  }
};
 
/**
 *  @brief A XML parser error exception class that additionally provides line and column information
 */

class TL_PUBLIC XMLLocatedException : public XMLException
{
public: 
  XMLLocatedException (const std::string &msg, int line, int column)
    : XMLException (msg, line, column),
      m_line (line), m_column (column)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Line number information of the exception
   */
  int line () const
  {
    return m_line;
  }

  /**
   *  @brief Column number information of the exception
   */
  int column () const
  {
    return m_column;
  }

private:
  int m_line;
  int m_column;
};
 
/**
 *  @brief An object wrapper base class for target object management
 *
 *  Implementations of this class through the XMLReaderProxy templates
 *  manage pointers to certain objects.
 */

class TL_PUBLIC XMLReaderProxyBase
{
public:
  XMLReaderProxyBase () { }
  virtual ~XMLReaderProxyBase () { }
  virtual void release () = 0;
  virtual void detach () = 0;
};

/**
 *  @brief An object wrapper base class for target object management specialized to a certain class
 */

template <class Obj>
class TL_PUBLIC_TEMPLATE XMLReaderProxy
  : public XMLReaderProxyBase
{
public:
  XMLReaderProxy (Obj *obj, bool owns_obj) 
    : mp_obj (obj), m_owns_obj (owns_obj) 
  { }

  virtual ~XMLReaderProxy () { }

  virtual void release () 
  {
    if (m_owns_obj && mp_obj) {
      delete mp_obj;
    }
    mp_obj = 0;
  }

  virtual void detach ()
  {
    m_owns_obj = false;
  }

  Obj *ptr () const 
  {
    return mp_obj;
  }

private:
  Obj *mp_obj;
  bool m_owns_obj;
};

/**
 *  @brief Helper class: A class tag
 */

template <class Obj>
struct XMLObjTag
{
  XMLObjTag() { }
  typedef Obj obj;
};

/**
 *  @brief Helper class: The reader state
 * 
 *  The reader state mainly comprises of a stack of objects being parsed and
 *  a string in which to collect cdata.
 */

class TL_PUBLIC XMLReaderState 
{
public:
  /**
   *  @brief Default constructor
   */
  XMLReaderState ();

  /**
   *  @brief Destructor
   */
  ~XMLReaderState ();

  /**
   *  @brief Push a new object on the stack
   */
  template <class Obj>
  void push (XMLObjTag<Obj> /*tag*/)
  {
    m_objects.push_back (new XMLReaderProxy<Obj> (new Obj (), true));
  }

  /**
   *  @brief Push an existing object on the stack
   */
  template <class Obj>
  void push (Obj *obj)
  {
    m_objects.push_back (new XMLReaderProxy<Obj> (obj, false));
  }

  /**
   *  @brief Push an existing object on the stack with the ownership flag
   */
  template <class Obj>
  void push (Obj *obj, bool owner)
  {
    m_objects.push_back (new XMLReaderProxy<Obj> (obj, owner));
  }

  /**
   *  @brief Get the top object
   */
  template <class Obj>
  Obj *back (XMLObjTag<Obj> /*tag*/) 
  {
    tl_assert (! m_objects.empty ());
    return (dynamic_cast <XMLReaderProxy<Obj> &> (*m_objects.back ())).ptr ();
  }

  /**
   *  @brief Get the top object and release
   */
  template <class Obj>
  Obj *detach_back (XMLObjTag<Obj> /*tag*/) 
  {
    tl_assert (! m_objects.empty ());
    m_objects.back ()->detach ();
    return (dynamic_cast <XMLReaderProxy<Obj> &> (*m_objects.back ())).ptr ();
  }

  /**
   *  @brief Pop an object from the stack
   */
  template <class Obj>
  void pop (XMLObjTag<Obj> /*tag*/) 
  {
    tl_assert (! m_objects.empty ());
    m_objects.back ()->release ();
    delete m_objects.back ();
    m_objects.pop_back ();
  }

  /**
   *  @brief Empty predicate: true, if no more object is on the stack
   */
  bool empty () const
  {
    return m_objects.empty ();
  }

  /**
   *  @brief Obtain the parent object from the stack
   */
  template <class Obj>
  Obj *parent (XMLObjTag<Obj> /*tag*/) 
  {
    tl_assert (m_objects.size () > 1);
    return (dynamic_cast <XMLReaderProxy<Obj> &> (*m_objects.end () [-2])).ptr ();
  }

  /**
   *  @brief The cdata string collected
   */
  std::string cdata;

private:
  std::vector <XMLReaderProxyBase *> m_objects;
};

//  The opaque source type
class XMLSourcePrivateData;

/**
 *  @brief A generic XML text source class
 *
 *  This class is the base class providing input for 
 *  the Qt XML parser and basically maps to a QXmlInputSource object
 *  for compatibility with the "libparsifal" branch.
 */

class TL_PUBLIC XMLSource 
{
public:
  XMLSource ();
  ~XMLSource ();

  XMLSourcePrivateData *source ()
  {
    return mp_source;
  }

  void reset ();

protected:
  void set_source (XMLSourcePrivateData *source)
  {
    mp_source = source;
  }

private:
  XMLSourcePrivateData *mp_source;
};

/**
 *  @brief A specialization of XMLSource to receive a string
 */

class TL_PUBLIC XMLStringSource : public XMLSource
{
public:
  XMLStringSource (const std::string &string);
  XMLStringSource (const char *cp);
  XMLStringSource (const char *cp, size_t len);
  ~XMLStringSource ();

private:
  std::string m_copy;
};

/**
 *  @brief A specialization of XMLSource to receive from a file
 */

class TL_PUBLIC XMLFileSource : public XMLSource
{
public:
  XMLFileSource (const std::string &path);
  XMLFileSource (const std::string &path, const std::string &progress_message);
  ~XMLFileSource ();
};

/**
 *  @brief A generic stream source class
 *
 *  This class implements a XML parser source from a tl::InputStream
 */

class TL_PUBLIC XMLStreamSource : public XMLSource
{
public:
  XMLStreamSource (tl::InputStream &stream);
  XMLStreamSource (tl::InputStream &stream, const std::string &progress_message);
  ~XMLStreamSource ();
};


// -----------------------------------------------------------------
//  The C++ structure definition interface (for use cases see tlXMLParser.ut)

class TL_PUBLIC XMLElementBase;

struct pass_by_value_tag { 
  pass_by_value_tag () { } 
};

struct pass_by_ref_tag { 
  pass_by_ref_tag () { } 
};

/**
 *  @brief The XML handler implementation
 *
 *  This class implements a XML handler using the given hierarchy of
 *  elements to implement the search algorithm.
 */

class TL_PUBLIC XMLStructureHandler
{
public:
  XMLStructureHandler (const XMLElementBase *root, XMLReaderState *reader_state);

  void characters (const std::string &ch);
  void end_element (const std::string &uri, const std::string &lname, const std::string &qname);
  void start_element (const std::string &uri, const std::string &lname, const std::string &qname);

private:
  std::vector <const XMLElementBase *> m_stack;
  const XMLElementBase *mp_root;
  XMLReaderState *mp_state;
};

class XMLParserPrivateData;

/**
 *  @brief The XML parser class
 *  This is the main entry point. It will take a structure handler and
 *  parse the given source.
 */
class TL_PUBLIC XMLParser
{
public:
  XMLParser ();
  ~XMLParser ();

  void parse (XMLSource &source, XMLStructureHandler &handler);

  /**
   *  @brief Returns true, if XML support is compiled in
   */
  static bool is_available ();

private:
  XMLParserPrivateData *mp_data;
};

/**
 *  @brief XMLElementProxy element of the XML structure definition
 *
 *  The purpose of this class is to provide a wrapper around the 
 *  different derivations of the XMLElementBase class, so we can 
 *  pack them into a vector. The proxy objects can be copied
 *  duplicating the wrapped objects with their "clone" methods.
 *  The proxy object can be given the pointer to the element 
 *  in two ways: the reference version creates a duplicate, the
 *  pointer version transfers the ownership of the XMLElementBase 
 *  object.
 */

class TL_PUBLIC XMLElementProxy
{
public:
  XMLElementProxy (const XMLElementProxy &d);
  XMLElementProxy (const XMLElementBase &d);
  XMLElementProxy (XMLElementBase *d);

  ~XMLElementProxy ();

  XMLElementBase *operator-> () const
  {
    return mp_ptr;
  }

  XMLElementBase *get () const
  {
    return mp_ptr;
  }

private:
  XMLElementBase *mp_ptr;
};

/**
 *  @brief A list of XML elements
 *
 *  This class provides a list of XML elements. 
 *  A list can be created by using the + operator and 
 *  supplying various objects based on XMLElementBase.
 */

class TL_PUBLIC XMLElementList 
{
public:
  typedef std::list <XMLElementProxy> children_list;
  typedef children_list::const_iterator iterator;

  XMLElementList ()
  {
    //  .. nothing yet ..
  }

  XMLElementList (const XMLElementBase &e)
  {
    m_elements.push_back (XMLElementProxy (e));
  }

  XMLElementList (XMLElementBase *e)
  {
    if (e) {
      m_elements.push_back (XMLElementProxy (e));
    }
  }

  XMLElementList (const XMLElementList &d, const XMLElementBase &e)
    : m_elements (d.m_elements)
  {
    m_elements.push_back (XMLElementProxy (e));
  }

  XMLElementList (const XMLElementList &d, XMLElementBase *e)
    : m_elements (d.m_elements)
  {
    if (e) {
      m_elements.push_back (XMLElementProxy (e));
    }
  }

  void append (const XMLElementBase &e)
  {
    m_elements.push_back (XMLElementProxy (e));
  }

  void append (XMLElementBase *e)
  {
    if (e) {
      m_elements.push_back (XMLElementProxy (e));
    }
  }

  iterator begin () const
  {
    return m_elements.begin ();
  }

  iterator end () const
  {
    return m_elements.end ();
  }

  static XMLElementList empty () 
  {
    return XMLElementList ();
  }

private:
  std::list <XMLElementProxy> m_elements; 
};

/**
 *  @brief Helper class: A stack of const objects being written 
 */

class TL_PUBLIC XMLWriterState 
{
public:
  /**
   *  @brief Default constructor
   */
  XMLWriterState ();

  /**
   *  @brief Push a new object on the stack
   */
  template <class Obj>
  void push (const Obj *obj)
  {
    m_objects.push_back (obj);
  }

  /**
   *  @brief Pop an object from the stack
   */
  template <class Obj>
  const Obj *pop (XMLObjTag<Obj> /*tag*/) 
  {
    tl_assert (! m_objects.empty ());
    const Obj *obj = reinterpret_cast <const Obj *> (m_objects.back ());
    m_objects.pop_back ();
    return obj;
  }

  /**
   *  @brief Obtain the parent object from the stack
   */
  template <class Obj>
  const Obj *back (XMLObjTag<Obj> /*tag*/) 
  {
    tl_assert (m_objects.size () > 0);
    return reinterpret_cast <const Obj *> (m_objects.end () [-1]);
  }

private:
  std::vector <const void *> m_objects;
};

/**
 *  @brief The XML element base object
 *
 *  This class is the base class for objects implementing
 *  the parser handler semantics. The basic methods are 
 *  create (create an actual object), cdata (supply data),
 *  finish (finalize the actual object).
 */

class TL_PUBLIC XMLElementBase
{
public:
  typedef XMLElementList::iterator iterator;

  XMLElementBase (const std::string &name, const XMLElementList &children)
    : m_name (name), mp_children (new XMLElementList (children)), m_owns_child_list (true)
  {
    // .. nothing yet ..
  }

  XMLElementBase (const std::string &name, const XMLElementList *children)
    : m_name (name), mp_children (children), m_owns_child_list (false)
  {
    // .. nothing yet ..
  }

  XMLElementBase (const XMLElementBase &d)
    : m_name (d.m_name), m_owns_child_list (d.m_owns_child_list)
  {
    if (m_owns_child_list) {
      mp_children = new XMLElementList (*d.mp_children);
    } else {
      mp_children = d.mp_children;
    }
  }

  virtual ~XMLElementBase ()
  {
    if (m_owns_child_list) {
      delete const_cast <XMLElementList *> (mp_children);
      mp_children = 0;
    }
  }

  virtual XMLElementBase *clone () const = 0;

  virtual void create (const XMLElementBase *parent, XMLReaderState &objs, const std::string &uri, const std::string &lname, const std::string &qname) const = 0;
  virtual void cdata (const std::string &cdata, XMLReaderState &objs) const = 0;
  virtual void finish (const XMLElementBase *parent, XMLReaderState &objs, const std::string &uri, const std::string &lname, const std::string &qname) const = 0;

  virtual void write (const XMLElementBase * /*parent*/, tl::OutputStream & /*os*/, int /*indent*/, XMLWriterState & /*objs*/) const { }
  virtual bool has_any (XMLWriterState & /*objs*/) const { return false; }

  static void write_indent (tl::OutputStream &os, int indent);
  static void write_string (tl::OutputStream &os, const std::string &s);

  const std::string &name () const
  {
    return m_name;
  }

  bool check_name (const std::string & /*uri*/, const std::string &lname, const std::string & /*qname*/) const
  {
    if (m_name == "*") {
      return true;
    } else {
      return m_name == lname; // no namespace currently
    }
  }

  iterator begin () const
  {
    return mp_children->begin ();
  }

  iterator end () const
  {
    return mp_children->end ();
  }

private:
  std::string m_name;
  const XMLElementList *mp_children;
  bool m_owns_child_list;
};

/**
 *  @brief A XML child element 
 *
 *  This class is a XML structure component describing a child
 *  element in the XML tree. There is no limit about the number of
 *  child elements. 
 *  This object must be provided a pointer to a factory method (in
 *  the parent's class), a name and a list of children (which can be
 *  empty).
 *  Write is a class providing a Obj &operator(Parent &) operator.
 *  It is supposed to create a new instance of Obj within Parent.
 *  Read is a class providing a start(const Parent &) method to start 
 *  iterating over the instances, a const Obj &operator() const for 
 *  access and a bool at_end() method to determine if the iterator
 *  is at the end and next() to increment the iterator.
 */

template <class Obj, class Parent, class Read, class Write>
class TL_PUBLIC_TEMPLATE XMLElement
  : public XMLElementBase
{
public:
  XMLElement (const Read &r, const Write &w, const std::string &name, const XMLElementList &children)
    : XMLElementBase (name, children), m_r (r), m_w (w)
  {
    // .. nothing yet ..
  }

  XMLElement (const Read &r, const Write &w, const std::string &name, const XMLElementList *children)
    : XMLElementBase (name, children), m_r (r), m_w (w)
  {
    // .. nothing yet ..
  }

  XMLElement (const XMLElement<Obj, Parent, Read, Write> &d)
    : XMLElementBase (d), m_r (d.m_r), m_w (d.m_w)
  {
    // .. nothing yet ..
  }

  virtual XMLElementBase *clone () const
  {
    return new XMLElement<Obj, Parent, Read, Write> (*this);
  }

  virtual void create (const XMLElementBase *, XMLReaderState &objs, const std::string & /*uri*/, const std::string & /*lname*/, const std::string & /*qname*/) const
  {
    XMLObjTag<Obj> tag;
    objs.push (tag);
  }

  virtual void cdata (const std::string & /*cdata*/, XMLReaderState & /*objs*/) const
  {
    // .. nothing yet ..
  }

  virtual void finish (const XMLElementBase * /*parent*/, XMLReaderState &objs, const std::string & /*uri*/, const std::string & /*lname*/, const std::string & /*qname*/) const
  {
    XMLObjTag<Obj> tag;
    XMLObjTag<Parent> parent_tag;
    m_w (*objs.parent (parent_tag), objs);
    objs.pop (tag);
  }

  virtual void write (const XMLElementBase * /*parent*/, tl::OutputStream &os, int indent, XMLWriterState &objs) const
  {
    XMLObjTag<Parent> parent_tag;
    Read r (m_r);
    r.start (*objs.back (parent_tag));
    while (! r.at_end ()) {
      XMLElementBase::write_indent (os, indent);
      os << "<" << this->name () << ">\n";
      typedef typename Read::tag read_tag_type;
      read_tag_type read_tag;
      write_obj (r (), os, indent, read_tag, objs);
      XMLElementBase::write_indent (os, indent);
      os << "</" << this->name () << ">\n";
      r.next ();
    }
  }

  virtual bool has_any (XMLWriterState &objs) const 
  {
    XMLObjTag<Parent> parent_tag;
    Read r (m_r);
    r.start (*objs.back (parent_tag));
    return (! r.at_end ());
  }

private:
  Read m_r;
  Write m_w;

  //  this write helper is used if the reader delivers an object by value
  void write_obj (Obj obj, tl::OutputStream &os, int indent, tl::pass_by_value_tag, XMLWriterState &objs) const
  {
    XMLObjTag<Obj> tag;
    objs.push (&obj);
    for (XMLElementBase::iterator c = this->begin (); c != this->end (); ++c) {
      c->get ()->write (this, os, indent + 1, objs);
    }
    objs.pop (tag);
  }

  void write_obj (const Obj &obj, tl::OutputStream &os, int indent, tl::pass_by_ref_tag, XMLWriterState &objs) const
  {
    XMLObjTag<Obj> tag;
    objs.push (&obj);
    for (XMLElementBase::iterator c = this->begin (); c != this->end (); ++c) {
      c->get ()->write (this, os, indent + 1, objs);
    }
    objs.pop (tag);
  }
};

/**
 *  @brief A XML child element with is instantiated with a parent reference
 *
 *  This declares a XML element with a constructor that takes a (object *) arguments
 *  to the parent.
 */

template <class Obj, class Parent, class Read, class Write>
class TL_PUBLIC_TEMPLATE XMLElementWithParentRef
  : public XMLElementBase
{
public:
  XMLElementWithParentRef (const Read &r, const Write &w, const std::string &name, const XMLElementList &children)
    : XMLElementBase (name, children), m_r (r), m_w (w)
  {
    // .. nothing yet ..
  }

  XMLElementWithParentRef (const Read &r, const Write &w, const std::string &name, const XMLElementList *children)
    : XMLElementBase (name, children), m_r (r), m_w (w)
  {
    // .. nothing yet ..
  }

  XMLElementWithParentRef (const XMLElementWithParentRef<Obj, Parent, Read, Write> &d)
    : XMLElementBase (d), m_r (d.m_r), m_w (d.m_w)
  {
    // .. nothing yet ..
  }

  virtual XMLElementBase *clone () const
  {
    return new XMLElementWithParentRef<Obj, Parent, Read, Write> (*this);
  }

  virtual void create (const XMLElementBase *, XMLReaderState &objs, const std::string & /*uri*/, const std::string & /*lname*/, const std::string & /*qname*/) const
  {
    XMLObjTag<Obj> tag;
    XMLObjTag<Parent> parent_tag;
    objs.push (new Obj (objs.back (parent_tag)), true);
  }

  virtual void cdata (const std::string & /*cdata*/, XMLReaderState & /*objs*/) const
  {
    // .. nothing yet ..
  }

  virtual void finish (const XMLElementBase * /*parent*/, XMLReaderState &objs, const std::string & /*uri*/, const std::string & /*lname*/, const std::string & /*qname*/) const
  {
    XMLObjTag<Obj> tag;
    XMLObjTag<Parent> parent_tag;
    m_w (*objs.parent (parent_tag), objs);
    objs.pop (tag);
  }

  virtual void write (const XMLElementBase * /*parent*/, tl::OutputStream &os, int indent, XMLWriterState &objs) const
  {
    XMLObjTag<Parent> parent_tag;
    Read r (m_r);
    r.start (*objs.back (parent_tag));
    while (! r.at_end ()) {
      XMLElementBase::write_indent (os, indent);
      os << "<" << this->name () << ">\n";
      typedef typename Read::tag read_tag_type;
      read_tag_type read_tag;
      write_obj (r (), os, indent, read_tag, objs);
      XMLElementBase::write_indent (os, indent);
      os << "</" << this->name () << ">\n";
      r.next ();
    }
  }

  virtual bool has_any (XMLWriterState &objs) const 
  {
    XMLObjTag<Parent> parent_tag;
    Read r (m_r);
    r.start (*objs.back (parent_tag));
    return (! r.at_end ());
  }

private:
  Read m_r;
  Write m_w;

  //  this write helper is used if the reader delivers an object by value
  void write_obj (Obj obj, tl::OutputStream &os, int indent, tl::pass_by_value_tag, XMLWriterState &objs) const
  {
    XMLObjTag<Obj> tag;
    objs.push (&obj);
    for (XMLElementBase::iterator c = this->begin (); c != this->end (); ++c) {
      c->get ()->write (this, os, indent + 1, objs);
    }
    objs.pop (tag);
  }

  void write_obj (const Obj &obj, tl::OutputStream &os, int indent, tl::pass_by_ref_tag, XMLWriterState &objs) const
  {
    XMLObjTag<Obj> tag;
    objs.push (&obj);
    for (XMLElementBase::iterator c = this->begin (); c != this->end (); ++c) {
      c->get ()->write (this, os, indent + 1, objs);
    }
    objs.pop (tag);
  }
};

/**
 *  @brief A XML child element mapped to a member
 *
 *  This class is a XML structure component describing a child
 *  element in the XML tree. The value of the child element 
 *  is directly mapped to a member of the parent's class.
 *  This object must be provided two adaptors: 
 *  Write is a writer providing an operator() (Parent &, const Value &)
 *  that adds a new value or sets the value.
 *  Read is a class providing a start(const Parent &) method to start 
 *  iterating over the instances, a const Value &operator() const for 
 *  access and a bool at_end() method to determine if the iterator
 *  is at the end and next() to increment the iterator.
 */

template <class Value, class Parent, class Read, class Write, class Converter>
class TL_PUBLIC_TEMPLATE XMLMember
  : public XMLElementBase
{
public:
  XMLMember (const Read &r, const Write &w, const std::string &name, Converter c = Converter ())
    : XMLElementBase (name, XMLElementList::empty ()), m_r (r), m_w (w), m_c (c)
  {
    // .. nothing yet ..
  }

  XMLMember (const XMLMember<Value, Parent, Read, Write, Converter> &d)
    : XMLElementBase (d), m_r (d.m_r), m_w (d.m_w), m_c (d.m_c)
  {
    // .. nothing yet ..
  }

  virtual XMLElementBase *clone () const
  {
    return new XMLMember<Value, Parent, Read, Write, Converter> (*this);
  }

  virtual void create (const XMLElementBase *, XMLReaderState &objs, const std::string & /*uri*/, const std::string & /*lname*/, const std::string & /*qname*/) const
  {
    objs.cdata = "";
  }

  virtual void cdata (const std::string &cd, XMLReaderState &objs) const
  {
    objs.cdata += cd;
  }

  virtual void finish (const XMLElementBase * /*parent*/, XMLReaderState &objs, const std::string & /*uri*/, const std::string & /*lname*/, const std::string & /*qname*/) const
  {
    XMLObjTag<Value> tag;
    XMLObjTag<Parent> parent_tag;

    XMLReaderState value_obj;
    value_obj.push (tag);

    m_c.from_string (objs.cdata, *value_obj.back (tag));
    m_w (*objs.back (parent_tag), value_obj);

    value_obj.pop (tag);
  }

  virtual void write (const XMLElementBase * /*parent*/, tl::OutputStream &os, int indent, XMLWriterState &objs) const
  {
    XMLObjTag<Parent> parent_tag;
    Read r (m_r);
    r.start (* objs.back (parent_tag));
    while (! r.at_end ()) {

      std::string value = m_c.to_string (r ());

      write_indent (os, indent);

      if (value.empty ()) {
        os << "<" << name () << "/>\n";
      } else {
        os << "<" << name () << ">";
        write_string (os, value);
        os << "</" << name () << ">\n";
      }

      r.next ();

    }
  }

  virtual bool has_any (XMLWriterState &objs) const 
  {
    XMLObjTag<Parent> parent_tag;
    Read r (m_r);
    r.start (*objs.back (parent_tag));
    return (! r.at_end ());
  }

private:
  Read m_r;
  Write m_w;
  Converter m_c;
};

/**
 *  @brief A XML wildcard reader
 *
 *  This class is a XML structure component that passes a key/value 
 *  pair to a given read adaptor. 
 */

template <class Value, class Parent, class Write, class Converter>
class TL_PUBLIC_TEMPLATE XMLWildcardMember
  : public XMLElementBase
{
public:
  XMLWildcardMember (const Write &w, Converter c = Converter ())
    : XMLElementBase ("*", XMLElementList::empty ()), m_w (w), m_c (c)
  {
    // .. nothing yet ..
  }

  XMLWildcardMember (const XMLWildcardMember<Value, Parent, Write, Converter> &d)
    : XMLElementBase (d), m_w (d.m_w), m_c (d.m_c)
  {
    // .. nothing yet ..
  }

  virtual XMLElementBase *clone () const
  {
    return new XMLWildcardMember<Value, Parent, Write, Converter> (*this);
  }

  virtual void create (const XMLElementBase *, XMLReaderState &objs, const std::string & /*uri*/, const std::string & /*lname*/, const std::string & /*qname*/) const
  {
    objs.cdata = "";
  }

  virtual void cdata (const std::string &cd, XMLReaderState &objs) const
  {
    objs.cdata += cd;
  }

  virtual void finish (const XMLElementBase * /*parent*/, XMLReaderState &objs, const std::string & /*uri*/, const std::string &lname, const std::string & /*qname*/) const
  {
    XMLObjTag<Value> tag;
    XMLObjTag<Parent> parent_tag;

    XMLReaderState value_obj;
    value_obj.push (tag);

    m_c.from_string (objs.cdata, *value_obj.back (tag));
    m_w (*objs.back (parent_tag), value_obj, lname);

    value_obj.pop (tag);
  }

  virtual void write (const XMLElementBase * /*parent*/, tl::OutputStream & /*os*/, int  /*indent*/, XMLWriterState & /*objs*/) const
  {
  }

  virtual bool has_any (XMLWriterState & /*objs*/) const 
  {
    return false;
  }

private:
  Write m_w;
  Converter m_c;
};

/**
 *  @brief The root element of the XML structure
 *
 *  The root element is also the handler implementation.
 *  It must be supplied a pointer to an actual root object,
 *  a name and a list of children.
 */

template <class Obj>
class TL_PUBLIC_TEMPLATE XMLStruct
  : public XMLElementBase
{
public:
  XMLStruct (const std::string &name, const XMLElementList *children)
    : XMLElementBase (name, children)
  {
    // .. nothing yet ..
  }

  XMLStruct (const std::string &name, const XMLElementList &children)
    : XMLElementBase (name, children)
  {
    // .. nothing yet ..
  }

  XMLStruct (const XMLStruct<Obj> &d)
    : XMLElementBase (d)
  {
    // .. nothing yet ..
  }

  virtual XMLElementBase *clone () const
  {
    return new XMLStruct<Obj> (*this);
  }

  virtual void create (const XMLElementBase *, XMLReaderState &, const std::string & /*uri*/, const std::string & /*lname*/, const std::string & /*qname*/) const
  {
    // .. nothing yet ..
  }

  virtual void cdata (const std::string &, XMLReaderState &) const
  {
    // .. nothing yet ..
  }

  virtual void finish (const XMLElementBase *, XMLReaderState &, const std::string & /*uri*/, const std::string & /*lname*/, const std::string & /*qname*/) const
  {
    // .. nothing yet ..
  }

  void write (tl::OutputStream &os, const Obj &root) const
  {
    XMLWriterState writer_state;
    writer_state.push (& root);

    os << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    os << "<" << this->name () << ">\n";
    for (XMLElementBase::iterator c = this->begin (); c != this->end (); ++c) {
      c->get ()->write (this, os, 1, writer_state);
    }
    os << "</" << this->name () << ">\n";

    os.flush ();
  }

  void parse (XMLSource &source, Obj &root) const
  {
    XMLObjTag<Obj> tag;
    XMLParser p;
    XMLReaderState rs;
    rs.push (&root);
    XMLStructureHandler h (this, &rs);
    p.parse (source, h);
    rs.pop (tag);
    tl_assert (rs.empty ());
  }

private:
  virtual void write (const XMLElementBase*, tl::OutputStream &, int, XMLWriterState &) const
  {
    // .. see write (os)
  }
};

/**
 *  @brief Utility: add a XML element to a list of some
 */
inline XMLElementList 
operator+ (const XMLElementList &l, const XMLElementBase &e)
{
  return XMLElementList (l, e);
}

/**
 *  @brief Utility: add a XML element to a list of some
 *
 *  This version transfers the ownership of the XMLElementBase object.
 */
inline XMLElementList 
operator+ (const XMLElementList &l, XMLElementBase *e)
{
  return XMLElementList (l, e);
}


//  Several adaptor classes needed to implement the various make_* utilities

template <class Value, class Parent>
struct XMLMemberDummyWriteAdaptor
{
  XMLMemberDummyWriteAdaptor ()
  {
    // .. nothing yet ..
  }
  
  void operator () (Parent &, XMLReaderState &) const
  {
    // .. nothing yet ..
  }
};
 
template <class Value, class Parent>
struct XMLMemberWriteAdaptor
{
  XMLMemberWriteAdaptor (Value Parent::*member)
    : mp_member (member)
  {
    // .. nothing yet ..
  }
  
  void operator () (Parent &owner, XMLReaderState &reader) const
  {
    XMLObjTag<Value> tag;
    owner.*mp_member = *reader.back (tag);
  }

private:
  Value Parent::*mp_member;
};
 
template <class Value, class Parent>
struct XMLMemberAccRefWriteAdaptor
{
  XMLMemberAccRefWriteAdaptor (void (Parent::*member) (const Value &))
    : mp_member (member)
  {
    // .. nothing yet ..
  }
  
  void operator () (Parent &owner, XMLReaderState &reader) const
  {
    XMLObjTag<Value> tag;
    (owner.*mp_member) (*reader.back (tag));
  }

private:
  void (Parent::*mp_member) (const Value &);
};
 
template <class Value, class Parent>
struct XMLMemberTransferWriteAdaptor
{
  XMLMemberTransferWriteAdaptor (void (Parent::*member) (Value *))
    : mp_member (member)
  {
    // .. nothing yet ..
  }
  
  void operator () (Parent &owner, XMLReaderState &reader) const
  {
    XMLObjTag<Value> tag;
    (owner.*mp_member) (reader.detach_back (tag));
  }

private:
  void (Parent::*mp_member) (Value *);
};
 
template <class Value, class Parent>
struct XMLMemberAccWriteAdaptor
{
  XMLMemberAccWriteAdaptor (void (Parent::*member) (Value))
    : mp_member (member)
  {
    // .. nothing yet ..
  }
  
  void operator () (Parent &owner, XMLReaderState &reader) const
  {
    XMLObjTag<Value> tag;
    (owner.*mp_member) (*reader.back (tag));
  }

private:
  void (Parent::*mp_member) (Value);
};
 
template <class Value, class Parent>
struct XMLMemberDummyReadAdaptor
{
  typedef pass_by_ref_tag tag;

  XMLMemberDummyReadAdaptor ()
  {
    // .. nothing yet ..
  }
  
  Value operator () () const
  {
    return Value ();
  }

  bool at_end () const 
  {
    return true;
  }

  void start (const Parent &) 
  {
    // .. nothing yet ..
  }

  void next () 
  {
    // .. nothing yet ..
  }
};
 
template <class Value, class Parent>
struct XMLMemberReadAdaptor
{
  typedef pass_by_ref_tag tag;

  XMLMemberReadAdaptor (Value Parent::*member)
    : mp_member (member), mp_owner (0), m_done (false)
  {
    // .. nothing yet ..
  }
  
  const Value &operator () () const
  {
    return mp_owner->*mp_member;
  }

  bool at_end () const 
  {
    return m_done;
  }

  void start (const Parent &owner) 
  {
    mp_owner = &owner;
    m_done = false;
  }

  void next () 
  {
    m_done = true;
  }

private:
  Value Parent::*mp_member;
  const Parent *mp_owner;
  bool m_done;
};
 
template <class Value, class Parent>
struct XMLMemberAccRefReadAdaptor
{
  typedef pass_by_ref_tag tag;

  XMLMemberAccRefReadAdaptor (const Value &(Parent::*member) () const)
    : mp_member (member), mp_owner (0), m_done (false)
  {
    // .. nothing yet ..
  }
  
  const Value &operator () () const
  {
    return (mp_owner->*mp_member) ();
  }

  bool at_end () const 
  {
    return m_done;
  }

  void start (const Parent &owner) 
  {
    mp_owner = &owner;
    m_done = false;
  }

  void next () 
  {
    m_done = true;
  }

private:
  const Value &(Parent::*mp_member) () const;
  const Parent *mp_owner;
  bool m_done;
};
 
template <class Value, class Parent>
struct XMLMemberAccReadAdaptor
{
  typedef pass_by_value_tag tag;

  XMLMemberAccReadAdaptor (Value (Parent::*member) () const)
    : mp_member (member), mp_owner (0), m_done (false)
  {
    // .. nothing yet ..
  }
  
  Value operator () () const
  {
    return (mp_owner->*mp_member) ();
  }

  bool at_end () const 
  {
    return m_done;
  }

  void start (const Parent &owner) 
  {
    mp_owner = &owner;
    m_done = false;
  }

  void next () 
  {
    m_done = true;
  }

private:
  Value (Parent::*mp_member) () const;
  const Parent *mp_owner;
  bool m_done;
};
 
template <class Value, class Iter, class Parent>
struct XMLMemberIterReadAdaptor
{
  typedef pass_by_ref_tag tag;

  XMLMemberIterReadAdaptor (Iter (Parent::*begin) () const, Iter (Parent::*end) () const)
    : mp_begin (begin), mp_end (end)
  {
    // .. nothing yet ..
  }
  
  Value operator () () const
  {
    return *m_iter;
  }

  bool at_end () const 
  {
    return m_iter == m_end;
  }

  void start (const Parent &parent) 
  {
    m_iter = (parent.*mp_begin) ();
    m_end = (parent.*mp_end) ();
  }

  void next () 
  {
    ++m_iter;
  }

private:
  Iter (Parent::*mp_begin) () const;
  Iter (Parent::*mp_end) () const;
  Iter m_iter, m_end;
};
 
/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element (void (Parent::*setter) (const Value &), const std::string &name, const XMLElementList *children)
{
  return XMLElement<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          XMLMemberDummyReadAdaptor <Value, Parent> (), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > 
make_element (void (Parent::*setter) (Value *), const std::string &name, const XMLElementList *children)
{
  return XMLElement<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > ( 
          XMLMemberDummyReadAdaptor <Value, Parent> (), 
          XMLMemberTransferWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, const XMLElementList *children)
{
  return XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > 
make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, const XMLElementList *children)
{
  return XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberTransferWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent> > 
make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, const XMLElementList *children)
{
  return XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element (Value (Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, const XMLElementList *children)
{
  return XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > 
make_element (Value (Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, const XMLElementList *children)
{
  return XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccReadAdaptor <Value, Parent> (getter), 
          XMLMemberTransferWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent> > 
make_element (Value (Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, const XMLElementList *children)
{
  return XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberReadAdaptor <Value, Parent>, XMLMemberWriteAdaptor <Value, Parent> > 
make_element (Value Parent::*member, const std::string &name, const XMLElementList *children)
{
  return XMLElement<Value, Parent, XMLMemberReadAdaptor <Value, Parent>, XMLMemberWriteAdaptor <Value, Parent> > ( 
          XMLMemberReadAdaptor <Value, Parent> (member), 
          XMLMemberWriteAdaptor <Value, Parent> (member), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Iter, class Parent>
XMLElement<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (const Value &), const std::string &name, const XMLElementList *children)
{
  return XMLElement<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          XMLMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Iter, class Parent>
XMLElement<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > 
make_element (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (Value *), const std::string &name, const XMLElementList *children)
{
  return XMLElement<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > ( 
          XMLMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end), 
          XMLMemberTransferWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element (void (Parent::*setter) (const Value &), const std::string &name, const XMLElementList &children)
{
  return XMLElement<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          XMLMemberDummyReadAdaptor <Value, Parent> (), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > 
make_element (void (Parent::*setter) (Value *), const std::string &name, const XMLElementList &children)
{
  return XMLElement<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > ( 
          XMLMemberDummyReadAdaptor <Value, Parent> (), 
          XMLMemberTransferWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, const XMLElementList &children)
{
  return XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > 
make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, const XMLElementList &children)
{
  return XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberTransferWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent> > 
make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, const XMLElementList &children)
{
  return XMLElement<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element (Value (Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, const XMLElementList &children)
{
  return XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > 
make_element (Value (Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, const XMLElementList &children)
{
  return XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccReadAdaptor <Value, Parent> (getter), 
          XMLMemberTransferWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent> > 
make_element (Value (Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, const XMLElementList &children)
{
  return XMLElement<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Parent>
XMLElement<Value, Parent, XMLMemberReadAdaptor <Value, Parent>, XMLMemberWriteAdaptor <Value, Parent> > 
make_element (Value Parent::*member, const std::string &name, const XMLElementList &children)
{
  return XMLElement<Value, Parent, XMLMemberReadAdaptor <Value, Parent>, XMLMemberWriteAdaptor <Value, Parent> > ( 
          XMLMemberReadAdaptor <Value, Parent> (member), 
          XMLMemberWriteAdaptor <Value, Parent> (member), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Iter, class Parent>
XMLElement<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (const Value &), const std::string &name, const XMLElementList &children)
{
  return XMLElement<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          XMLMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief Utility: create a XMLElement object
 */
template <class Value, class Iter, class Parent>
XMLElement<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > 
make_element (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (Value *), const std::string &name, const XMLElementList &children)
{
  return XMLElement<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > ( 
          XMLMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end), 
          XMLMemberTransferWriteAdaptor <Value, Parent> (setter), name, children); 
}

template <class Value, class Parent>
XMLElementWithParentRef<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element_with_parent_ref (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, const XMLElementList &children)
{
  return XMLElementWithParentRef<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

template <class Value, class Parent>
XMLElementWithParentRef<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element_with_parent_ref (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, const XMLElementList *children)
{
  return XMLElementWithParentRef<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

template <class Value, class Parent>
XMLElementWithParentRef<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > 
make_element_with_parent_ref (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, const XMLElementList &children)
{
  return XMLElementWithParentRef<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberTransferWriteAdaptor <Value, Parent> (setter), name, children); 
}

template <class Value, class Parent>
XMLElementWithParentRef<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > 
make_element_with_parent_ref (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, const XMLElementList *children)
{
  return XMLElementWithParentRef<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberTransferWriteAdaptor <Value, Parent> (setter), name, children); 
}

template <class Value, class Iter, class Parent>
XMLElementWithParentRef<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > 
make_element_with_parent_ref (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (const Value &), const std::string &name, const XMLElementList &children)
{
  return XMLElementWithParentRef<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent> > ( 
          XMLMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, children); 
}

template <class Value, class Iter, class Parent>
XMLElementWithParentRef<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > 
make_element_with_parent_ref (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (Value *), const std::string &name, const XMLElementList &children)
{
  return XMLElementWithParentRef<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberTransferWriteAdaptor <Value, Parent> > ( 
          XMLMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end), 
          XMLMemberTransferWriteAdaptor <Value, Parent> (setter), name, children); 
}

/**
 *  @brief A helper class providing string to value (and back) conversion
 */

template <class Value>
struct XMLStdConverter
{
  std::string to_string (const Value &v) const
  {
    return tl::to_string (v);
  }

  void from_string (const std::string &s, Value &v) const
  {
    tl::from_string (s, v);
  }
};

/**
 *  @brief Utility: create a XMLMember object without read & write capability
 */
template <class Parent>
XMLMember<std::string, Parent, XMLMemberDummyReadAdaptor <std::string, Parent>, XMLMemberDummyWriteAdaptor <std::string, Parent>, XMLStdConverter <std::string> > 
make_member (const std::string &name)
{
  return XMLMember<std::string, Parent, XMLMemberDummyReadAdaptor <std::string, Parent>, XMLMemberDummyWriteAdaptor <std::string, Parent>, XMLStdConverter <std::string> > ( 
          XMLMemberDummyReadAdaptor <std::string, Parent> (), 
          XMLMemberDummyWriteAdaptor <std::string, Parent> (), name); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent>
XMLMember<Value, Parent, XMLMemberReadAdaptor <Value, Parent>, XMLMemberWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > 
make_member (Value Parent::*member, const std::string &name)
{
  return XMLMember<Value, Parent, XMLMemberReadAdaptor <Value, Parent>, XMLMemberWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > ( 
          XMLMemberReadAdaptor <Value, Parent> (member), 
          XMLMemberWriteAdaptor <Value, Parent> (member), name); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent>
XMLMember<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > 
make_member (void (Parent::*setter) (const Value &), const std::string &name)
{
  return XMLMember<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > ( 
          XMLMemberDummyReadAdaptor <Value, Parent> (), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent>
XMLMember<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent>, XMLStdConverter <Value> >
make_member (void (Parent::*setter) (Value), const std::string &name)
{
  return XMLMember<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > (
          XMLMemberDummyReadAdaptor <Value, Parent> (),
          XMLMemberAccWriteAdaptor <Value, Parent> (setter), name);
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent>
XMLMember<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > 
make_member (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name)
{
  return XMLMember<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent>
XMLMember<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > 
make_member (Value (Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name)
{
  return XMLMember<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > ( 
          XMLMemberAccReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccWriteAdaptor <Value, Parent> (setter), name); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent>
XMLMember<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > 
make_member (const Value & (Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name)
{
  return XMLMember<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccWriteAdaptor <Value, Parent> (setter), name); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent>
XMLMember<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > 
make_member (Value (Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name)
{
  return XMLMember<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > ( 
          XMLMemberAccReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Iter, class Parent>
XMLMember<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > 
make_member (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (const Value &), const std::string &name)
{
  return XMLMember<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, XMLStdConverter <Value> > ( 
          XMLMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent, class Converter>
XMLMember<Value, Parent, XMLMemberReadAdaptor <Value, Parent>, XMLMemberWriteAdaptor <Value, Parent>, Converter> 
make_member (Value Parent::*member, const std::string &name, Converter conv)
{
  return XMLMember<Value, Parent, XMLMemberReadAdaptor <Value, Parent>, XMLMemberWriteAdaptor <Value, Parent>, Converter> ( 
          XMLMemberReadAdaptor <Value, Parent> (member), 
          XMLMemberWriteAdaptor <Value, Parent> (member), name, conv); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent, class Converter>
XMLMember<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, Converter> 
make_member (void (Parent::*setter) (const Value &), const std::string &name, Converter conv)
{
  return XMLMember<Value, Parent, XMLMemberDummyReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, Converter> ( 
          XMLMemberDummyReadAdaptor <Value, Parent> (), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, conv); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent, class Converter>
XMLMember<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberDummyWriteAdaptor <Value, Parent>, Converter> 
make_member (void (Parent::*setter) (Value), const std::string &name, Converter conv)
{
  return XMLMember<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberDummyWriteAdaptor <Value, Parent>, Converter> ( 
          XMLMemberAccReadAdaptor <Value, Parent> (setter), 
          XMLMemberDummyWriteAdaptor <Value, Parent> (), name, conv); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent, class Converter>
XMLMember<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, Converter> 
make_member (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, Converter conv)
{
  return XMLMember<Value, Parent, XMLMemberAccRefReadAdaptor <Value, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, Converter> ( 
          XMLMemberAccRefReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, conv); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Parent, class Converter>
XMLMember<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent>, Converter> 
make_member (Value (Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, Converter conv)
{
  return XMLMember<Value, Parent, XMLMemberAccReadAdaptor <Value, Parent>, XMLMemberAccWriteAdaptor <Value, Parent>, Converter> ( 
          XMLMemberAccReadAdaptor <Value, Parent> (getter), 
          XMLMemberAccWriteAdaptor <Value, Parent> (setter), name, conv); 
}

/**
 *  @brief Utility: create a XMLMember object
 */
template <class Value, class Iter, class Parent, class Converter>
XMLMember<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, Converter> 
make_member (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (const Value &), const std::string &name, Converter conv)
{
  return XMLMember<Value, Parent, XMLMemberIterReadAdaptor <const Value &, Iter, Parent>, XMLMemberAccRefWriteAdaptor <Value, Parent>, Converter> ( 
          XMLMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end), 
          XMLMemberAccRefWriteAdaptor <Value, Parent> (setter), name, conv); 
}

} // namespace tl

#endif

