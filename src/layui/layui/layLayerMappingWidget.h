
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

#ifndef HDR_layLayerMappingWidget
#define HDR_layLayerMappingWidget

#include <QFrame>

#include "layuiCommon.h"
#include "dbStreamLayers.h"

namespace Ui
{
  class LayerMappingWidget;
}

namespace lay
{

class FileDialog;

/**
 *  @brief A widget for editing the layer mapping for the reader options
 */
class LAYUI_PUBLIC LayerMappingWidget
  : public QFrame
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  LayerMappingWidget (QWidget *parent);

  /**
   *  @brief Destructor
   */
  ~LayerMappingWidget ();

  /**
   *  @brief Set the layer mapping
   */
  void set_layer_map (const db::LayerMap &lm);

  /**
   *  @brief Get the layer mapping
   */
  db::LayerMap get_layer_map () const;

  /**
   *  @brief Get a value indicating whether the layer list is empty
   */
  bool is_empty () const;

signals:
  void layerListChanged ();
  void layerItemDeleted ();
  void layerItemAdded ();
  void enable_all_layers (bool en);

private slots:
  void load_button_pressed ();
  void add_button_pressed ();
  void delete_button_pressed ();
  void edit_button_pressed ();
  void current_tab_changed (int tab);

private:
  lay::FileDialog *mp_layer_table_file_dialog;
  std::string m_layer_table_file;
  Ui::LayerMappingWidget *mp_ui;

  db::LayerMap get_layer_map_from_tab (int tab) const;
};

} // namespace lay

#endif

#endif  //  defined(HAVE_QT)
