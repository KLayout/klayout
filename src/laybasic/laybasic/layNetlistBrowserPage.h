
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#ifndef HDR_layNetlistBrowserPage
#define HDR_layNetlistBrowserPage

#include "ui_NetlistBrowserPage.h"
#include "layNetlistBrowser.h"
#include "dbBox.h"

#include <QFrame>

class QAction;

namespace db
{
  class LayoutToNetlist;
}

namespace lay
{

class LayoutView;
class PluginRoot;

/**
 *  @brief A marker browser page
 */
class NetlistBrowserPage
  : public QFrame,
    public Ui::NetlistBrowserPage
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  NetlistBrowserPage (QWidget *parent);

  /**
   *  @brief Destructor
   */
  ~NetlistBrowserPage ();

  /**
   *  @brief Sets the plugin root object for this object
   */
  void set_plugin_root (lay::PluginRoot *pr);

  /**
   *  @brief Attaches the page to a view
   *
   *  This method can be given a layout view object.
   *  If that pointer is non-null, the browser will attach itself to
   *  the view and provide highlights for the selected markers inside the given cellview.
   */
  void set_view (lay::LayoutView *view, unsigned int cv_index);

  /**
   *  @brief Attach the page to a L2N DB
   *
   *  To detach the page from any L2N DB, pass 0 for the pointer.
   */
  void set_l2ndb (db::LayoutToNetlist *database);


  /**
   *  @brief Set the window type and window dimensions
   */
  void set_window (lay::NetlistBrowserConfig::net_window_type window_type, double window_dim, lay::NetlistBrowserConfig::net_context_mode_type context);

  /**
   *  @brief Update the net highlights
   *
   *  This method should be called if the cellview has changed so the highlights can
   *  be recomputed and shown in the new cell context.
   */
  void update_highlights ();

  /**
   *  @brief Set the maximum number of shapes highlighted for a net
   */
  void set_max_shape_count (size_t max_shape_count);

  /**
   *  @brief Set the highlight style
   *
   *  @param color The color or an invalid color to take the default color for selection
   *  @param line_width The line width or negative for the default line width
   *  @param vertex_size The vertex size or negative for the default vertex size
   *  @param halo The halo flag or -1 for default
   *  @param dither_pattern The dither pattern index of -1 to take the default
   */
  void set_highlight_style (QColor color, int line_width, int vertex_size, int halo, int dither_pattern);

  /**
   *  @brief Gets a value indicating whether all items in the netlist tree are shown (specifically for cross-reference DBs)
   */
  bool show_all () const
  {
    return m_show_all;
  }

  /**
   *  @brief Sets a value indicating whether all items in the netlist tree are shown (specifically for cross-reference DBs)
   *
   *  If this property is set to false, only cross-reference entries with error are shown.
   */
  void show_all (bool f);

  /**
   *  @brief Enable or disable updates
   */
  void enable_updates (bool f);

private slots:
  void show_all_clicked ();
  void filter_changed ();

private:
  bool m_show_all;
  QAction *m_show_all_action;
  NetlistBrowserConfig::net_context_mode_type m_context;
  NetlistBrowserConfig::net_window_type m_window;
  double m_window_dim;
  size_t m_max_shape_count;
  QColor m_marker_color;
  int m_marker_line_width;
  int m_marker_vertex_size;
  int m_marker_halo;
  int m_marker_dither_pattern;
  lay::LayoutView *mp_view;
  unsigned int m_cv_index;
  lay::PluginRoot *mp_plugin_root;
};

} // namespace lay

#endif

