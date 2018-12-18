
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#include "dbCommon.h"
#include "tlVariant.h"

#include <string>

namespace db
{
  class NetlistProperty;
}

namespace tl
{
  class Extractor;

  //  specialization of tl::VariantUserClass for the purpose of NetlistProperty representation
  template <> class DB_PUBLIC tl::VariantUserClass<db::NetlistProperty>
    : public tl::VariantUserClassBase
  {
  public:
    //  creation not supported
    virtual void *create () const { tl_assert (false); }

    virtual void destroy (void *p) const;
    virtual bool equal (const void *a, const void *b) const;
    virtual bool less (const void *a, const void *b) const;
    virtual void *clone (const void *p) const;
    virtual const char *name () const { return ""; }
    virtual bool is_const () const { return false; }
    virtual const gsi::ClassBase *gsi_cls () const { return 0; }
    virtual const tl::EvalClass *eval_cls () const { return 0; }
    virtual std::string to_string (const void *p) const;
    virtual void read (void *p, tl::Extractor &ex) const;
    virtual void assign (void *self, const void *other) const;
    virtual void *deref_proxy (tl::Object *proxy) const;

    db::NetlistProperty *get (void *ptr) const { return reinterpret_cast<db::NetlistProperty *> (ptr); }
    const db::NetlistProperty *get (const void *ptr) const { return reinterpret_cast<const db::NetlistProperty *> (ptr); }

  protected:
    void register_instance (const tl::VariantUserClassBase *inst, bool is_const);
    void unregister_instance (const tl::VariantUserClassBase *inst, bool is_const);
  };

}

namespace db
{

/**
 *  @brief The base class for a netlist property attached to a shape
 *
 *  This class provides a wrapper for binding a netlist property
 *  to a tl::Variant. Hence it can be kept as a shape property
 *  in the context of db::Layout's propery repo.
 */
class DB_PUBLIC NetlistProperty
{
public:
  /**
   *  @brief Gets the class descriptor for keeping the object inside a tl::Variant
   *  
   *  For a Variant that owns a NetlistProperty object, use
   *  
   *  @code
   *  db::NetlistProperty *prop = new db::NetlistProperty ();
   *  bool shared = true; // the variant will own the object
   *  tl::Variant prop_in_var (prop, prop->variant_class (), shared);
   *  @endcode
   */
  static const tl::VariantUserClass<db::NetlistProperty> *variant_class ();

  /**
   *  @brief Constructor
   */
  NetlistProperty ();

  /**
   *  @brief Copy constructor
   */
  NetlistProperty (const NetlistProperty &other);

  /**
   *  @brief (virtual) Destructor
   */
  virtual ~NetlistProperty ();

  /**
   *  @brief Clones the object
   */
  virtual NetlistProperty *clone () const 
  { 
    return new NetlistProperty (*this);
  }

  /**
   *  @brief Compares two objects (equal). Both types are guaranteed to be the same.
   */
  virtual bool equals (const NetlistProperty *) const
  {
    return true;
  }

  /**
   *  @brief Compares two objects (less). Both types are guaranteed to be the same.
   */
  virtual bool less (const NetlistProperty *) const
  {
    return false;
  }

  /**
   *  @brief Assigned the other object to self. Both types are guaranteed to be identical.
   */
  virtual void assign (const NetlistProperty *)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Converts to a string
   */
  virtual std::string to_string () const
  {
    return std::string ();
  }

  /**
   *  @brief Pulls the object from a string
   */
  virtual void read (tl::Extractor &)
  {
    //  .. nothing yet ..
  }
};

/**
 *  @brief A property meaning a net name
 */
class DB_PUBLIC NetNameProperty
  : public db::NetlistProperty
{
public:
  /**
   *  @brief Creates a netlist name property without a specific name
   */
  NetNameProperty ();

  /**
   *  @brief copy constructor
   */
  NetNameProperty (const NetNameProperty &other);

  /**
   *  @brief Creates a netlist name property with the given name
   */
  NetNameProperty (const std::string &n);

  /**
   *  @brief Assignment
   */
  NetNameProperty &operator= (const NetNameProperty &other);

  /**
   *  @brief Sets the name
   */
  void set_name (const std::string &n);

  /**
   *  @brief Gets the name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Clones the object
   */
  virtual NetlistProperty *clone () const
  {
    return new NetNameProperty (*this);
  }

  /**
   *  @brief Compares two objects (equal). Both types are guaranteed to be the same.
   */
  virtual bool equals (const NetlistProperty *) const;

  /**
   *  @brief Compares two objects (less). Both types are guaranteed to be the same.
   */
  virtual bool less (const NetlistProperty *) const;

  /**
   *  @brief Assigned the other object to self. Both types are guaranteed to be identical.
   */
  virtual void assign (const NetlistProperty *);

  /**
   *  @brief Converts to a string
   */
  virtual std::string to_string () const;

  /**
   *  @brief Pulls the object from a string
   */
  virtual void read (tl::Extractor &);

private:
  std::string m_name;
};

}
