
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

#ifndef HDR_laySaltManagerDialog
#define HDR_laySaltManagerDialog

#include "ui_SaltManagerDialog.h"

#include <QDialog>
#include <memory>

namespace lay
{

class Salt;
class SaltGrain;
class SaltGrainPropertiesDialog;

/**
 *  @brief The dialog for managing the Salt ("Packages")
 */
class SaltManagerDialog
  : public QDialog, private Ui::SaltManagerDialog
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  SaltManagerDialog (QWidget *parent);

private slots:
  /**
   *  @brief Called when the list of packages (grains) has changed
   */
  void salt_changed ();

  /**
   *  @brief Called when the repository (salt mine) has changed
   */
  void salt_mine_changed ();

  /**
   *  @brief Called when the currently selected package (grain) has changed
   */
  void current_changed ();

  /**
   *  @brief Called when the currently selected package from the salt mine has changed
   */
  void mine_current_changed ();

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

private:
  lay::Salt *mp_salt, *mp_salt_mine;
  std::auto_ptr<lay::SaltGrain> m_remote_grain;
  bool m_current_changed_enabled;
  lay::SaltGrainPropertiesDialog *mp_properties_dialog;

  lay::SaltGrain *current_grain ();
  lay::SaltGrain *mine_current_grain ();
};

}

#endif
