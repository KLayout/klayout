
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

#ifndef HDR_laySaltManagerDialog
#define HDR_laySaltManagerDialog

#include "ui_SaltManagerDialog.h"
#include "laySalt.h"
#include "tlDeferredExecution.h"
#include "tlHttpStream.h"
#include "tlException.h"

#include <QDialog>
#include <memory>
#include <vector>

namespace lay
{

class SaltGrainPropertiesDialog;

/**
 *  @brief The dialog for managing the Salt ("Packages")
 */
class SaltManagerDialog
  : public QDialog, private Ui::SaltManagerDialog, public tl::Object
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  SaltManagerDialog (QWidget *parent, lay::Salt *salt, const std::string &salt_mine_url);

  /**
   *  @brief Gets the URL for the package index
   */
  const std::string &salt_mine_url () const
  {
    return m_salt_mine_url;
  }

private:
  /**
   *  @brief Called when data is available from the grain downloader
   */
  void data_ready ();

  /**
   *  @brief Called when data is available from the salt mine downloader
   */
  void salt_mine_data_ready ();

private slots:
  /**
   *  @brief Called when the list of packages (grains) is about to change
   */
  void salt_about_to_change ();

  /**
   *  @brief Called when the list of packages (grains) has changed
   */
  void salt_changed ();

  /**
   *  @brief Called when the repository (salt mine) is about to change
   */
  void salt_mine_about_to_change ();

  /**
   *  @brief Called when the repository (salt mine) has changed
   */
  void salt_mine_changed ();

  /**
   *  @brief Called when the currently selected package (grain) has changed
   */
  void selected_changed ();

  /**
   *  @brief Called when the currently selected package from the update page has changed
   */
  void mine_update_selected_changed ();

  /**
   *  @brief Called when the currently selected package from the new installation page has changed
   */
  void mine_new_selected_changed ();

  /**
   *  @brief Called when the "edit" button is pressed
   */
  void edit_properties ();

  /**
   *  @brief Called when the "mark" button is pressed
   */
  void mark_clicked ();

  /**
   *  @brief Called when the "edit" button is pressed
   */
  void create_grain ();

  /**
   *  @brief Called when the "delete" button is pressed
   */
  void delete_grain ();

  /**
   *  @brief Called when the mode tab changed
   */
  void mode_changed ();

  /**
   *  @brief Called when the "apply" button is clicked
   */
  void apply ();

  /**
   *  @brief Called when one search text changed
   */
  void search_text_changed (const QString &text);

  /**
   *  @brief Called to show the marked items only (new packages tab)
   */
  void show_marked_only_new ();

  /**
   *  @brief Called to unmark all items (new packages tab)
   */
  void unmark_all_new ();

  /**
   *  @brief Called to mark all items (new packages tab)
   */
  void mark_all_new ();

  /**
   *  @brief Called to show the marked items only (update packages tab)
   */
  void show_marked_only_update ();

  /**
   *  @brief Called to unmark all items (update packages tab)
   */
  void unmark_all_update ();

  /**
   *  @brief Called to mark all items (update packages tab)
   */
  void mark_all_update ();

  /**
   *  @brief Reloads the salt mine
   */
  void refresh ();

private:
  Salt *mp_salt;
  Salt m_salt_mine;
  std::string m_salt_mine_url;
  SaltGrainPropertiesDialog *mp_properties_dialog;
  tl::DeferredMethod<SaltManagerDialog> dm_update_models;
  int m_current_tab;
  std::unique_ptr<tl::InputStream> m_downloaded_grain_reader;
  std::unique_ptr<lay::SaltGrain> m_downloaded_grain, m_salt_mine_grain;
  SaltGrainDetailsTextWidget *mp_downloaded_target;
  std::unique_ptr<tl::InputStream> m_salt_mine_reader;
  tl::DeferredMethod<SaltManagerDialog> dm_mine_update_selected_changed;
  tl::DeferredMethod<SaltManagerDialog> dm_mine_new_selected_changed;
  std::map<std::string, lay::SaltGrain> m_salt_grain_cache;

  SaltGrain *current_grain ();
  std::vector<lay::SaltGrain *> current_grains ();
  void set_current_grain_by_name (const std::string &current);
  void update_models ();
  void update_apply_state ();
  void get_remote_grain_info (lay::SaltGrain *g, SaltGrainDetailsTextWidget *details);
  void consolidate_salt_mine_entries ();
  void show_error (tl::Exception &ex);
  void salt_mine_download_started ();
  void salt_mine_download_finished ();
  void do_mine_update_selected_changed ();
  void do_mine_new_selected_changed ();
};

}

#endif
