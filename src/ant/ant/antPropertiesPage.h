
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

#if defined(HAVE_QT)

#ifndef HDR_antPropertiesPage
#define HDR_antPropertiesPage

#include "layPlugin.h"
#include "layProperties.h"
#include "antService.h"
#include "ui_RulerPropertiesPage.h"

namespace ant
{

class PropertiesPage
  : public lay::PropertiesPage,
    public Ui::RulerPropertiesPage
{
Q_OBJECT

public:
  PropertiesPage (ant::Service *rulers, db::Manager *manager, QWidget *parent);
  ~PropertiesPage ();

  virtual size_t count () const;
  virtual void select_entries (const std::vector<size_t> &entries);
  virtual std::string description (size_t entry) const;
  virtual std::string description () const;
  virtual void update ();
  virtual void leave ();
  virtual bool readonly ();
  virtual void apply ();

private slots:
  void swap_points_clicked ();
  void snap_to_layout_clicked ();
  void something_changed ();

private:
  std::vector <ant::Service::obj_iterator> m_selection;
  size_t m_index;
  ant::Service *mp_rulers;
  bool m_enable_cb_callback;
  bool m_in_something_changed;

  const ant::Object &current () const;
  void get_points (db::DPoint &p1, db::DPoint &p2);
  void get_point (db::DPoint &p);
  void get_points (ant::Object::point_list &points);
  void update_with (const ant::Object &obj);
  void get_object (ant::Object &obj);
};

}

#endif

#endif
