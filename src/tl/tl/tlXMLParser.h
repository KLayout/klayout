
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "tlXMLReader.h"
#include "tlProtocolBuffer.h"
#include "tlAssert.h"
#include "tlInternational.h"
#include "tlString.h"
#include "tlStream.h"

class QIODevice;

namespace tl
{

/**
 *  NOTE: This XML parser package also supports a ProtocolBuffer flavor.
 *  This allows binding the same scheme to efficient binary PB format.
 */

class XMLReaderState;
class XMLReaderState;

// -----------------------------------------------------------------
//  The C++ structure definition interface (for use cases see tlXMLParser.ut)

class TL_PUBLIC XMLElementBase;

struct pass_by_value_tag { 
  pass_by_value_tag () { } 
};

struct pass_by_ref_tag { 
  pass_by_ref_tag () { } 
};

struct pb_zero_cardinality_tag {
  pb_zero_cardinality_tag () { }
};

struct pb_single_cardinality_tag {
  pb_single_cardinality_tag () { }
};

struct pb_many_cardinality_tag {
  pb_many_cardinality_tag () { }
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

  XMLElementList ();
  XMLElementList (const XMLElementBase &e);
  XMLElementList (XMLElementBase *e);
  XMLElementList (const std::string &name, const XMLElementList &d);
  XMLElementList (const XMLElementList &d);
  XMLElementList (const XMLElementList &d, const XMLElementBase &e);
  XMLElementList (const XMLElementList &d, XMLElementBase *e);

  void append (const XMLElementBase &e);
  void append (XMLElementBase *e);

  iterator begin () const;
  iterator end () const;

  static XMLElementList empty ();

  size_t oid () const
  {
    return m_oid;
  }

  const std::string &name () const
  {
    return m_name;
  }

private:
  std::list <XMLElementProxy> m_elements; 
  size_t m_oid;
  std::string m_name;
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
 *  @brief The PB parser class
 *  This is the main entry point. It will take a PB reader, the structure (in "root") and
 *  a reader state initialized with the top level object.
 */
class TL_PUBLIC PBParser
{
public:
  PBParser ();
  ~PBParser ();

  void parse (tl::ProtocolBufferReaderBase &reader, const XMLElementBase *root, XMLReaderState *reader_state);
  void parse_element (const XMLElementBase *parent, tl::ProtocolBufferReaderBase &reader);

  void expect_header (tl::ProtocolBufferReaderBase &reader, int name_tag, const std::string &name);

  XMLReaderState &reader_state ()
  {
    return *mp_state;
  }

private:
  XMLReaderState *mp_state;
};

/**
 *  @brief Helper class: A stack of const objects being written (PB binding)
 */

class TL_PUBLIC PBWriterState
{
public:
  /**
   *  @brief Default constructor
   */
  PBWriterState ();

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

  enum Cardinality {
    Zero, Single, Many
  };

  XMLElementBase (const std::string &name, const XMLElementList &children);
  XMLElementBase (const std::string &name, const XMLElementList *children);
  XMLElementBase (const XMLElementBase &d);
  virtual ~XMLElementBase ();

  virtual XMLElementBase *clone () const = 0;

  virtual void create (const XMLElementBase *parent, XMLReaderState &objs, const std::string &uri, const std::string &lname, const std::string &qname) const = 0;
  virtual void cdata (const std::string &cdata, XMLReaderState &objs) const = 0;
  virtual void finish (const XMLElementBase *parent, XMLReaderState &objs, const std::string &uri, const std::string &lname, const std::string &qname) const = 0;

  virtual void write (const XMLElementBase * /*parent*/, tl::OutputStream & /*os*/, int /*indent*/, XMLWriterState & /*objs*/) const { }
  virtual bool has_any (XMLWriterState & /*objs*/) const { return false; }

  static void write_indent (tl::OutputStream &os, int indent);
  static void write_string (tl::OutputStream &os, const std::string &s);

  virtual void pb_create (const XMLElementBase *parent, XMLReaderState &objs) const = 0;
  virtual void pb_parse (PBParser *, tl::ProtocolBufferReaderBase &) const = 0;
  virtual void pb_finish (const XMLElementBase *parent, XMLReaderState &objs) const = 0;

  virtual void pb_write (const XMLElementBase *, tl::ProtocolBufferWriterBase &, PBWriterState &) const { }

  virtual Cardinality cardinality () const;

  size_t oid () const
  {
    return mp_children->oid ();
  }

  int tag () const
  {
    return m_tag;
  }

  const std::string &name () const
  {
    return m_name;
  }

  bool check_name (const std::string & /*uri*/, const std::string &lname, const std::string & /*qname*/) const;

  /**
   *  @brief Returns a name suitable for code
   *  Specifically, hyphens are replaced by underscores.
   */
  std::string name4code () const;

  iterator begin () const;
  iterator end () const;

  std::string create_def (std::map<size_t, std::pair <const XMLElementBase *, std::string> > &messages) const;

protected:
  virtual void collect_messages (std::map<size_t, std::pair <const XMLElementBase *, std::string> > &messages) const;
  virtual std::string create_def_entry (std::map<size_t, std::pair <const XMLElementBase *, std::string> > &messages) const = 0;

  std::string make_message_name () const;

  static Cardinality get_cardinality (tl::pb_zero_cardinality_tag)
  {
    return Zero;
  }

  static Cardinality get_cardinality (tl::pb_single_cardinality_tag)
  {
    return Single;
  }

  static Cardinality get_cardinality (tl::pb_many_cardinality_tag)
  {
    return Many;
  }

private:
  std::string m_name;
  int m_tag;
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

  virtual void pb_create (const XMLElementBase *, XMLReaderState &objs) const
  {
    XMLObjTag<Obj> tag;
    objs.push (tag);
  }

  virtual void pb_finish (const XMLElementBase * /*parent*/, XMLReaderState &objs) const
  {
    XMLObjTag<Obj> tag;
    XMLObjTag<Parent> parent_tag;
    m_w (*objs.parent (parent_tag), objs);
    objs.pop (tag);
  }

  virtual void pb_parse (PBParser *parser, tl::ProtocolBufferReaderBase &reader) const
  {
    reader.open ();
    parser->parse_element (this, reader);
    reader.close ();
  }

  virtual void pb_write (const XMLElementBase * /*parent*/, tl::ProtocolBufferWriterBase &writer, PBWriterState &objs) const
  {
    XMLObjTag<Parent> parent_tag;

    Read r (m_r);
    r.start (*objs.back (parent_tag));
    while (! r.at_end ()) {
      typedef typename Read::tag read_tag_type;
      pb_write_obj (r (), tag (), writer, read_tag_type (), objs);
      r.next ();
    }
  }

  virtual Cardinality cardinality () const
  {
    typedef typename Read::cardinality cardinality_type;
    return get_cardinality (cardinality_type ());
  }

  virtual void collect_messages (std::map<size_t, std::pair <const XMLElementBase *, std::string> > &messages) const
  {
    if (messages.find (oid ()) == messages.end ()) {
      messages [oid ()] = std::make_pair (this, make_message_name ());
      XMLElementBase::collect_messages (messages);
    }
  }

  virtual std::string create_def_entry (std::map<size_t, std::pair <const XMLElementBase *, std::string> > &messages) const
  {
    auto m = messages.find (oid ());
    if (m != messages.end ()) {
      return m->second.second + " " + name4code () + " = " + tl::to_string (tag ()) + ";";
    } else {
      return std::string ();
    }
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

  //  this write helper is used if the reader delivers an object by value
  void pb_write_obj (Obj obj, int tag, tl::ProtocolBufferWriterBase &writer, tl::pass_by_value_tag, PBWriterState &objs) const
  {
    XMLObjTag<Obj> self_tag;

    for (unsigned int pass = 0; pass < 2; ++pass) {
      writer.begin_seq (tag, pass == 0);
      objs.push (&obj);
      for (XMLElementBase::iterator c = this->begin (); c != this->end (); ++c) {
        c->get ()->write (this, writer, objs);
      }
      objs.pop (self_tag);
      writer.end_seq ();
      if (! writer.is_counting ()) {
        break;
      }
    }
  }

  void pb_write_obj (const Obj &obj, int tag, tl::ProtocolBufferWriterBase &writer, tl::pass_by_ref_tag, PBWriterState &objs) const
  {
    XMLObjTag<Obj> self_tag;

    for (unsigned int pass = 0; pass < 2; ++pass) {
      writer.begin_seq (tag, pass == 0);
      objs.push (&obj);
      for (XMLElementBase::iterator c = this->begin (); c != this->end (); ++c) {
        c->get ()->pb_write (this, writer, objs);
      }
      objs.pop (self_tag);
      writer.end_seq ();
      if (writer.is_counting ()) {
        break;
      }
    }
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
  : public XMLElement<Obj, Parent, Read, Write>
{
public:
  XMLElementWithParentRef (const Read &r, const Write &w, const std::string &name, const XMLElementList &children)
    : XMLElement<Obj, Parent, Read, Write> (r, w, name, children)
  {
    // .. nothing yet ..
  }

  XMLElementWithParentRef (const Read &r, const Write &w, const std::string &name, const XMLElementList *children)
    : XMLElement<Obj, Parent, Read, Write> (r, w, name, children)
  {
    // .. nothing yet ..
  }

  XMLElementWithParentRef (const XMLElementWithParentRef<Obj, Parent, Read, Write> &d)
    : XMLElement<Obj, Parent, Read, Write> (d)
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

  virtual void finish (const XMLElementBase * /*parent*/, XMLReaderState &objs, const std::string & /*uri*/, const std::string & /*lname*/, const std::string & /*qname*/) const
  {
    XMLObjTag<Obj> tag;
    XMLObjTag<Parent> parent_tag;
    m_w (*objs.parent (parent_tag), objs);
    objs.pop (tag);
  }

  virtual void pb_create (const XMLElementBase *, XMLReaderState &objs) const
  {
    XMLObjTag<Obj> tag;
    XMLObjTag<Parent> parent_tag;
    objs.push (new Obj (objs.back (parent_tag)), true);
  }

  virtual void pb_finish (const XMLElementBase * /*parent*/, XMLReaderState &objs) const
  {
    XMLObjTag<Obj> tag;
    XMLObjTag<Parent> parent_tag;
    m_w (*objs.parent (parent_tag), objs);
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

  virtual void pb_create (const XMLElementBase *, XMLReaderState &) const
  {
    // .. nothing yet ..
  }

  virtual void pb_finish (const XMLElementBase *, XMLReaderState &) const
  {
    // .. nothing yet ..
  }

  virtual void pb_parse (PBParser *parser, tl::ProtocolBufferReaderBase &reader) const
  {
    XMLObjTag<Value> tag;
    XMLObjTag<Parent> parent_tag;

    XMLReaderState value_obj;
    value_obj.push (tag);

    read (reader, *value_obj.back (tag));
    m_w (*parser->reader_state ().back (parent_tag), value_obj);

    value_obj.pop (tag);
  }

  virtual void pb_write (const XMLElementBase * /*parent*/, tl::ProtocolBufferWriterBase &writer, PBWriterState &objs) const
  {
    XMLObjTag<Parent> parent_tag;
    Read r (m_r);
    r.start (* objs.back (parent_tag));
    while (! r.at_end ()) {
      write (writer, tag (), r ());
      r.next ();
    }
  }

  virtual Cardinality cardinality () const
  {
    typedef typename Read::cardinality cardinality_type;
    return get_cardinality (cardinality_type ());
  }

  virtual void collect_messages (std::map<size_t, std::pair <const XMLElementBase *, std::string> > & /*messages*/) const
  {
    //  no messages here.
  }

  virtual std::string create_def_entry (std::map<size_t, std::pair <const XMLElementBase *, std::string> > & /*messages*/) const
  {
    const Value *v = 0;
    return typestring (v) + " " + name4code () + " = " + tl::to_string (tag ()) + ";";
  }

private:
  Read m_r;
  Write m_w;
  Converter m_c;

  //  write incarnations
  void write (tl::ProtocolBufferWriterBase &writer, int tag, float v) const
  {
    writer.write (tag, v);
  }

  void write (tl::ProtocolBufferWriterBase &writer, int tag, double v) const
  {
    writer.write (tag, v);
  }

  void write (tl::ProtocolBufferWriterBase &writer, int tag, uint8_t v) const
  {
    writer.write (tag, (uint32_t) v);
  }

  void write (tl::ProtocolBufferWriterBase &writer, int tag, int8_t v) const
  {
    writer.write (tag, (int32_t) v);
  }

  void write (tl::ProtocolBufferWriterBase &writer, int tag, uint16_t v) const
  {
    writer.write (tag, (uint32_t) v);
  }

  void write (tl::ProtocolBufferWriterBase &writer, int tag, int16_t v) const
  {
    writer.write (tag, (int32_t) v);
  }

  void write (tl::ProtocolBufferWriterBase &writer, int tag, uint32_t v) const
  {
    writer.write (tag, v);
  }

  void write (tl::ProtocolBufferWriterBase &writer, int tag, int32_t v) const
  {
    writer.write (tag, v);
  }

  void write (tl::ProtocolBufferWriterBase &writer, int tag, uint64_t v) const
  {
    writer.write (tag, v);
  }

  void write (tl::ProtocolBufferWriterBase &writer, int tag, int64_t v) const
  {
    writer.write (tag, v);
  }

  void write (tl::ProtocolBufferWriterBase &writer, int tag, bool v) const
  {
    writer.write (tag, v);
  }

  void write (tl::ProtocolBufferWriterBase &writer, int tag, const std::string &v) const
  {
    writer.write (tag, v);
  }

  template <class T>
  void write (tl::ProtocolBufferWriterBase &writer, int tag, const T &v) const
  {
    writer.write (tag, m_c.pb_encode (v));
  }

  //  read incarnations
  void read (tl::ProtocolBufferReaderBase &reader, float &v) const
  {
    reader.read (v);
  }

  void read (tl::ProtocolBufferReaderBase &reader, double &v) const
  {
    reader.read (v);
  }

  void read (tl::ProtocolBufferReaderBase &reader, uint8_t &v) const
  {
    uint32_t vv = 0;
    reader.read (vv);
    //  TODO: check for overflow?
    v = vv;
  }

  void read (tl::ProtocolBufferReaderBase &reader, int8_t &v) const
  {
    int32_t vv = 0;
    reader.read (vv);
    //  TODO: check for overflow?
    v = vv;
  }

  void read (tl::ProtocolBufferReaderBase &reader, uint16_t &v) const
  {
    uint32_t vv = 0;
    reader.read (vv);
    //  TODO: check for overflow?
    v = vv;
  }

  void read (tl::ProtocolBufferReaderBase &reader, int16_t &v) const
  {
    int32_t vv = 0;
    reader.read (vv);
    //  TODO: check for overflow?
    v = vv;
  }

  void read (tl::ProtocolBufferReaderBase &reader, uint32_t &v) const
  {
    reader.read (v);
  }

  void read (tl::ProtocolBufferReaderBase &reader, int32_t &v) const
  {
    reader.read (v);
  }

  void read (tl::ProtocolBufferReaderBase &reader, uint64_t &v) const
  {
    reader.read (v);
  }

  void read (tl::ProtocolBufferReaderBase &reader, int64_t &v) const
  {
    reader.read (v);
  }

  void read (tl::ProtocolBufferReaderBase &reader, bool &v) const
  {
    reader.read (v);
  }

  void read (tl::ProtocolBufferReaderBase &reader, std::string &v) const
  {
    reader.read (v);
  }

  template <class T>
  void read (tl::ProtocolBufferReaderBase &reader, T &v) const
  {
    typename Converter::pb_type vv;
    reader.read (vv);
    m_c.pb_decode (vv, v);
  }

  //  type strings
  std::string typestring (const float *) const
  {
    return "float";
  }

  std::string typestring (const double *) const
  {
    return "double";
  }

  std::string typestring (const uint8_t *) const
  {
    return "uint32";
  }

  std::string typestring (const uint16_t *) const
  {
    return "uint32";
  }

  std::string typestring (const uint32_t *) const
  {
    return "uint32";
  }

  std::string typestring (const uint64_t *) const
  {
    return "uint64";
  }

  std::string typestring (const int8_t *) const
  {
    return "sint32";
  }

  std::string typestring (const int16_t *) const
  {
    return "sint32";
  }

  std::string typestring (const int32_t *) const
  {
    return "sint32";
  }

  std::string typestring (const int64_t *) const
  {
    return "sint64";
  }

  std::string typestring (const bool *) const
  {
    return "uint32";
  }

  std::string typestring (const std::string *) const
  {
    return "string";
  }

  template <class T>
  std::string typestring (const T *) const
  {
    const typename Converter::pb_type *v = 0;
    return typestring (v);
  }
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

  /**
   *  @brief Serializes the given object (root) to the writer
   */
  void write (tl::ProtocolBufferWriterBase &writer, const Obj &root) const
  {
    PBWriterState writer_state;
    writer_state.push (& root);

    writer.write (tag (), name ());

    for (XMLElementBase::iterator c = this->begin (); c != this->end (); ++c) {
      c->get ()->pb_write (this, writer, writer_state);
    }
  }

  /**
   *  @brief Deserializes the given object (root) from the reader
   */
  void parse (tl::ProtocolBufferReaderBase &reader, Obj &root) const
  {
    XMLObjTag<Obj> self_tag;
    XMLReaderState rs;
    rs.push (&root);
    PBParser h;
    h.expect_header (reader, tag (), name ());
    h.parse (reader, this, &rs);
    rs.pop (self_tag);
    tl_assert (rs.empty ());
  }

  /**
   *  @brief Produces a definition for the protoc compiler
   */
  std::string create_def () const
  {
    std::map<size_t, std::pair <const XMLElementBase *, std::string> > msgs;
    collect_messages (msgs);
    msgs[oid ()] = std::make_pair (this, make_message_name ());

    std::string res = "// created from KLayout proto definition '" + name () + "'\n\n";
    res += "syntax = \"proto2\";";

    for (auto i = msgs.begin (); i != msgs.end (); ++i) {
      std::string entry = i->second.first->create_def (msgs);
      if (! entry.empty ()) {
        res += "\n";
        res += "\n";
        res += entry;
      }
    }

    return res;
  }

private:
  virtual void write (const XMLElementBase*, tl::OutputStream &, int, XMLWriterState &) const
  {
    // .. see write (os)
  }

  virtual void pb_write (const XMLElementBase*, tl::ProtocolBufferWriterBase &, PBWriterState &) const
  {
    // disable base class implementation
  }

  virtual void pb_parse (PBParser *, tl::ProtocolBufferReaderBase &) const
  {
    // disable base class implementation
  }

  virtual void pb_create (const XMLElementBase *, XMLReaderState &) const
  {
    // disable base class implementation
  }

  virtual void pb_finish (const XMLElementBase *, XMLReaderState &) const
  {
    // disable base class implementation
  }

  virtual std::string create_def_entry (std::map<size_t, std::pair<const XMLElementBase *, std::string> > &) const
  {
    return std::string ();
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
  typedef pb_zero_cardinality_tag cardinality;

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
  typedef pb_single_cardinality_tag cardinality;

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
  typedef pb_single_cardinality_tag cardinality;

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
  typedef pb_single_cardinality_tag cardinality;

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
  typedef pb_many_cardinality_tag cardinality;

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

