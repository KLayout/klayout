
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


#ifndef HDR_SearchReplaceDialog
#define HDR_SearchReplaceDialog

#include "ui_SearchReplaceDialog.h"

#include "layBrowser.h"
#include "layMargin.h"
#include "rdb.h"
#include "dbLayout.h"
#include "dbShape.h"
#include "dbInstances.h"

#include <QAbstractItemModel>

#include <vector>
#include <set>

namespace db
{
  class LayoutQuery;
  class LayoutQueryIterator;
}

namespace lay
{

class LayoutView;
class MarkerBase;

class SearchReplaceResults
  : public QAbstractItemModel
{
Q_OBJECT 

public:
  struct QueryShapeResult
  {
    QueryShapeResult (const db::Shape &s, unsigned int l, const db::ICplxTrans &t, db::cell_index_type ci, db::cell_index_type ici)
      : shape (s), layer_index (l), trans (t), cell_index (ci), initial_cell_index (ici)
    { }

    db::Shape shape;
    unsigned int layer_index;
    db::ICplxTrans trans;
    db::cell_index_type cell_index;
    db::cell_index_type initial_cell_index;
  };

  struct QueryInstResult
  {
    QueryInstResult (const db::Instance &i, const db::ICplxTrans &t, db::cell_index_type ci, db::cell_index_type ici)
      : inst (i), trans (t), cell_index (ci), initial_cell_index (ici)
    { }

    db::Instance inst;
    db::ICplxTrans trans;
    db::cell_index_type cell_index;
    db::cell_index_type initial_cell_index;
  };

  struct QueryCellResult
  {
    QueryCellResult (db::cell_index_type ci, db::cell_index_type pci)
      : cell_index (ci), parent_cell_index (pci)
    { }

    db::cell_index_type cell_index;
    db::cell_index_type parent_cell_index;
  };

  SearchReplaceResults ();

  void clear ();
  void push_back (const tl::Variant &v);
  void push_back (const QueryShapeResult &v);
  void push_back (const QueryInstResult &v);
  void push_back (const QueryCellResult &v);
  void begin_changes (const db::Layout *layout);
  void end_changes ();

  const std::vector<tl::Variant> &data () const
  {
    return m_data_result;
  }

  const std::vector<QueryShapeResult> &shapes () const
  {
    return m_shape_result;
  }

  const std::vector<QueryInstResult> &instances () const
  {
    return m_inst_result;
  }

  const std::vector<QueryCellResult> &cells () const
  {
    return m_cell_result;
  }

  int columnCount (const QModelIndex &parent) const;
  QVariant data (const QModelIndex &index, int role) const;
  Qt::ItemFlags flags (const QModelIndex &index) const;
  bool hasChildren (const QModelIndex &parent) const;
  bool hasIndex (int row, int column, const QModelIndex &parent) const;
  QVariant headerData (int section, Qt::Orientation orientation, int role) const;
  QModelIndex index (int row, int column, const QModelIndex &parent) const;
  QModelIndex parent (const QModelIndex &index) const;
  int rowCount (const QModelIndex &parent) const;
  void has_more (bool hm);

  void export_csv (const std::string &file);
  void export_layout (db::Layout &layout);
  void export_rdb (rdb::Database &rdb, double dbu);

private:
  std::vector<tl::Variant> m_data_result;
  std::vector<QueryShapeResult> m_shape_result;
  std::vector<QueryInstResult> m_inst_result;
  std::vector<QueryCellResult> m_cell_result;
  size_t m_data_columns;
  mutable int m_last_column_count;
  std::map<db::cell_index_type, std::string> m_cellname_map;
  std::map<unsigned int, db::LayerProperties> m_lp_map;
  bool m_has_more;

  size_t size () const;
};

class SearchReplaceDialog
  : public lay::Browser,
    private Ui::SearchReplaceDialog
{
Q_OBJECT 

public:
  enum window_type { DontChange = 0, FitCell, FitMarker, Center, CenterSize };

  struct SavedQuery
  {
    std::string description;
    std::string text;
  };

  SearchReplaceDialog (lay::Dispatcher *root, lay::LayoutViewBase *view);
  ~SearchReplaceDialog ();

private:
  //  implementation of the lay::Plugin interface
  virtual bool configure (const std::string &name, const std::string &value);

  //  implementation of the lay::Plugin interface
  void menu_activated (const std::string &symbol);

  lay::LayoutViewBase *mp_view;
  std::vector<std::string> m_mru;
  std::vector<SavedQuery> m_saved;
  int m_current_mode;

  window_type m_window;
  lay::Margin m_window_dim;
  unsigned int m_max_item_count;
  std::vector<lay::MarkerBase *> mp_markers;

  std::string m_find_query;
  std::string m_last_query;
  int m_last_query_cv_index;
  std::string m_execute_query;

  SearchReplaceResults m_model;

private slots:
  void find_all_button_clicked ();
  void delete_button_clicked ();
  void delete_all_button_clicked ();
  void replace_button_clicked ();
  void replace_all_button_clicked ();
  void configure_button_clicked ();
  void execute_all_button_clicked ();
  void execute_selected_button_clicked ();
  void add_saved_button_clicked ();
  void replace_saved_button_clicked ();
  void delete_saved_button_clicked ();
  void rename_saved_button_clicked ();
  void tab_index_changed (int index);
  void saved_query_double_clicked ();
  void recent_query_index_changed (int);
  void result_selection_changed ();
  void header_columns_changed (int from, int to);
  void cancel ();
  void cancel_exec ();
  void export_csv ();
  void export_rdb ();
  void export_layout ();

private:
  std::string build_find_expression (QStackedWidget *prop_page, QComboBox *context);
  std::string build_delete_expression ();
  std::string build_replace_expression ();
  void update_saved_list ();
  void update_mru_list ();
  void restore_state ();
  void save_state ();
  void issue_query (const std::string &q, const std::set<size_t> *selected_items, bool with_results);
  void update_results (const std::string &q);
  void remove_markers ();
  bool fill_model (const db::LayoutQuery &lq, db::LayoutQueryIterator &iq, const db::Layout *layout, bool all);
  bool query_to_model (SearchReplaceResults &model, const db::LayoutQuery &lq, db::LayoutQueryIterator &iq, size_t max_item_count, bool all);
  void attach_layout (db::Layout *layout);
  void layout_changed ();

  //  implementation of the lay::Browser interface
  virtual void activated ();
  virtual void deactivated ();
};

}

#endif

