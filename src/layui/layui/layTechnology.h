
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

#if defined(HAVE_QT)

#ifndef HDR_layTechnology
#define HDR_layTechnology

#include "layuiCommon.h"

#include "dbTechnology.h"

#include <QFrame>

namespace lay
{

/**
 *  @brief A base class for an editor for a technology component
 *
 *  A technology component provider can provide an editor for the component
 *  To do so, it must implement a TechnologyComponentEditor that provides an 
 *  editor for it's component.
 */
class LAYUI_PUBLIC TechnologyComponentEditor
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
  void set_technology (db::Technology *tech, db::TechnologyComponent *tech_component)
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
  db::Technology *tech ()
  {
    return mp_tech;
  }

  db::TechnologyComponent *tech_component ()
  {
    return mp_tech_component;
  }

private:
  db::Technology *mp_tech;
  db::TechnologyComponent *mp_tech_component;
};

/**
 *  @brief A base class for a technology component provider
 */
class LAYUI_PUBLIC TechnologyEditorProvider
{
public:
  /**
   *  @brief The constructor
   */
  TechnologyEditorProvider ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The destructor
   */
  virtual ~TechnologyEditorProvider ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Creates the technology component
   */
  virtual TechnologyComponentEditor *create_editor (QWidget * /*parent*/) const 
  {
    return 0;
  }
};

}

#endif

#endif  //  defined(HAVE_QT)
