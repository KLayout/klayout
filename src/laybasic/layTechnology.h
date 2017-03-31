
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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


#ifndef HDR_layTechnology
#define HDR_layTechnology

#include "laybasicCommon.h"

#include "tlStableVector.h"
#include "tlString.h"
#include "tlEvents.h"
#include "tlXMLParser.h"
#include "tlTypeTraits.h"
#include "dbStreamLayers.h"
#include "dbLoadLayoutOptions.h"
#include "dbSaveLayoutOptions.h"

#include <QFrame>

namespace lay
{

class Technology;
class TechnologyComponent;
class TechnologyComponentEditor;

/**
 *  @brief A container for the technology settings
 *
 *  The container associates a technology with a name and provides an
 *  iterator for the technologies.
 *  The container features at least one technology (the default) which is
 *  present in any case. If a technology with an unknown name is requested,
 *  this default technology is returned.
 */
class LAYBASIC_PUBLIC Technologies
  : public tl::Object
{
public:
  typedef tl::stable_vector<Technology>::const_iterator const_iterator;
  typedef tl::stable_vector<Technology>::iterator iterator;

  /**
   *  @brief The constructor
   */
  Technologies ();

  /**
   *  @brief The destructor
   */
  ~Technologies ();

  /**
   *  @brief Copy ctor
   */
  Technologies (const Technologies &other); 

  /**
   *  @brief Assignment operator
   */
  Technologies &operator= (const Technologies &other);

  /**
   *  @brief Const iterator - begin 
   */
  const_iterator begin () const
  {
    return m_technologies.begin ();
  }

  /**
   *  @brief Const iterator - end 
   */
  const_iterator end () const
  {
    return m_technologies.end ();
  }

  /**
   *  @brief iterator - begin
   */
  iterator begin ()
  {
    return m_technologies.begin ();
  }

  /**
   *  @brief Const iterator - end
   */
  iterator end ()
  {
    return m_technologies.end ();
  }

  /**
   *  @brief The number of technologies
   */
  size_t technologies () const
  {
    return m_technologies.size ();
  }

  /**
   *  @brief Adds a technology to the setup
   *
   *  The container becomes owner of the technology object.
   *  Replaces a technology with the name of the given technology.
   */
  void add (Technology *technology);

  /**
   *  @brief Remove a technology with the given name from the setup
   */
  void remove (const std::string &name);

  /**
   *  @brief Checks, if a technology with the given name exists
   */
  bool has_technology (const std::string &name) const;

  /**
   *  @brief Returns the technology with the given name
   *
   *  If no technology with that name exists, the default technology is returned.
   */
  Technology *technology_by_name (const std::string &name);

  /**
   *  @brief Returns the technology with the given name (const version)
   *
   *  If no technology with that name exists, the default technology is returned.
   */
  const Technology *technology_by_name (const std::string &name) const
  {
    return const_cast<Technologies *> (this)->technology_by_name (name);
  }

  /**
   *  @brief Converts the list into an XML string
   */
  std::string to_xml () const;

  /**
   *  @brief Reads the list from an XML string
   */
  void load_from_xml (const std::string &s);

  /**
   *  @brief Returns the singleton instance
   */
  static lay::Technologies *instance ();

  /**
   *  @brief An event indicating that the list of technologies has changed
   *  If a technology is added or removed, this event is triggered.
   */
  tl::Event technologies_changed_event;

  /**
   *  @brief An event indicating that one technology in the list has changed
   *  If a technology is modified, this event is triggered with that technology as argument of the event.
   */
  tl::event<Technology *> technology_changed_event;

protected:
  /**
   *  @brief Forward the event from the individual technologies
   */
  void technology_changed (Technology *t)
  {
    technology_changed_event (t);
  }

private:
  tl::stable_vector<Technology> m_technologies;
};

/**
 *  @brief A technology 
 *
 *  This class represents one technology.
 *  A technology has a name and a description.
 */
class LAYBASIC_PUBLIC Technology
  : public tl::Object
{
public:
  /**
   *  @brief The default constructor 
   */
  Technology ();

  /**
   *  @brief The constructor 
   */
  Technology (const std::string &name, const std::string &description);

  /**
   *  @brief The copy constructor 
   */
  Technology (const Technology &tech);

  /**
   *  @brief The destructor
   */
  ~Technology ();

  /**
   *  @brief Assignment
   */
  Technology &operator= (const Technology &tech);

  /**
   *  @brief Gets the name 
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the name 
   */
  void set_name (const std::string &n) 
  {
    if (n != m_name) {
      m_name = n;
      technology_changed ();
    }
  }

  /**
   *  @brief Gets the base path 
   *
   *  The base path is an effective path - if the explicit path is set, it is 
   *  used. If not, the default path is used. The default path is the one from which
   *  a technology file was imported. The explicit one is the one that is specified
   *  explicitly.
   */
  const std::string &base_path () const
  {
    return m_explicit_base_path.empty () ? m_default_base_path : m_explicit_base_path;
  }

  /**
   *  @brief Makes a file path relative to the base path if one is specified.
   *
   *  Only files below the base path will be made relative. Files above or beside
   *  won't be made relative.
   */
  std::string correct_path (const std::string &fp) const;

  /**
   *  @brief Gets the default base path 
   */
  const std::string &default_base_path () const
  {
    return m_default_base_path;
  }

  /**
   *  @brief Sets the default base path 
   */
  void set_default_base_path (const std::string &p) 
  {
    if (m_default_base_path != p) {
      m_default_base_path = p;
      technology_changed ();
    }
  }

  /**
   *  @brief Gets the explicit base path 
   */
  const std::string &explicit_base_path () const
  {
    return m_explicit_base_path;
  }

  /**
   *  @brief Sets the explicit base path 
   */
  void set_explicit_base_path (const std::string &p) 
  {
    if (m_explicit_base_path != p) {
      m_explicit_base_path = p;
      technology_changed ();
    }
  }

  /**
   *  @brief Gets the description 
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Sets the description 
   */
  void set_description (const std::string &d) 
  {
    if (m_description != d) {
      m_description = d;
      technology_changed ();
    }
  }

  /**
   *  @brief Gets the default database unit
   */
  double dbu () const
  {
    return m_dbu;
  }

  /**
   *  @brief Sets the default database unit
   */
  void set_dbu (double d)
  {
    if (fabs (m_dbu - d) > 1e-10) {
      m_dbu = d;
      technology_changed ();
    }
  }

  /**
   *  @brief Gets the layer properties file path (empty if none is specified)
   */
  const std::string &layer_properties_file () const
  {
    return m_lyp_path;
  }

  /**
   *  @brief Gets the effective layer properties file path (empty if none is specified)
   *
   *  The effective path is the one extended by the base path if relative.
   */
  std::string eff_layer_properties_file () const
  {
    return build_effective_path (m_lyp_path);
  }

  /**
   *  @brief Sets the layer properties file path (set to empty string to remove layer properties file)
   */
  void set_layer_properties_file (const std::string &lyp)
  {
    if (m_lyp_path != lyp) {
      m_lyp_path = lyp;
      technology_changed ();
    }
  }

  /**
   *  @brief Gets the flag indicating whether to add other layers to the layer properties
   */
  bool add_other_layers () const
  {
    return m_add_other_layers;
  }

  /**
   *  @brief Sets the flag indicating whether to add other layers to the layer properties
   *
   *  If "add_other_layers" is true, the layers in the layout but not specified in the
   *  layer properties file will be added automatically.
   */
  void set_add_other_layers (bool add_other_layers)
  {
    if (m_add_other_layers != add_other_layers) {
      m_add_other_layers = add_other_layers;
      technology_changed ();
    }
  }

  /**
   *  @brief gets the layout reader options
   */
  const db::LoadLayoutOptions &load_layout_options () const
  {
    return m_load_layout_options;
  }

  /**
   *  @brief Sets the layout reader options
   */
  void set_load_layout_options (const db::LoadLayoutOptions &options)
  {
    m_load_layout_options = options;
    technology_changed ();
  }

  /**
   *  @brief gets the layout writer options
   */
  const db::SaveLayoutOptions &save_layout_options () const
  {
    return m_save_layout_options;
  }

  /**
   *  @brief Sets the layout writer options
   */
  void set_save_layout_options (const db::SaveLayoutOptions &options)
  {
    m_save_layout_options = options;
    technology_changed ();
  }

  /**
   *  @brief Load from file (import)
   */
  void load (const std::string &fn);

  /**
   *  @brief Save to file (export)
   */
  void save (const std::string &fn) const;

  /**
   *  @brief Delivers the XMLElementList that specifies the technology's XML representation
   */
  static tl::XMLElementList xml_elements ();

  /**
   *  @brief Sets the technology component by the component name
   *
   *  This replaces the technology component with the given name.
   *  The Technology object will become owner of the component.
   */
  void set_component (TechnologyComponent *component);

  /**
   *  @brief Gets the technology component by the component name
   *
   *  If no component with that name exists, 0 is returned.
   */
  const TechnologyComponent *component_by_name (const std::string &component_name) const;

  /**
   *  @brief Gets the technology component by the component name (non-const version)
   *
   *  If no component with that name exists, 0 is returned.
   */
  TechnologyComponent *component_by_name (const std::string &component_name);

  /**
   *  @brief Gets the component names
   */
  std::vector <std::string> component_names () const;

  /**
   *  @brief Builds the effective path from a relative or absolute one using the base path if necessary
   */
  std::string build_effective_path (const std::string &p) const;

  /**
   *  @brief Returns a flag indicating whether the technology is persisted or not
   *
   *  If the flag is false, this technology is not included into the XML string
   *  of the technologies.
   */
  bool is_persisted () const
  {
    return m_persisted;
  }

  /**
   *  @brief Sets a flag indicating whether the technology is persisted
   */
  void set_persisted (bool f)
  {
    m_persisted = f;
  }

  /**
   *  @brief An event indicating that the technology has changed
   */
  tl::Event technology_changed_event;

  /**
   *  @brief An event indicating that the technology has changed (with a sender argument)
   */
  tl::event<Technology *> technology_changed_with_sender_event;

private:
  std::string m_name, m_description;
  double m_dbu;
  std::string m_explicit_base_path, m_default_base_path;
  db::LoadLayoutOptions m_load_layout_options;
  db::SaveLayoutOptions m_save_layout_options;
  std::string m_lyp_path;
  bool m_add_other_layers;
  std::vector <TechnologyComponent *> m_components;
  bool m_persisted;

  void init ();

  void technology_changed ()
  {
    technology_changed_with_sender_event (this);
    technology_changed_event ();
  }
};

/**
 *  @brief A technology component
 *
 *  A technology component is a part of the data for one technology. 
 *  Plugins may register technology components in every technology and
 *  use those components to store their specific data.
 *  A technology component has a name and a description. The name is used
 *  to identify a component within a technology. The description is shown
 *  in the setup dialogs.
 *  This class is the base class for all technology components.
 */
class LAYBASIC_PUBLIC TechnologyComponent
{
public:
  /**
   *  @brief The constructor
   *
   *  @param name The name of the technology component
   *  @param descriptor The description of the technology component
   */
  TechnologyComponent (const std::string &name, const std::string &description)
    : m_name (name), m_description (description)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The destructor
   */
  virtual ~TechnologyComponent ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Gets the name 
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Gets the description 
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Creates the editor 
   *
   *  The implementation of this method is supposed to create an editor for
   *  the given component. It is guaranteed that the component that is given
   *  to the editor is one that has been delivered by create_component ().
   */
  virtual TechnologyComponentEditor *create_editor (QWidget * /*parent*/) 
  {
    return 0;
  }

  /**
   *  @brief Clone this instance
   */
  virtual TechnologyComponent *clone () const = 0;

private:
  std::string m_name, m_description;
};

/**
 *  @brief A base class for an editor for a technology component
 *
 *  A technology component provider can provide an editor for the component
 *  To do so, it must implement a TechnologyComponentEditor that provides an 
 *  editor for it's component.
 */
class LAYBASIC_PUBLIC TechnologyComponentEditor
  : public QFrame
{
public:
  /**
   *  @brief The constructor
   */
  TechnologyComponentEditor (QWidget *parent)
    : QFrame (parent), mp_tech (0), mp_tech_component (0)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The destructor
   */
  virtual ~TechnologyComponentEditor ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Set the technology and component for the editor
   */
  void set_technology (Technology *tech, TechnologyComponent *tech_component)
  {
    mp_tech = tech;
    mp_tech_component = tech_component;
  }

  /**
   *  @brief Sets up the editor with the given technology and component
   */
  virtual void setup () 
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Commits the edits to the technology and component
   */
  virtual void commit () 
  {
    // .. nothing yet ..
  }

protected:
  Technology *tech () 
  {
    return mp_tech;
  }

  TechnologyComponent *tech_component () 
  {
    return mp_tech_component;
  }

private:
  Technology *mp_tech;
  TechnologyComponent *mp_tech_component;
};

/**
 *  @brief A base class for a technology component provider
 */
class LAYBASIC_PUBLIC TechnologyComponentProvider
{
public:
  /**
   *  @brief The constructor
   */
  TechnologyComponentProvider ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The destructor
   */
  virtual ~TechnologyComponentProvider ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Creates the technology component
   */
  virtual TechnologyComponent *create_component () const = 0;

  /**
   *  @brief Delivers the XMLElement object that represents this component within a technology XML tree
   *
   *  The object returned is destroyed by the caller.
   */
  virtual tl::XMLElementBase *xml_element () const = 0;
};

/**
 *  @brief A helper class for the XML serialization of the technology component (custom read adaptor)
 */

template <class TC>
class TechnologyComponentReadAdaptor 
{
public:
  typedef tl::pass_by_ref_tag tag;

  TechnologyComponentReadAdaptor (const std::string &name)
    : m_name (name), mp_t (0), m_done (false)
  {
    // .. nothing yet ..
  }

  const TC &operator () () const
  {
    const TC *tc = dynamic_cast<const TC *> ((const_cast <lay::Technology *> (mp_t))->component_by_name (m_name));
    if (! tc) {
      throw tl::Exception (tl::to_string (QObject::tr ("Unknown technology component: ")) + m_name);
    }

    return *tc;
  }

  bool at_end () const 
  {
    return m_done;
  }

  void start (const lay::Technology &t) 
  {
    mp_t = &t;
    m_done = false;
  }

  void next () 
  {
    m_done = true;
  }

private:
  std::string m_name;
  const lay::Technology *mp_t;
  bool m_done;
};

/**
 *  @brief A helper class for the XML serialization of the technology component (custom write adaptor)
 */

template <class TC>
class TechnologyComponentWriteAdaptor 
{
public:
  TechnologyComponentWriteAdaptor (const std::string &name)
    : m_name (name)
  {
    // .. nothing yet ..
  }

  void operator () (lay::Technology &t, tl::XMLReaderState &reader) const
  {
    const TechnologyComponent *tc_basic = t.component_by_name (m_name);
    TC *tc = 0;
    if (! tc_basic) {
      tc = new TC ();
    } else {
      tc = dynamic_cast<TC *> (tc_basic->clone ());
      if (! tc) {
        throw tl::Exception (tl::to_string (QObject::tr ("Invalid technology component: ")) + m_name);
      }
    }

    tl::XMLObjTag<TC> tag;
    *tc = *reader.back (tag);

    t.set_component (tc);
  }

private:
  std::string m_name;
};

/**
 *  @brief A custom XMLElement for the serialization of technology components
 *
 *  TechnologyComponentProvider::xml_element can return such an element to 
 *  insert a custom XML element into the XML tree which represents the 
 *  technology component.
 *
 *  The name of the element will be the name of the technology component.
 */

template <class TC>
class TechnologyComponentXMLElement
  : public tl::XMLElement<TC, lay::Technology, TechnologyComponentReadAdaptor<TC>, TechnologyComponentWriteAdaptor<TC> >
{
public:
  TechnologyComponentXMLElement (const std::string &name, const tl::XMLElementList &children)
    : tl::XMLElement<TC, lay::Technology, TechnologyComponentReadAdaptor<TC>, TechnologyComponentWriteAdaptor<TC> > (TechnologyComponentReadAdaptor<TC> (name), TechnologyComponentWriteAdaptor<TC> (name), name, children)
  {
    //  .. nothing yet ..
  }

  TechnologyComponentXMLElement (const TechnologyComponentXMLElement &d)
    : tl::XMLElement<TC, lay::Technology, TechnologyComponentReadAdaptor<TC>, TechnologyComponentWriteAdaptor<TC> > (d)
  {
    //  .. nothing yet ..
  }

  virtual tl::XMLElementBase *clone () const 
  {
    return new TechnologyComponentXMLElement (*this);
  }
};

}

namespace tl
{
  /**
   *  @brief Type traits
   */
  template <> struct type_traits <lay::TechnologyComponent> : public type_traits<void> {
    typedef tl::false_tag has_default_constructor;
    typedef tl::false_tag has_copy_constructor;
  };
}

#endif

