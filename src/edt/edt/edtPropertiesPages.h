
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef HDR_edtPropertiesPages
#define HDR_edtPropertiesPages

#include "layPlugin.h"
#include "layProperties.h"
#include "layWidgets.h"
#include "edtService.h"
#include "ui_PolygonPropertiesPage.h"
#include "ui_BoxPropertiesPage.h"
#include "ui_PointPropertiesPage.h"
#include "ui_PathPropertiesPage.h"
#include "ui_EditablePathPropertiesPage.h"
#include "ui_TextPropertiesPage.h"

namespace edt
{

class ChangeApplicator;

class ShapePropertiesPage
  : public lay::PropertiesPage
{
Q_OBJECT

public:
  ShapePropertiesPage (const std::string &description, edt::Service *service, db::Manager *manager, QWidget *parent);
  ~ShapePropertiesPage ();

  virtual size_t count () const;
  virtual void select_entries (const std::vector<size_t> &entries);
  virtual std::string description (size_t entry) const;
  virtual std::string description () const;
  virtual void confine_selection (const std::vector<size_t> &remaining_entries);
  virtual QIcon icon (size_t entry, int w, int h) const;
  virtual QIcon icon (int w, int h) const { return lay::PropertiesPage::icon (w, h); }
  virtual void leave ();

protected:
  virtual bool readonly ();

private:
  virtual void update ();
  virtual void apply (bool commit);
  virtual void apply_to_all (bool relative, bool commit);
  virtual bool can_apply_to_all () const;
  void do_apply (bool current_only, bool relative, bool commit);
  void recompute_selection_ptrs (const std::vector<lay::ObjectInstPath> &new_sel);
  void apply_change (const ChangeApplicator *applicator, unsigned int cv_index, bool current_only, bool relative, bool commit);

protected:
  std::string m_description;
  std::vector<EditableSelectionIterator::pointer> m_selection_ptrs;
  std::vector<size_t> m_indexes;
  edt::Service *mp_service;
  bool m_enable_cb_callback;
  db::properties_id_type m_prop_id;

  virtual void do_update (const db::Shape &shape, double dbu) = 0;
  virtual ChangeApplicator *create_applicator (db::Shapes &shapes, const db::Shape &shape, double dbu) = 0;
  virtual QCheckBox *dbu_checkbox () const = 0;
  virtual QCheckBox *abs_checkbox () const = 0;
  virtual lay::LayerSelectionComboBox *layer_selector () const = 0;
  virtual QLabel *cell_label () const = 0;
  bool dbu_units () const;
  bool abs_trans () const;
  db::ICplxTrans trans () const;
  void setup ();
  lay::LayoutViewBase *view () const;
  const db::Shape &shape (size_t entry) const;
  double dbu (size_t entry) const;

public slots:
  void show_inst ();
  void show_props ();
  void display_mode_changed (bool);
  void update_shape ();
  void current_layer_changed ();
};


class PolygonPropertiesPage
  : public ShapePropertiesPage,
    public Ui::PolygonPropertiesPage
{
Q_OBJECT

public:
  PolygonPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent);

  virtual std::string description (size_t entry) const;
  virtual std::string description () const { return ShapePropertiesPage::description (); }
  virtual void do_update (const db::Shape &shape, double dbu);
  virtual ChangeApplicator *create_applicator (db::Shapes &shapes, const db::Shape &shape, double dbu);

protected:
  virtual QCheckBox *dbu_checkbox () const { return dbu_cb; }
  virtual QCheckBox *abs_checkbox () const { return abs_cb; }
  virtual lay::LayerSelectionComboBox *layer_selector () const { return layer_cbx; }
  virtual QLabel *cell_label () const { return cell_lbl; }

public slots:
  void text_changed ();

private:
  bool m_in_text_changed;
};

class BoxPropertiesPage
  : public ShapePropertiesPage,
    public Ui::BoxPropertiesPage
{
Q_OBJECT

public:
  BoxPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent);

  virtual std::string description (size_t entry) const;
  virtual std::string description () const { return ShapePropertiesPage::description (); }
  virtual void do_update (const db::Shape &shape, double dbu);
  virtual ChangeApplicator *create_applicator (db::Shapes &shapes, const db::Shape &shape, double dbu);

public slots:
  void changed ();

protected:
  virtual QCheckBox *dbu_checkbox () const { return dbu_cb; }
  virtual QCheckBox *abs_checkbox () const { return abs_cb; }
  virtual lay::LayerSelectionComboBox *layer_selector () const { return layer_cbx; }
  virtual QLabel *cell_label () const { return cell_lbl; }

private:
  bool m_recursion_sentinel;
  int m_tab_index;
  double m_dbu;
  mutable bool m_lr_swapped, m_tb_swapped;

  db::Box get_box (int mode) const;
  void set_box (const db::Box &box);
};

class PointPropertiesPage
  : public ShapePropertiesPage,
    public Ui::PointPropertiesPage
{
Q_OBJECT

public:
  PointPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent);

  virtual std::string description (size_t entry) const;
  virtual std::string description () const { return ShapePropertiesPage::description (); }
  virtual void do_update (const db::Shape &shape, double dbu);
  virtual ChangeApplicator *create_applicator (db::Shapes &shapes, const db::Shape &shape, double dbu);

public slots:
  void changed ();

protected:
  virtual QCheckBox *dbu_checkbox () const { return dbu_cb; }
  virtual QCheckBox *abs_checkbox () const { return abs_cb; }
  virtual lay::LayerSelectionComboBox *layer_selector () const { return layer_cbx; }
  virtual QLabel *cell_label () const { return cell_lbl; }

private:
  double m_dbu;

  db::Point get_point () const;
  void set_point (const db::Point &point);
};

class TextPropertiesPage
  : public ShapePropertiesPage,
    public Ui::TextPropertiesPage
{
Q_OBJECT

public:
  TextPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent);

  virtual std::string description (size_t entry) const;
  virtual std::string description () const { return ShapePropertiesPage::description (); }
  virtual void do_update (const db::Shape &shape, double dbu);
  virtual ChangeApplicator *create_applicator (db::Shapes &shapes, const db::Shape &shape, double dbu);

protected:
  virtual QCheckBox *dbu_checkbox () const { return dbu_cb; }
  virtual QCheckBox *abs_checkbox () const { return abs_cb; }
  virtual lay::LayerSelectionComboBox *layer_selector () const { return layer_cbx; }
  virtual QLabel *cell_label () const { return cell_lbl; }
};

class PathPropertiesPage
  : public ShapePropertiesPage,
    public Ui::PathPropertiesPage
{
Q_OBJECT

public:
  PathPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent);

  virtual std::string description (size_t entry) const;
  virtual std::string description () const { return ShapePropertiesPage::description (); }
  virtual void do_update (const db::Shape &shape, double dbu);
  virtual ChangeApplicator *create_applicator (db::Shapes &shapes, const db::Shape &shape, double dbu);

protected:
  virtual QCheckBox *dbu_checkbox () const { return dbu_cb; }
  virtual QCheckBox *abs_checkbox () const { return abs_cb; }
  virtual lay::LayerSelectionComboBox *layer_selector () const { return layer_cbx; }
  virtual QLabel *cell_label () const { return cell_lbl; }

private:
  bool m_in_text_changed;
};

class EditablePathPropertiesPage
  : public ShapePropertiesPage,
    public Ui::EditablePathPropertiesPage
{
Q_OBJECT

public:
  EditablePathPropertiesPage (edt::Service *service, db::Manager *manager, QWidget *parent);

  virtual std::string description (size_t entry) const;
  virtual std::string description () const { return ShapePropertiesPage::description (); }
  virtual void do_update (const db::Shape &shape, double dbu);
  virtual ChangeApplicator *create_applicator (db::Shapes &shapes, const db::Shape &shape, double dbu);

protected:
  virtual QCheckBox *dbu_checkbox () const { return dbu_cb; }
  virtual QCheckBox *abs_checkbox () const { return abs_cb; }
  virtual lay::LayerSelectionComboBox *layer_selector () const { return layer_cbx; }
  virtual QLabel *cell_label () const { return cell_lbl; }

public slots:
  void type_selected (int); 
  void text_changed ();

private:
  bool m_in_text_changed;
};

}

#endif

#endif
