
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

#ifndef HDR_tlProtocolBufferStruct
#define HDR_tlProtocolBufferStruct

#include "tlCommon.h"
#include "tlProtocolBuffer.h"

#include <map>

namespace tl
{

/**
 *  @brief A basic PB parser error exception class
 */

class TL_PUBLIC PBException : public tl::Exception
{
public:
  PBException (const char *msg)
    : Exception (tl::to_string (tr ("Protocol buffer parser error: %s")).c_str ()),
      m_msg (msg)
  {
    //  .. nothing yet ..
  }

  PBException (const std::string &msg)
    : Exception (fmt (0).c_str (), msg.c_str ()),
      m_msg (msg)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Raw (unprefixed) message of the PB parser
   */
  const std::string &
  raw_msg () const
  {
    return m_msg;
  }

protected:
  PBException (const std::string &msg, size_t pos)
    : Exception (fmt (pos).c_str (), msg.c_str (), pos),
      m_msg (msg)
  {
    //  .. nothing yet ..
  }

private:
  std::string m_msg;

  static std::string fmt (size_t pos)
  {
    if (pos == 0) {
      return tl::to_string (tr ("Protocol buffer parser error: %s")).c_str ();
    } else {
      return tl::to_string (tr ("Protocol buffer parser error: %s at position %ul")).c_str ();
    }
  }
};

/**
 *  @brief A PB parser error exception class that additionally provides line and column information
 */

class TL_PUBLIC PBLocatedException : public PBException
{
public:
  PBLocatedException (const std::string &msg, size_t pos)
    : PBException (msg, pos),
      m_pos (pos)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Line number information of the exception
   */
  size_t pos () const
  {
    return m_pos;
  }

private:
  size_t m_pos;
};

/**
 *  @brief An object wrapper base class for target object management
 *
 *  Implementations of this class through the PBReaderProxy templates
 *  manage pointers to certain objects.
 */

class TL_PUBLIC PBReaderProxyBase
{
public:
  PBReaderProxyBase () { }
  virtual ~PBReaderProxyBase () { }
  virtual void release () = 0;
  virtual void detach () = 0;
};

/**
 *  @brief An object wrapper base class for target object management specialized to a certain class
 */

template <class Obj>
class TL_PUBLIC_TEMPLATE PBReaderProxy
  : public PBReaderProxyBase
{
public:
  PBReaderProxy (Obj *obj, bool owns_obj)
    : mp_obj (obj), m_owns_obj (owns_obj)
  { }

  virtual ~PBReaderProxy () { }

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
struct PBObjTag
{
  PBObjTag() { }
  typedef Obj obj;
};

/**
 *  @brief Helper class: The reader state
 *
 *  The reader state mainly consists of a stack of objects being parsed.
 */

class TL_PUBLIC PBReaderState
{
public:
  /**
   *  @brief Default constructor
   */
  PBReaderState ();

  /**
   *  @brief Destructor
   */
  ~PBReaderState ();

  /**
   *  @brief Push a new object on the stack
   */
  template <class Obj>
  void push (PBObjTag<Obj> /*tag*/)
  {
    m_objects.push_back (new PBReaderProxy<Obj> (new Obj (), true));
  }

  /**
   *  @brief Push an existing object on the stack
   */
  template <class Obj>
  void push (Obj *obj)
  {
    m_objects.push_back (new PBReaderProxy<Obj> (obj, false));
  }

  /**
   *  @brief Push an existing object on the stack with the ownership flag
   */
  template <class Obj>
  void push (Obj *obj, bool owner)
  {
    m_objects.push_back (new PBReaderProxy<Obj> (obj, owner));
  }

  /**
   *  @brief Get the top object
   */
  template <class Obj>
  Obj *back (PBObjTag<Obj> /*tag*/)
  {
    tl_assert (! m_objects.empty ());
    return (dynamic_cast <PBReaderProxy<Obj> &> (*m_objects.back ())).ptr ();
  }

  /**
   *  @brief Get the top object and release
   */
  template <class Obj>
  Obj *detach_back (PBObjTag<Obj> /*tag*/)
  {
    tl_assert (! m_objects.empty ());
    m_objects.back ()->detach ();
    return (dynamic_cast <PBReaderProxy<Obj> &> (*m_objects.back ())).ptr ();
  }

  /**
   *  @brief Pop an object from the stack
   */
  template <class Obj>
  void pop (PBObjTag<Obj> /*tag*/)
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
  Obj *parent (PBObjTag<Obj> /*tag*/)
  {
    tl_assert (m_objects.size () > 1);
    return (dynamic_cast <PBReaderProxy<Obj> &> (*m_objects.end () [-2])).ptr ();
  }

private:
  std::vector <PBReaderProxyBase *> m_objects;
};

// -----------------------------------------------------------------
//  The C++ structure definition interface (for use cases see tlProtocolBufferStruct.ut)

class TL_PUBLIC PBElementBase;

struct pb_pass_by_value_tag {
  pb_pass_by_value_tag () { }
};

struct pb_pass_by_ref_tag {
  pb_pass_by_ref_tag () { }
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
 *  @brief The PB parser class
 *  This is the main entry point. It will take a PB reader, the structure (in "root") and
 *  a reader state initialized with the top level object.
 */
class TL_PUBLIC PBParser
{
public:
  PBParser ();
  ~PBParser ();

  void parse (tl::ProtocolBufferReaderBase &reader, const PBElementBase *root, PBReaderState *reader_state);
  void parse_element (const PBElementBase *parent, tl::ProtocolBufferReaderBase &reader);

  void expect_header (tl::ProtocolBufferReaderBase &reader, int name_tag, const std::string &name);

  PBReaderState &reader_state ()
  {
    return *mp_state;
  }

private:
  PBReaderState *mp_state;
};

/**
 *  @brief PBElementProxy element of the PB structure definition
 *
 *  The purpose of this class is to provide a wrapper around the
 *  different derivations of the PBElementBase class, so we can
 *  pack them into a vector. The proxy objects can be copied
 *  duplicating the wrapped objects with their "clone" methods.
 *  The proxy object can be given the pointer to the element
 *  in two ways: the reference version creates a duplicate, the
 *  pointer version transfers the ownership of the PBElementBase
 *  object.
 */

class TL_PUBLIC PBElementProxy
{
public:
  PBElementProxy (const PBElementProxy &d);
  PBElementProxy (const PBElementBase &d);
  PBElementProxy (PBElementBase *d);

  ~PBElementProxy ();

  PBElementBase *operator-> () const
  {
    return mp_ptr;
  }

  PBElementBase *get () const
  {
    return mp_ptr;
  }

private:
  PBElementBase *mp_ptr;
};

/**
 *  @brief A list of PB elements
 *
 *  This class provides a list of PB elements.
 *  A list can be created by using the + operator and
 *  supplying various objects based on PBElementBase.
 */

class TL_PUBLIC PBElementList
{
public:
  typedef std::list <PBElementProxy> children_list;
  typedef children_list::const_iterator iterator;

  PBElementList ();
  PBElementList (const PBElementBase &e);
  PBElementList (PBElementBase *e);
  PBElementList (const std::string &name, const PBElementList &d);
  PBElementList (const PBElementList &d);
  PBElementList (const PBElementList &d, const PBElementBase &e);
  PBElementList (const PBElementList &d, PBElementBase *e);

  void append (const PBElementBase &e);
  void append (PBElementBase *e);

  iterator begin () const;
  iterator end () const;

  static PBElementList empty ();

  size_t oid () const
  {
    return m_oid;
  }

  const std::string &name () const
  {
    return m_name;
  }

private:
  std::list <PBElementProxy> m_elements;
  size_t m_oid;
  std::string m_name;
};

/**
 *  @brief Helper class: A stack of const objects being written
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
  const Obj *pop (PBObjTag<Obj> /*tag*/)
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
  const Obj *back (PBObjTag<Obj> /*tag*/)
  {
    tl_assert (m_objects.size () > 0);
    return reinterpret_cast <const Obj *> (m_objects.end () [-1]);
  }

private:
  std::vector <const void *> m_objects;
};

/**
 *  @brief The PB element base object
 *
 *  This class is the base class for objects implementing
 *  the parser handler semantics. The basic methods are
 *  create (create an actual object), read (read value),
 *  finish (finalize the actual object).
 *
 *  Writing of objects is also handled through this class.
 *  The basic method is "write".
 */

class TL_PUBLIC PBElementBase
{
public:
  typedef PBElementList::iterator iterator;

  enum Cardinality {
    Zero, Single, Many
  };

  PBElementBase (const std::string &name, int tag, const PBElementList &children);
  PBElementBase (const std::string &name, int tag, const PBElementList *children);
  PBElementBase (const PBElementBase &d);
  virtual ~PBElementBase ();

  virtual PBElementBase *clone () const = 0;

  virtual void create (const PBElementBase *parent, PBReaderState &objs) const = 0;
  virtual void parse (PBParser *, tl::ProtocolBufferReaderBase &) const = 0;
  virtual void finish (const PBElementBase *parent, PBReaderState &objs) const = 0;

  virtual void write (const PBElementBase *, tl::ProtocolBufferWriterBase &, PBWriterState &) const { }

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

  /**
   *  @brief Returns a name suitable for code
   *  Specifically, hyphens are replaced by underscores.
   */
  std::string name4code () const;

  iterator begin () const;
  iterator end () const;

  std::string create_def (std::map<size_t, std::pair <const PBElementBase *, std::string> > &messages) const;

protected:
  virtual void collect_messages (std::map<size_t, std::pair <const PBElementBase *, std::string> > &messages) const;
  virtual std::string create_def_entry (std::map<size_t, std::pair <const PBElementBase *, std::string> > &messages) const = 0;

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
  const PBElementList *mp_children;
  bool m_owns_child_list;
};

/**
 *  @brief A PB child element
 *
 *  This class is a PB structure component describing a child
 *  element in the PB tree. There is no limit about the number of
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
class TL_PUBLIC_TEMPLATE PBElement
  : public PBElementBase
{
public:
  PBElement (const Read &r, const Write &w, const std::string &name, int tag, const PBElementList &children)
    : PBElementBase (name, tag, children), m_r (r), m_w (w)
  {
    // .. nothing yet ..
  }

  PBElement (const Read &r, const Write &w, const std::string &name, int tag, const PBElementList *children)
    : PBElementBase (name, tag, children), m_r (r), m_w (w)
  {
    // .. nothing yet ..
  }

  PBElement (const PBElement<Obj, Parent, Read, Write> &d)
    : PBElementBase (d), m_r (d.m_r), m_w (d.m_w)
  {
    // .. nothing yet ..
  }

  virtual PBElementBase *clone () const
  {
    return new PBElement<Obj, Parent, Read, Write> (*this);
  }

  virtual void create (const PBElementBase *, PBReaderState &objs) const
  {
    PBObjTag<Obj> tag;
    objs.push (tag);
  }

  virtual void finish (const PBElementBase * /*parent*/, PBReaderState &objs) const
  {
    PBObjTag<Obj> tag;
    PBObjTag<Parent> parent_tag;
    m_w (*objs.parent (parent_tag), objs);
    objs.pop (tag);
  }

  virtual void parse (PBParser *parser, tl::ProtocolBufferReaderBase &reader) const
  {
    reader.open ();
    parser->parse_element (this, reader);
    reader.close ();
  }

  virtual void write (const PBElementBase * /*parent*/, tl::ProtocolBufferWriterBase &writer, PBWriterState &objs) const
  {
    PBObjTag<Parent> parent_tag;

    Read r (m_r);
    r.start (*objs.back (parent_tag));
    while (! r.at_end ()) {
      typedef typename Read::tag read_tag_type;
      write_obj (r (), tag (), writer, read_tag_type (), objs);
      r.next ();
    }
  }

  virtual Cardinality cardinality () const
  {
    typedef typename Read::cardinality cardinality_type;
    return get_cardinality (cardinality_type ());
  }

  virtual void collect_messages (std::map<size_t, std::pair <const PBElementBase *, std::string> > &messages) const
  {
    if (messages.find (oid ()) == messages.end ()) {
      messages [oid ()] = std::make_pair (this, make_message_name ());
      PBElementBase::collect_messages (messages);
    }
  }

  virtual std::string create_def_entry (std::map<size_t, std::pair <const PBElementBase *, std::string> > &messages) const
  {
    auto m = messages.find (oid ());
    if (m != messages.end ()) {
      return m->second.second + " " + name4code () + " = " + tl::to_string (tag ()) + ";";
    } else {
      return std::string ();
    }
  }

protected:
  Read &rear () { return m_r; }
  Write &write () { return m_w; }

private:
  Read m_r;
  Write m_w;

  //  this write helper is used if the reader delivers an object by value
  void write_obj (Obj obj, int tag, tl::ProtocolBufferWriterBase &writer, tl::pb_pass_by_value_tag, PBWriterState &objs) const
  {
    PBObjTag<Obj> self_tag;

    for (unsigned int pass = 0; pass < 2; ++pass) {
      writer.begin_seq (tag, pass == 0);
      objs.push (&obj);
      for (PBElementBase::iterator c = this->begin (); c != this->end (); ++c) {
        c->get ()->write (this, writer, objs);
      }
      objs.pop (self_tag);
      writer.end_seq ();
      if (! writer.is_counting ()) {
        break;
      }
    }
  }

  void write_obj (const Obj &obj, int tag, tl::ProtocolBufferWriterBase &writer, tl::pb_pass_by_ref_tag, PBWriterState &objs) const
  {
    PBObjTag<Obj> self_tag;

    for (unsigned int pass = 0; pass < 2; ++pass) {
      writer.begin_seq (tag, pass == 0);
      objs.push (&obj);
      for (PBElementBase::iterator c = this->begin (); c != this->end (); ++c) {
        c->get ()->write (this, writer, objs);
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
 *  @brief A PB child element with is instantiated with a parent reference
 *
 *  This declares a PB element with a constructor that takes a (object *) arguments
 *  to the parent.
 */

template <class Obj, class Parent, class Read, class Write>
class TL_PUBLIC_TEMPLATE PBElementWithParentRef
  : public PBElement<Obj, Parent, Read, Write>
{
public:
  PBElementWithParentRef (const Read &r, const Write &w, const std::string &name, int tag, const PBElementList &children)
    : PBElement<Obj, Parent, Read, Write> (r, w, name, tag, children)
  {
    // .. nothing yet ..
  }

  PBElementWithParentRef (const Read &r, const Write &w, const std::string &name, int tag, const PBElementList *children)
    : PBElement<Obj, Parent, Read, Write> (r, w, name, tag, children)
  {
    // .. nothing yet ..
  }

  PBElementWithParentRef (const PBElementWithParentRef<Obj, Parent, Read, Write> &d)
    : PBElement<Obj, Parent, Read, Write> (d)
  {
    // .. nothing yet ..
  }

  virtual PBElementBase *clone () const
  {
    return new PBElementWithParentRef<Obj, Parent, Read, Write> (*this);
  }

  virtual void create (const PBElementBase *, PBReaderState &objs) const
  {
    PBObjTag<Obj> tag;
    PBObjTag<Parent> parent_tag;
    objs.push (new Obj (objs.back (parent_tag)), true);
  }

  virtual void finish (const PBElementBase * /*parent*/, PBReaderState &objs) const
  {
    PBObjTag<Obj> tag;
    PBObjTag<Parent> parent_tag;
    m_w (*objs.parent (parent_tag), objs);
    objs.pop (tag);
  }
};

/**
 *  @brief A PB child element mapped to a member
 *
 *  This class is a PB structure component describing a child
 *  element in the PB tree. The value of the child element
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
class TL_PUBLIC_TEMPLATE PBMember
  : public PBElementBase
{
public:
  PBMember (const Read &r, const Write &w, const std::string &name, int tag, Converter c = Converter ())
    : PBElementBase (name, tag, PBElementList::empty ()), m_r (r), m_w (w), m_c (c)
  {
    // .. nothing yet ..
  }

  PBMember (const PBMember<Value, Parent, Read, Write, Converter> &d)
    : PBElementBase (d), m_r (d.m_r), m_w (d.m_w), m_c (d.m_c)
  {
    // .. nothing yet ..
  }

  virtual PBElementBase *clone () const
  {
    return new PBMember<Value, Parent, Read, Write, Converter> (*this);
  }

  virtual void create (const PBElementBase *, PBReaderState &) const
  {
    // .. nothing yet ..
  }

  virtual void finish (const PBElementBase *, PBReaderState &) const
  {
    // .. nothing yet ..
  }

  virtual void parse (PBParser *parser, tl::ProtocolBufferReaderBase &reader) const
  {
    PBObjTag<Value> tag;
    PBObjTag<Parent> parent_tag;

    PBReaderState value_obj;
    value_obj.push (tag);

    read (reader, *value_obj.back (tag));
    m_w (*parser->reader_state ().back (parent_tag), value_obj);

    value_obj.pop (tag);
  }

  virtual void write (const PBElementBase * /*parent*/, tl::ProtocolBufferWriterBase &writer, PBWriterState &objs) const
  {
    PBObjTag<Parent> parent_tag;
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

  virtual void collect_messages (std::map<size_t, std::pair <const PBElementBase *, std::string> > & /*messages*/) const
  {
    //  no messages here.
  }

  virtual std::string create_def_entry (std::map<size_t, std::pair <const PBElementBase *, std::string> > & /*messages*/) const
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
 *  @brief The root element of the PB structure
 *
 *  The root object is also a element node. It belongs to an object
 *  representing the root of the object hierarchy. Correspondingly,
 *  child elements (members, elements - aka submessages) need to be
 *  specified.
 *
 *  In addition, the root element has a name and a name tag.
 *  The name / name tag are emitted as the first element in the
 *  message, providing a file header with fixed content. This
 *  allows identifying the file as a encoded protocol buffer
 *  file for a specific object type.
 */

template <class Obj>
class TL_PUBLIC_TEMPLATE PBStruct
  : public PBElementBase
{
public:
  PBStruct (const std::string &name, int name_tag, const PBElementList *children)
    : PBElementBase (name, name_tag, children)
  {
    // .. nothing yet ..
  }

  PBStruct (const std::string &name, int name_tag, const PBElementList &children)
    : PBElementBase (name, name_tag, children)
  {
    // .. nothing yet ..
  }

  PBStruct (const PBStruct<Obj> &d)
    : PBElementBase (d)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Creates a deep copy of the structure
   */
  virtual PBElementBase *clone () const
  {
    return new PBStruct<Obj> (*this);
  }

  /**
   *  @brief Serializes the given object (root) to the writer
   */
  void write (tl::ProtocolBufferWriterBase &writer, const Obj &root) const
  {
    PBWriterState writer_state;
    writer_state.push (& root);

    writer.write (tag (), name ());

    for (PBElementBase::iterator c = this->begin (); c != this->end (); ++c) {
      c->get ()->write (this, writer, writer_state);
    }
  }

  /**
   *  @brief Deserializes the given object (root) from the reader
   */
  void parse (tl::ProtocolBufferReaderBase &reader, Obj &root) const
  {
    PBObjTag<Obj> self_tag;
    PBReaderState rs;
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
    std::map<size_t, std::pair <const PBElementBase *, std::string> > msgs;
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
  virtual void write (const PBElementBase*, tl::ProtocolBufferWriterBase &, PBWriterState &) const
  {
    // disable base class implementation
  }

  virtual void parse (PBParser *, tl::ProtocolBufferReaderBase &) const
  {
    // disable base class implementation
  }

  virtual void create (const PBElementBase *, PBReaderState &) const
  {
    // disable base class implementation
  }

  virtual void finish (const PBElementBase *, PBReaderState &) const
  {
    // disable base class implementation
  }

  virtual std::string create_def_entry (std::map<size_t, std::pair<const PBElementBase *, std::string> > &) const
  {
    return std::string ();
  }
};

/**
 *  @brief Utility: add a PB element to a list of some
 */
inline PBElementList
operator+ (const PBElementList &l, const PBElementBase &e)
{
  return PBElementList (l, e);
}

/**
 *  @brief Utility: add a PB element to a list of some
 *
 *  This version transfers the ownership of the PBElementBase object.
 */
inline PBElementList
operator+ (const PBElementList &l, PBElementBase *e)
{
  return PBElementList (l, e);
}


//  Several adaptor classes needed to implement the various make_* utilities

template <class Value, class Parent>
struct PBMemberDummyWriteAdaptor
{
  PBMemberDummyWriteAdaptor ()
  {
    // .. nothing yet ..
  }

  void operator () (Parent &, PBReaderState &) const
  {
    // .. nothing yet ..
  }
};

template <class Value, class Parent>
struct PBMemberWriteAdaptor
{
  PBMemberWriteAdaptor (Value Parent::*member)
    : mp_member (member)
  {
    // .. nothing yet ..
  }

  void operator () (Parent &owner, PBReaderState &reader) const
  {
    PBObjTag<Value> tag;
    owner.*mp_member = *reader.back (tag);
  }

private:
  Value Parent::*mp_member;
};

template <class Value, class Parent>
struct PBMemberAccRefWriteAdaptor
{
  PBMemberAccRefWriteAdaptor (void (Parent::*member) (const Value &))
    : mp_member (member)
  {
    // .. nothing yet ..
  }

  void operator () (Parent &owner, PBReaderState &reader) const
  {
    PBObjTag<Value> tag;
    (owner.*mp_member) (*reader.back (tag));
  }

private:
  void (Parent::*mp_member) (const Value &);
};

template <class Value, class Parent>
struct PBMemberTransferWriteAdaptor
{
  PBMemberTransferWriteAdaptor (void (Parent::*member) (Value *))
    : mp_member (member)
  {
    // .. nothing yet ..
  }

  void operator () (Parent &owner, PBReaderState &reader) const
  {
    PBObjTag<Value> tag;
    (owner.*mp_member) (reader.detach_back (tag));
  }

private:
  void (Parent::*mp_member) (Value *);
};

template <class Value, class Parent>
struct PBMemberAccWriteAdaptor
{
  PBMemberAccWriteAdaptor (void (Parent::*member) (Value))
    : mp_member (member)
  {
    // .. nothing yet ..
  }

  void operator () (Parent &owner, PBReaderState &reader) const
  {
    PBObjTag<Value> tag;
    (owner.*mp_member) (*reader.back (tag));
  }

private:
  void (Parent::*mp_member) (Value);
};

template <class Value, class Parent>
struct PBMemberDummyReadAdaptor
{
  typedef pb_pass_by_ref_tag tag;
  typedef pb_zero_cardinality_tag cardinality;

  PBMemberDummyReadAdaptor ()
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
struct PBMemberReadAdaptor
{
  typedef pb_pass_by_ref_tag tag;
  typedef pb_single_cardinality_tag cardinality;

  PBMemberReadAdaptor (Value Parent::*member)
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
struct PBMemberAccRefReadAdaptor
{
  typedef pb_pass_by_ref_tag tag;
  typedef pb_single_cardinality_tag cardinality;

  PBMemberAccRefReadAdaptor (const Value &(Parent::*member) () const)
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
struct PBMemberAccReadAdaptor
{
  typedef pb_pass_by_value_tag tag;
  typedef pb_single_cardinality_tag cardinality;

  PBMemberAccReadAdaptor (Value (Parent::*member) () const)
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
struct PBMemberIterReadAdaptor
{
  typedef pb_pass_by_ref_tag tag;
  typedef pb_many_cardinality_tag cardinality;

  PBMemberIterReadAdaptor (Iter (Parent::*begin) () const, Iter (Parent::*end) () const)
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
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> >
pb_make_element (void (Parent::*setter) (const Value &), const std::string &name, int tag, const PBElementList *children)
{
  return PBElement<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> > (
          PBMemberDummyReadAdaptor <Value, Parent> (),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> >
pb_make_element (void (Parent::*setter) (Value *), const std::string &name, int tag, const PBElementList *children)
{
  return PBElement<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> > (
          PBMemberDummyReadAdaptor <Value, Parent> (),
          PBMemberTransferWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> >
pb_make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag, const PBElementList *children)
{
  return PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> >
pb_make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, int tag, const PBElementList *children)
{
  return PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberTransferWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent> >
pb_make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, int tag, const PBElementList *children)
{
  return PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberAccWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> >
pb_make_element (Value (Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag, const PBElementList *children)
{
  return PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> > (
          PBMemberAccReadAdaptor <Value, Parent> (getter),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> >
pb_make_element (Value (Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, int tag, const PBElementList *children)
{
  return PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> > (
          PBMemberAccReadAdaptor <Value, Parent> (getter),
          PBMemberTransferWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent> >
pb_make_element (Value (Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, int tag, const PBElementList *children)
{
  return PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent> > (
          PBMemberAccReadAdaptor <Value, Parent> (getter),
          PBMemberAccWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberReadAdaptor <Value, Parent>, PBMemberWriteAdaptor <Value, Parent> >
pb_make_element (Value Parent::*member, const std::string &name, int tag, const PBElementList *children)
{
  return PBElement<Value, Parent, PBMemberReadAdaptor <Value, Parent>, PBMemberWriteAdaptor <Value, Parent> > (
          PBMemberReadAdaptor <Value, Parent> (member),
          PBMemberWriteAdaptor <Value, Parent> (member), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Iter, class Parent>
PBElement<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> >
pb_make_element (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag, const PBElementList *children)
{
  return PBElement<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> > (
          PBMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Iter, class Parent>
PBElement<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> >
pb_make_element (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (Value *), const std::string &name, int tag, const PBElementList *children)
{
  return PBElement<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> > (
          PBMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end),
          PBMemberTransferWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> >
pb_make_element (void (Parent::*setter) (const Value &), const std::string &name, int tag, const PBElementList &children)
{
  return PBElement<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> > (
          PBMemberDummyReadAdaptor <Value, Parent> (),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> >
pb_make_element (void (Parent::*setter) (Value *), const std::string &name, int tag, const PBElementList &children)
{
  return PBElement<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> > (
          PBMemberDummyReadAdaptor <Value, Parent> (),
          PBMemberTransferWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> >
pb_make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag, const PBElementList &children)
{
  return PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> >
pb_make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, int tag, const PBElementList &children)
{
  return PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberTransferWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent> >
pb_make_element (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, int tag, const PBElementList &children)
{
  return PBElement<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberAccWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> >
pb_make_element (Value (Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag, const PBElementList &children)
{
  return PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> > (
          PBMemberAccReadAdaptor <Value, Parent> (getter),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> >
pb_make_element (Value (Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, int tag, const PBElementList &children)
{
  return PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> > (
          PBMemberAccReadAdaptor <Value, Parent> (getter),
          PBMemberTransferWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent> >
pb_make_element (Value (Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, int tag, const PBElementList &children)
{
  return PBElement<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent> > (
          PBMemberAccReadAdaptor <Value, Parent> (getter),
          PBMemberAccWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Parent>
PBElement<Value, Parent, PBMemberReadAdaptor <Value, Parent>, PBMemberWriteAdaptor <Value, Parent> >
pb_make_element (Value Parent::*member, const std::string &name, int tag, const PBElementList &children)
{
  return PBElement<Value, Parent, PBMemberReadAdaptor <Value, Parent>, PBMemberWriteAdaptor <Value, Parent> > (
          PBMemberReadAdaptor <Value, Parent> (member),
          PBMemberWriteAdaptor <Value, Parent> (member), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Iter, class Parent>
PBElement<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> >
pb_make_element (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag, const PBElementList &children)
{
  return PBElement<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> > (
          PBMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief Utility: create a PBElement object
 */
template <class Value, class Iter, class Parent>
PBElement<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> >
pb_make_element (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (Value *), const std::string &name, int tag, const PBElementList &children)
{
  return PBElement<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> > (
          PBMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end),
          PBMemberTransferWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

template <class Value, class Parent>
PBElementWithParentRef<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> >
pb_make_element_with_parent_ref (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag, const PBElementList &children)
{
  return PBElementWithParentRef<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

template <class Value, class Parent>
PBElementWithParentRef<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> >
pb_make_element_with_parent_ref (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag, const PBElementList *children)
{
  return PBElementWithParentRef<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

template <class Value, class Parent>
PBElementWithParentRef<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> >
pb_make_element_with_parent_ref (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, int tag, const PBElementList &children)
{
  return PBElementWithParentRef<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberTransferWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

template <class Value, class Parent>
PBElementWithParentRef<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> >
pb_make_element_with_parent_ref (const Value &(Parent::*getter) () const, void (Parent::*setter) (Value *), const std::string &name, int tag, const PBElementList *children)
{
  return PBElementWithParentRef<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberTransferWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

template <class Value, class Iter, class Parent>
PBElementWithParentRef<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> >
pb_make_element_with_parent_ref (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag, const PBElementList &children)
{
  return PBElementWithParentRef<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent> > (
          PBMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

template <class Value, class Iter, class Parent>
PBElementWithParentRef<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> >
pb_make_element_with_parent_ref (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (Value *), const std::string &name, int tag, const PBElementList &children)
{
  return PBElementWithParentRef<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberTransferWriteAdaptor <Value, Parent> > (
          PBMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end),
          PBMemberTransferWriteAdaptor <Value, Parent> (setter), name, tag, children);
}

/**
 *  @brief A helper class providing string to value (and back) conversion
 */

template <class Value>
struct PBStdConverter
{
  typedef std::string pb_type;

  pb_type pb_encode (const Value &v) const
  {
    return tl::to_string (v);
  }

  void pb_decode (const pb_type &s, Value &v) const
  {
    tl::from_string (s, v);
  }
};

/**
 *  @brief Utility: create a PBMember object without read & write capability
 */
template <class Parent>
PBMember<std::string, Parent, PBMemberDummyReadAdaptor <std::string, Parent>, PBMemberDummyWriteAdaptor <std::string, Parent>, PBStdConverter <std::string> >
pb_make_member (const std::string &name, int tag)
{
  return PBMember<std::string, Parent, PBMemberDummyReadAdaptor <std::string, Parent>, PBMemberDummyWriteAdaptor <std::string, Parent>, PBStdConverter <std::string> > (
          PBMemberDummyReadAdaptor <std::string, Parent> (),
          PBMemberDummyWriteAdaptor <std::string, Parent> (), name, tag);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent>
PBMember<Value, Parent, PBMemberReadAdaptor <Value, Parent>, PBMemberWriteAdaptor <Value, Parent>, PBStdConverter <Value> >
pb_make_member (Value Parent::*member, const std::string &name, int tag)
{
  return PBMember<Value, Parent, PBMemberReadAdaptor <Value, Parent>, PBMemberWriteAdaptor <Value, Parent>, PBStdConverter <Value> > (
          PBMemberReadAdaptor <Value, Parent> (member),
          PBMemberWriteAdaptor <Value, Parent> (member), name, tag);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent>
PBMember<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, PBStdConverter <Value> >
pb_make_member (void (Parent::*setter) (const Value &), const std::string &name, int tag)
{
  return PBMember<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, PBStdConverter <Value> > (
          PBMemberDummyReadAdaptor <Value, Parent> (),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent>
PBMember<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent>, PBStdConverter <Value> >
pb_make_member (void (Parent::*setter) (Value), const std::string &name, int tag)
{
  return PBMember<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent>, PBStdConverter <Value> > (
          PBMemberDummyReadAdaptor <Value, Parent> (),
          PBMemberAccWriteAdaptor <Value, Parent> (setter), name, tag);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent>
PBMember<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, PBStdConverter <Value> >
pb_make_member (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag)
{
  return PBMember<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, PBStdConverter <Value> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent>
PBMember<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent>, PBStdConverter <Value> >
pb_make_member (Value (Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, int tag)
{
  return PBMember<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent>, PBStdConverter <Value> > (
          PBMemberAccReadAdaptor <Value, Parent> (getter),
          PBMemberAccWriteAdaptor <Value, Parent> (setter), name, tag);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent>
PBMember<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent>, PBStdConverter <Value> >
pb_make_member (const Value & (Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, int tag)
{
  return PBMember<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent>, PBStdConverter <Value> > (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberAccWriteAdaptor <Value, Parent> (setter), name, tag);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent>
PBMember<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, PBStdConverter <Value> >
pb_make_member (Value (Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag)
{
  return PBMember<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, PBStdConverter <Value> > (
          PBMemberAccReadAdaptor <Value, Parent> (getter),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Iter, class Parent>
PBMember<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, PBStdConverter <Value> >
pb_make_member (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag)
{
  return PBMember<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, PBStdConverter <Value> > (
          PBMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent, class Converter>
PBMember<Value, Parent, PBMemberReadAdaptor <Value, Parent>, PBMemberWriteAdaptor <Value, Parent>, Converter>
pb_make_member (Value Parent::*member, const std::string &name, int tag, Converter conv)
{
  return PBMember<Value, Parent, PBMemberReadAdaptor <Value, Parent>, PBMemberWriteAdaptor <Value, Parent>, Converter> (
          PBMemberReadAdaptor <Value, Parent> (member),
          PBMemberWriteAdaptor <Value, Parent> (member), name, tag, conv);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent, class Converter>
PBMember<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, Converter>
pb_make_member (void (Parent::*setter) (const Value &), const std::string &name, int tag, Converter conv)
{
  return PBMember<Value, Parent, PBMemberDummyReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, Converter> (
          PBMemberDummyReadAdaptor <Value, Parent> (),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, conv);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent, class Converter>
PBMember<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberDummyWriteAdaptor <Value, Parent>, Converter>
pb_make_member (void (Parent::*setter) (Value), const std::string &name, int tag, Converter conv)
{
  return PBMember<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberDummyWriteAdaptor <Value, Parent>, Converter> (
          PBMemberAccReadAdaptor <Value, Parent> (setter),
          PBMemberDummyWriteAdaptor <Value, Parent> (), name, tag, conv);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent, class Converter>
PBMember<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, Converter>
pb_make_member (const Value &(Parent::*getter) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag, Converter conv)
{
  return PBMember<Value, Parent, PBMemberAccRefReadAdaptor <Value, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, Converter> (
          PBMemberAccRefReadAdaptor <Value, Parent> (getter),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, conv);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Parent, class Converter>
PBMember<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent>, Converter>
pb_make_member (Value (Parent::*getter) () const, void (Parent::*setter) (Value), const std::string &name, int tag, Converter conv)
{
  return PBMember<Value, Parent, PBMemberAccReadAdaptor <Value, Parent>, PBMemberAccWriteAdaptor <Value, Parent>, Converter> (
          PBMemberAccReadAdaptor <Value, Parent> (getter),
          PBMemberAccWriteAdaptor <Value, Parent> (setter), name, tag, conv);
}

/**
 *  @brief Utility: create a PBMember object
 */
template <class Value, class Iter, class Parent, class Converter>
PBMember<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, Converter>
pb_make_member (Iter (Parent::*begin) () const, Iter (Parent::*end) () const, void (Parent::*setter) (const Value &), const std::string &name, int tag, Converter conv)
{
  return PBMember<Value, Parent, PBMemberIterReadAdaptor <const Value &, Iter, Parent>, PBMemberAccRefWriteAdaptor <Value, Parent>, Converter> (
          PBMemberIterReadAdaptor <const Value &, Iter, Parent> (begin, end),
          PBMemberAccRefWriteAdaptor <Value, Parent> (setter), name, tag, conv);
}

}

#endif
