
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

#include "gsiDecl.h"
#include "dbNetlist.h"
#include "dbNetlistSpiceWriter.h"
#include "dbNetlistWriter.h"

namespace gsi
{

/**
 *  @brief A SPICE writer delegate base class for reimplementation
 */
class NetlistSpiceWriterDelegateImpl
  : public db::NetlistSpiceWriterDelegate, public gsi::ObjectBase
{
public:
  NetlistSpiceWriterDelegateImpl ()
    : db::NetlistSpiceWriterDelegate ()
  {
    //  .. nothing yet ..
  }

  virtual void write_header () const
  {
    if (cb_write_header.can_issue ()) {
      cb_write_header.issue<db::NetlistSpiceWriterDelegate> (&db::NetlistSpiceWriterDelegate::write_header);
    } else {
      db::NetlistSpiceWriterDelegate::write_header ();
    }
  }

  virtual void write_device_intro (const db::DeviceClass &ccls) const
  {
    reimpl_write_device_intro (const_cast<db::DeviceClass &> (ccls));
  }

  //  NOTE: we pass non-const refs to Ruby/Python - everything else is a bit of a nightmare.
  //  Still that's not really clean. Just say, the implementation promises not to change the objects.
  void reimpl_write_device_intro (db::DeviceClass &cls) const
  {
    if (cb_write_device_intro.can_issue ()) {
      cb_write_device_intro.issue<NetlistSpiceWriterDelegateImpl, db::DeviceClass &> (&NetlistSpiceWriterDelegateImpl::org_write_device_intro, const_cast<db::DeviceClass &> (cls));
    } else {
      org_write_device_intro (cls);
    }
  }

  void org_write_device_intro (db::DeviceClass &cls) const
  {
    db::NetlistSpiceWriterDelegate::write_device_intro (cls);
  }

  virtual void write_device (const db::Device &cdev) const
  {
    reimpl_write_device (const_cast<db::Device &> (cdev));
  }

  //  NOTE: we pass non-const refs to Ruby/Python - everthing else is a bit of a nightmare.
  //  Still that's not really clean. Just say, the implementation promises not to change the objects.
  void reimpl_write_device (db::Device &dev) const
  {
    if (cb_write_device.can_issue ()) {
      cb_write_device.issue<NetlistSpiceWriterDelegateImpl, db::Device &> (&NetlistSpiceWriterDelegateImpl::org_write_device, dev);
    } else {
      org_write_device (dev);
    }
  }

  void org_write_device (db::Device &dev) const
  {
    db::NetlistSpiceWriterDelegate::write_device (dev);
  }

  void org_write_header () const
  {
    db::NetlistSpiceWriterDelegate::write_header ();
  }

  gsi::Callback cb_write_header;
  gsi::Callback cb_write_device_intro;
  gsi::Callback cb_write_device;
};

Class<NetlistSpiceWriterDelegateImpl> db_NetlistSpiceWriterDelegate ("db", "NetlistSpiceWriterDelegate",
  gsi::method ("write_header", &NetlistSpiceWriterDelegateImpl::org_write_header, "@hide") +
  gsi::callback ("write_header", &NetlistSpiceWriterDelegateImpl::write_header, &NetlistSpiceWriterDelegateImpl::cb_write_header,
    "@brief Writes the text at the beginning of the SPICE netlist\n"
    "Reimplement this method to insert your own text at the beginning of the file"
  ) +
  gsi::method ("write_device_intro", &NetlistSpiceWriterDelegateImpl::org_write_device_intro, "@hide") +
  gsi::callback ("write_device_intro", &NetlistSpiceWriterDelegateImpl::reimpl_write_device_intro, &NetlistSpiceWriterDelegateImpl::cb_write_device_intro, gsi::arg ("device_class"),
    "@brief Inserts a text for the given device class\n"
    "Reimplement this method to insert your own text at the beginning of the file for the given device class"
  ) +
  gsi::method ("write_device", &NetlistSpiceWriterDelegateImpl::org_write_device, gsi::arg ("device"), "@hide") +
  gsi::callback ("write_device", &NetlistSpiceWriterDelegateImpl::reimpl_write_device, &NetlistSpiceWriterDelegateImpl::cb_write_device, gsi::arg ("device"),
    "@brief Inserts a text for the given device\n"
    "Reimplement this method to write the given device in the desired way. "
    "The default implementation will utilize the device class information to write native SPICE "
    "elements for the devices."
  ) +
  gsi::method ("emit_comment", &NetlistSpiceWriterDelegateImpl::emit_comment, gsi::arg ("comment"),
    "@brief Writes the given comment into the file"
  ) +
  gsi::method ("emit_line", &NetlistSpiceWriterDelegateImpl::emit_line, gsi::arg ("line"),
    "@brief Writes the given line into the file"
  ) +
  gsi::method ("net_to_string", &NetlistSpiceWriterDelegateImpl::net_to_string, gsi::arg ("net"),
    "@brief Gets the node ID for the given net\n"
    "The node ID is a numeric string instead of the full name of the net. Numeric IDs are used within "
    "SPICE netlist because they are usually shorter.\n"
  ) +
  gsi::method ("format_name", &NetlistSpiceWriterDelegateImpl::format_name, gsi::arg ("name"),
    "@brief Formats the given name in a SPICE-compatible way"
  ),
  "@brief Provides a delegate for the SPICE writer for doing special formatting for devices\n"
  "Supply a customized class to provide a specialized writing scheme for devices. "
  "You need a customized class if you want to implement special devices or you want to use "
  "subcircuits rather than the built-in devices.\n"
  "\n"
  "See \\NetlistSpiceWriter for more details.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

namespace {

class NetlistSpiceWriterWithOwnership
  : public db::NetlistSpiceWriter
{
public:
  NetlistSpiceWriterWithOwnership (NetlistSpiceWriterDelegateImpl *delegate)
    : db::NetlistSpiceWriter (delegate), m_ownership (delegate)
  {
    if (delegate) {
      delegate->keep ();
    }
  }

private:
  tl::shared_ptr<NetlistSpiceWriterDelegateImpl> m_ownership;
};

}

db::NetlistSpiceWriter *new_spice_writer (const std::string &profile)
{
  auto *writer = new db::NetlistSpiceWriter ();
  writer->set_profile (profile);
  return writer;
}

db::NetlistSpiceWriter *new_spice_writer2 (NetlistSpiceWriterDelegateImpl *delegate, const std::string &profile)
{
  auto *writer = new NetlistSpiceWriterWithOwnership (delegate);
  writer->set_profile (profile);
  return writer;
}

Class<db::NetlistWriter> db_NetlistWriter ("db", "NetlistWriter",
  gsi::Methods (),
  "@brief Base class for netlist writers\n"
  "This class is provided as a base class for netlist writers. It is not intended for reimplementation on script level, but used internally as an interface.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::NetlistSpiceWriter> db_NetlistSpiceWriter (db_NetlistWriter, "db", "NetlistSpiceWriter",
  gsi::constructor ("new", &new_spice_writer, gsi::arg ("profile", std::string ()),
    "@brief Creates a new writer without delegate.\n"
    "The profile string gives the name of the SPICE profile to use, when taking the SPICE representation "
    "from the device classes in the netlist.\n"
    "\n"
    "The profile argument has been added in version 0.31.0."
  ) +
  gsi::constructor ("new", &new_spice_writer2, gsi::arg ("delegate"), gsi::arg ("profile", std::string ()),
    "@brief Creates a new writer with a delegate.\n"
    "The profile string gives the name of the SPICE profile to use, when taking the SPICE representation "
    "from the device classes in the netlist.\n"
    "\n"
    "The profile argument has been added in version 0.31.0."
  ) +
  gsi::method ("not_connect_prefix", &db::NetlistSpiceWriter::not_connect_prefix,
    "@brief Gets the prefix used for terminals or pins which are not connected.\n"
    "See \\not_connect_prefix= for details.\n"
    "\n"
    "This attribute has been introduced in version 0.31.0."
  ) +
  gsi::method ("not_connect_prefix=", &db::NetlistSpiceWriter::set_not_connect_prefix, gsi::arg ("s"),
    "@brief Sets the prefix used for terminals or pins which are not connected.\n"
    "By default, the prefix is 'nc_'."
    "\n"
    "This attribute has been introduced in version 0.31.0."
  ) +
  gsi::method ("allowed_name_characters", &db::NetlistSpiceWriter::allowed_name_chars,
    "@brief Gets a string listing the allowed characters for names (beside alphanumeric).\n"
    "See \\allowed_name_characters= for details.\n"
    "\n"
    "This attribute has been introduced in version 0.31.0."
  ) +
  gsi::method ("allowed_name_characters=", &db::NetlistSpiceWriter::set_allowed_name_chars, gsi::arg ("s"),
    "@brief Sets a string listing the allowed characters for names (beside alphanumeric).\n"
    "\n"
    "This attribute has been introduced in version 0.31.0."
  ) +
  gsi::method ("use_net_names?", &db::NetlistSpiceWriter::use_net_names,
    "@brief Gets a value indicating whether to use net names (true) or net numbers (false).\n"
  ) +
  gsi::method ("use_net_names=", &db::NetlistSpiceWriter::set_use_net_names, gsi::arg ("f"),
    "@brief Sets a value indicating whether to use net names (true) or net numbers (false).\n"
    "The default is to use net numbers."
  ) +
  gsi::method ("use_net_names?", &db::NetlistSpiceWriter::use_net_names,
    "@brief Gets a value indicating whether to use net names (true) or net numbers (false).\n"
  ) +
  gsi::method ("with_comments=", &db::NetlistSpiceWriter::set_with_comments, gsi::arg ("f"),
    "@brief Sets a value indicating whether to embed comments for position etc. (true) or not (false).\n"
    "The default is to embed comments."
  ) +
  gsi::method ("with_comments?", &db::NetlistSpiceWriter::with_comments,
    "@brief Gets a value indicating whether to embed comments for position etc. (true) or not (false).\n"
  ),
  "@brief Implements a netlist writer for the SPICE format.\n"
  "Provide a delegate for customizing the way devices are written.\n"
  "\n"
  "Use the SPICE writer like this:\n"
  "\n"
  "@code\n"
  "writer = RBA::NetlistSpiceWriter::new\n"
  "netlist.write(path, writer)\n"
  "@/code\n"
  "\n"
  "You can give a custom description for the headline:\n"
  "\n"
  "@code\n"
  "writer = RBA::NetlistSpiceWriter::new\n"
  "netlist.write(path, writer, \"A custom description\")\n"
  "@/code\n"
  "\n"
  "To customize the output, you can use a device writer delegate.\n"
  "The delegate is an object of a class derived from \\NetlistSpiceWriterDelegate which "
  "reimplements several methods to customize the following parts:\n"
  "\n"
  "@ul\n"
  "@li A global header (\\NetlistSpiceWriterDelegate#write_header): this method is called to print the part right after the headline @/li\n"
  "@li A per-device class header (\\NetlistSpiceWriterDelegate#write_device_intro): this method is called for every device class and may print device-class specific headers (e.g. model definitions) @/li\n"
  "@li Per-device output: this method (\\NetlistSpiceWriterDelegate#write_device): this method is called for every device and may print the device statement(s) in a specific way. @/li\n"
  "@/ul\n"
  "\n"
  "The delegate must use \\NetlistSpiceWriterDelegate#emit_line to print a line, \\NetlistSpiceWriterDelegate#emit_comment to print a comment etc.\n"
  "For more method see \\NetlistSpiceWriterDelegate.\n"
  "\n"
  "A sample with a delegate is this:\n"
  "\n"
  "@code\n"
  "class MyDelegate < RBA::NetlistSpiceWriterDelegate\n"
  "\n"
  "  def write_header\n"
  "    emit_line(\"*** My special header\")\n"
  "  end\n"
  "\n"
  "  def write_device_intro(cls)\n"
  "    emit_comment(\"My intro for class \" + cls.name)\n"
  "  end\n"
  "\n"
  "  def write_device(dev)\n"
  "    if dev.device_class.name != \"MYDEVICE\"\n"
  "      emit_comment(\"Terminal #1: \" + net_to_string(dev.net_for_terminal(0)))\n"
  "      emit_comment(\"Terminal #2: \" + net_to_string(dev.net_for_terminal(1)))\n"
  "      super(dev)\n"
  "      emit_comment(\"After device \" + dev.expanded_name)\n"
  "    else\n"
  "      super(dev)\n"
  "    end\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "# write the netlist with delegate:\n"
  "writer = RBA::NetlistSpiceWriter::new(MyDelegate::new)\n"
  "netlist.write(path, writer)\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.26. "
  "SPICE profiles have been added to device classes in version 0.31.0."
);

}

