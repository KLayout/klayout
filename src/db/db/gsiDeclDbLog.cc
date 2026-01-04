
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

#include "gsiDecl.h"
#include "gsiEnums.h"
#include "dbLog.h"
#include "dbNet.h"
#include "dbCircuit.h"

namespace gsi
{

db::LogEntryData *new_le1 (db::Severity severity, const std::string &msg)
{
  return new db::LogEntryData (severity, msg);
}

db::LogEntryData *new_le2 (db::Severity severity, const std::string &cell_name, const std::string &msg)
{
  return new db::LogEntryData (severity, cell_name, msg);
}

db::LogEntryData *new_le3 (db::Severity severity, const std::string &cell_name, const std::string &net_name, const std::string &msg)
{
  return new db::LogEntryData (severity, cell_name, net_name, msg);
}

db::LogEntryData *new_le4 (db::Severity severity, const db::Net *net, const std::string &msg)
{
  if (! net || ! net->circuit ()) {
    return new db::LogEntryData (severity, msg);
  } else {
    return new db::LogEntryData (severity, net->circuit ()->name (), net->expanded_name (), msg);
  }
}

Class<db::LogEntryData> decl_dbNetlistDeviceExtractorError ("db", "LogEntryData",
  gsi::constructor ("new", &new_le1, gsi::arg ("severity"), gsi::arg ("msg"),
    "@brief Creates a new LogEntry object with the given severity and message\n"
    "This convenience constructor has been added in version 0.30.6\n"
  ) +
  gsi::constructor ("new", &new_le2, gsi::arg ("severity"), gsi::arg ("cell_name"), gsi::arg ("msg"),
    "@brief Creates a new LogEntry object with the given severity, cell or circuit name and message\n"
    "This convenience constructor has been added in version 0.30.6\n"
  ) +
  gsi::constructor ("new", &new_le3, gsi::arg ("severity"), gsi::arg ("cell_name"), gsi::arg ("new_name"), gsi::arg ("msg"),
    "@brief Creates a new LogEntry object with the given severity, cell or circuit name, net name and message\n"
    "This convenience constructor has been added in version 0.30.6\n"
  ) +
  gsi::constructor ("new", &new_le4, gsi::arg ("severity"), gsi::arg ("net"), gsi::arg ("msg"),
    "@brief Creates a new LogEntry object with the given severity and message and circuit and net name taken from the given \\Net object\n"
    "This convenience constructor has been added in version 0.30.6\n"
  ) +
  gsi::method ("severity", &db::LogEntryData::severity,
    "@brief Gets the severity attribute.\n"
  ) +
  gsi::method ("severity=", &db::LogEntryData::set_severity, gsi::arg ("severity"),
    "@brief Sets the severity attribute.\n"
  ) +
  gsi::method ("message", &db::LogEntryData::message,
    "@brief Gets the message text.\n"
  ) +
  gsi::method ("message=", &db::LogEntryData::set_message, gsi::arg ("message"),
    "@brief Sets the message text.\n"
  ) +
  gsi::method ("cell_name", &db::LogEntryData::cell_name,
    "@brief Gets the cell name.\n"
    "See \\cell_name= for details about this attribute."
  ) +
  gsi::method ("cell_name=", &db::LogEntryData::set_cell_name, gsi::arg ("cell_name"),
    "@brief Sets the cell name.\n"
    "The cell (or circuit) name specifies the cell or circuit the "
    "log entry is related to. If the log entry is an error or "
    "warning generated during device extraction, the cell name is "
    "the circuit the device should have appeared in."
  ) +
  gsi::method ("net_name", &db::LogEntryData::net_name,
    "@brief Gets the net name.\n"
    "See \\net_name= for details about this attribute."
    "\n"
    "The net_name attribute has been introduced in version 0.30.6.\n"
  ) +
  gsi::method ("net_name=", &db::LogEntryData::set_net_name, gsi::arg ("net_name"),
    "@brief Sets the net name.\n"
    "The net (or circuit) name specifies the net the "
    "log entry is related to.\n"
    "\n"
    "By convention, the net name is the expanded net name (see \\Net#expanded_name).\n"
    "\n"
    "The net_name attribute has been introduced in version 0.30.6.\n"
  ) +
  gsi::method ("geometry", &db::LogEntryData::geometry,
    "@brief Gets the geometry.\n"
    "See \\geometry= for more details."
  ) +
  gsi::method ("geometry=", &db::LogEntryData::set_geometry, gsi::arg ("polygon"),
    "@brief Sets the geometry.\n"
    "The geometry is optional. If given, a marker may be shown when selecting this error."
  ) +
  gsi::method ("category_name", &db::LogEntryData::category_name,
    "@brief Gets the category name.\n"
    "See \\category_name= for more details."
  ) +
  gsi::method ("category_name=", &db::LogEntryData::set_category_name, gsi::arg ("name"),
    "@brief Sets the category name.\n"
    "The category name is optional. If given, it specifies a formal category name. Errors with the same "
    "category name are shown in that category. If in addition a category description is specified "
    "(see \\category_description), this description will be displayed as the title."
  ) +
  gsi::method ("category_description", &db::LogEntryData::category_description,
    "@brief Gets the category description.\n"
    "See \\category_name= for details about categories."
  ) +
  gsi::method ("category_description=", &db::LogEntryData::set_category_description, gsi::arg ("description"),
    "@brief Sets the category description.\n"
    "See \\category_name= for details about categories."
  ) +
  gsi::method ("to_s", &db::LogEntryData::to_string, gsi::arg ("with_geometry", true),
    "@brief Gets the string representation of this error or warning.\n"
    "This method has been introduced in version 0.28.13."
  ),
  "@brief A generic log entry\n"
  "This class is used for example by the device extractor (see \\NetlistDeviceExtractor) to keep errors or warnings "
  "that occurred during extraction of the devices.\n"
  "\n"
  "Other classes also make use of this object to store errors, warnings or information. "
  "The log entry object features a severity (warning, error, info), a message, an optional "
  "category name and description (good for filtering if needed) and an optional \\DPolygon object "
  "for indicating some location or error marker."
  "\n"
  "The original class used to be \"NetlistDeviceExtractorError\" which had been introduced in version 0.26. "
  "It was generalized and renamed in version 0.28.13 as it was basically not useful as a separate class."
);

const gsi::Enum<db::Severity> &get_decl_Severity ()
{
  static gsi::Enum<db::Severity> decl_Severity ("db", "Severity",
    gsi::enum_const ("NoSeverity", db::NoSeverity,
      "@brief Specifies no particular severity (default)\n"
    ) +
    gsi::enum_const ("Warning", db::Warning,
      "@brief Specifies warning severity (log with high priority, but do not stop)\n"
    ) +
    gsi::enum_const ("Error", db::Error,
      "@brief Specifies error severity (preferred action is stop)\n"
    ) +
    gsi::enum_const ("Info", db::Info,
      "@brief Specifies info severity (print if requested, otherwise silent)\n"
    ),
    "@brief This enum specifies the severity level for log entries.\n"
    "\n"
    "This enum was introduced in version 0.28.13.\n"
  );

  return decl_Severity;
}

gsi::ClassExt<db::LogEntryData> inject_SeverityEnum_into_LogEntryData (get_decl_Severity ().defs ());

}
