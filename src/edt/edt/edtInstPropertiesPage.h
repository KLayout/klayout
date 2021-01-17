
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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



#ifndef HDR_edtInstPropertiesPage
#define HDR_edtInstPropertiesPage

#include "layPlugin.h"
#include "layProperties.h"
#include "ui_InstPropertiesPage.h"
#include "edtService.h"

namespace edt
{

class PCellParametersPage;
class ChangeApplicator;

class InstPropertiesPage
  : public lay::PropertiesPage,
    public Ui::InstPropertiesPage
{
Q_OBJECT

public:
  InstPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent);
  ~InstPropertiesPage ();

  virtual void back ();
  virtual void front ();
  virtual bool at_begin () const;
  virtual bool at_end () const;
  virtual void operator-- ();
  virtual void operator++ ();
  virtual void leave ();

private:
  virtual void update ();
  void recompute_selection_ptrs (const std::vector<lay::ObjectInstPath> &new_sel);

protected:
  std::vector<edt::Service::obj_iterator> m_selection_ptrs;
  unsigned int m_index;
  edt::Service *mp_service;
  bool m_enable_cb_callback;
  db::properties_id_type m_prop_id;
  edt::PCellParametersPage *mp_pcell_parameters;

  virtual bool readonly ();
  virtual void apply (); 
  virtual void apply_to_all (bool relative);
  virtual bool can_apply_to_all () const;
  void do_apply (bool current_only, bool relative);
  virtual ChangeApplicator *create_applicator (db::Cell &cell, const db::Instance &inst, double dbu);

protected slots:
  void show_inst ();
  void show_cell ();
  void show_props ();
  void cell_name_changed (const QString &s);
  void display_mode_changed (bool);
  void browse_cell ();
  void update_pcell_parameters ();
  void library_changed (int index);
};

}

#endif

