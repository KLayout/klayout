
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


#include "rdb.h"
#include "rdbReader.h"
#include "rdbCommon.h"

#include "tlTimer.h"
#include "tlProgress.h"
#include "tlXMLParser.h"
#include "tlClassRegistry.h"

#include <fstream>
#include <string>

namespace rdb
{

struct ValueConverter
{
  typedef std::string pb_type;

  ValueConverter ()
  {
  }

  std::string to_string (const ValueWrapper &value) const
  {
    return value.to_string ();
  }

  void from_string (const std::string &s, ValueWrapper &value) const
  {
    value.from_string (s);
  }

  std::string pb_encode (const ValueWrapper &value) const
  {
    return value.to_string ();
  }

  void pb_decode (const std::string &s, ValueWrapper &value) const
  {
    value.from_string (s);
  }
};

//  generation of the RDB file XML structure
static tl::XMLStruct <rdb::Database> 
make_rdb_structure ()
{
  static
  tl::XMLElementList categories_format =
    tl::make_element_with_parent_ref<rdb::Category, rdb::Categories::const_iterator, rdb::Categories> (&rdb::Categories::begin, &rdb::Categories::end, &rdb::Categories::import_category, "category#1",
      tl::make_member<std::string, rdb::Category> (&rdb::Category::name, &rdb::Category::set_name, "name#1") +
      tl::make_member<std::string, rdb::Category> (&rdb::Category::description, &rdb::Category::set_description, "description#2") +
      tl::make_element_with_parent_ref<rdb::Categories, rdb::Category> (&rdb::Category::sub_categories, &rdb::Category::import_sub_categories, "categories#3",
        &categories_format
      )
    )
  ;

  return tl::XMLStruct <rdb::Database>("report-database#0",
    tl::make_member<std::string, rdb::Database> (&rdb::Database::description, &rdb::Database::set_description, "description#1") +
    tl::make_member<std::string, rdb::Database> (&rdb::Database::original_file, &rdb::Database::set_original_file, "original-file#2") +
    tl::make_member<std::string, rdb::Database> (&rdb::Database::generator, &rdb::Database::set_generator, "generator#3") +
    tl::make_member<std::string, rdb::Database> (&rdb::Database::top_cell_name, &rdb::Database::set_top_cell_name, "top-cell#4") +
    tl::make_element<rdb::Tags, rdb::Database> (&rdb::Database::tags, &rdb::Database::import_tags, "tags#5",
      tl::make_element<rdb::Tag, rdb::Tags::const_iterator, rdb::Tags> (&rdb::Tags::begin_tags, &rdb::Tags::end_tags, &rdb::Tags::import_tag, "tag#1",
        tl::make_member<std::string, rdb::Tag> (&rdb::Tag::name, &rdb::Tag::set_name, "name#1") +
        tl::make_member<std::string, rdb::Tag> (&rdb::Tag::description, &rdb::Tag::set_description, "description#2")
      ) 
    ) + 
    tl::make_element_with_parent_ref<rdb::Categories, rdb::Database> (&rdb::Database::categories, &rdb::Database::import_categories, "categories#6",
      &categories_format
    ) +
    tl::make_element_with_parent_ref<rdb::Cells, rdb::Database> (&rdb::Database::cells, &rdb::Database::import_cells, "cells#7",
      // must be sorted cells (children after parents)!
      tl::make_element_with_parent_ref<rdb::Cell, rdb::Cells::const_iterator, rdb::Cells> (&rdb::Cells::begin, &rdb::Cells::end, &rdb::Cells::import_cell, "cell#1",
        tl::make_member<std::string, rdb::Cell> (&rdb::Cell::name, &rdb::Cell::set_name, "name#1") +
        tl::make_member<std::string, rdb::Cell> (&rdb::Cell::variant, &rdb::Cell::set_variant, "variant#2") +
        tl::make_member<std::string, rdb::Cell> (&rdb::Cell::layout_name, &rdb::Cell::set_layout_name, "layout-name#3") +
        tl::make_element_with_parent_ref<rdb::References, rdb::Cell> (&rdb::Cell::references, &rdb::Cell::import_references, "references#4",
          tl::make_element_with_parent_ref<rdb::Reference, rdb::References::const_iterator, rdb::References> (&rdb::References::begin, &rdb::References::end, &rdb::References::insert, "ref#1",
            tl::make_member<std::string, rdb::Reference> (&rdb::Reference::parent_cell_qname, &rdb::Reference::set_parent_cell_qname, "parent#1") +
            tl::make_member<std::string, rdb::Reference> (&rdb::Reference::trans_str, &rdb::Reference::set_trans_str, "trans#2")
          )
        )
      )
    ) + 
    tl::make_element_with_parent_ref<rdb::Items, rdb::Database> (&rdb::Database::items, &rdb::Database::set_items, "items#8",
      tl::make_element_with_parent_ref<rdb::Item, rdb::Items::const_iterator, rdb::Items> (&rdb::Items::begin, &rdb::Items::end, &rdb::Items::add_item, "item#1",
        tl::make_member<std::string, rdb::Item> (&rdb::Item::tag_str, &rdb::Item::set_tag_str, "tags#1") +
        tl::make_member<std::string, rdb::Item> (&rdb::Item::category_name, &rdb::Item::set_category_name, "category#2") +
        tl::make_member<std::string, rdb::Item> (&rdb::Item::cell_qname, &rdb::Item::set_cell_qname, "cell#3") +
        tl::make_member<bool, rdb::Item> (&rdb::Item::visited, &rdb::Item::set_visited, "visited#4") +
        tl::make_member<size_t, rdb::Item> (&rdb::Item::multiplicity, &rdb::Item::set_multiplicity, "multiplicity#5") +
        tl::make_member<std::string, rdb::Item> (&rdb::Item::comment, &rdb::Item::set_comment, "comment#6") +
        tl::make_member<std::string, rdb::Item> (&rdb::Item::image_str, &rdb::Item::set_image_str, "image#7") +
        tl::make_element<rdb::Values, rdb::Item> (&rdb::Item::values, &rdb::Item::set_values, "values#8",
          tl::make_member<rdb::ValueWrapper, rdb::Values::const_iterator, rdb::Values> (&rdb::Values::begin, &rdb::Values::end, &rdb::Values::add, "value#1", ValueConverter ())        )
      )
    )
  );
}

static tl::XMLStruct <rdb::Database> s_rdb_struct = make_rdb_structure ();
static tl::RegisteredClass<tl::XMLElementBase> rdb_struct_def (&s_rdb_struct, 0, "KLayout-RDB", false);

// -------------------------------------------------------------
//  Implementation of rdb::Database::save and write
//  TODO: move this somewhere else - with generalized functionality

void
rdb::Database::save (const std::string &fn, bool binary)
{
  write (fn, binary);
  set_filename (fn);
  set_binary (binary);
}

void
rdb::Database::write (const std::string &fn, bool binary)
{
  tl::OutputStream os (fn, tl::OutputStream::OM_Auto);

  if (binary) {

    tl::ProtocolBufferWriter writer (os);
    s_rdb_struct.write (writer, *this);

    if (tl::verbosity () >= 10) {
      tl::log << tl::to_string (tr ("Saved binary RDB to ")) << fn;
    }

  } else {

    s_rdb_struct.write (os, *this);

    if (tl::verbosity () >= 10) {
      tl::log << tl::to_string (tr ("Saved RDB to ")) << fn;
    }

  }
}

// -------------------------------------------------------------
//  Implementation of rdb::Database::load and the standard file plugin

class StandardReader 
  : public ReaderBase
{
public:
  StandardReader (tl::InputStream &stream, bool binary)
    : m_input_stream (stream), m_binary (binary)
  {
    // .. nothing yet ..
  }

  virtual void read (Database &db) 
  {
    tl::SelfTimer timer (tl::verbosity () >= 11, "Reading marker database file");

    if (m_binary) {

      tl::ProtocolBufferReader reader (m_input_stream);
      s_rdb_struct.parse (reader, db);
      db.set_binary (true);

    } else {

      tl::XMLStreamSource in (m_input_stream, tl::to_string (tr ("Reading RDB")));
      s_rdb_struct.parse (in, db);
      db.set_binary (false);

    }
  }

  virtual const char *format () const 
  {
    return m_binary ? "KLayout-RDB-PB" : "KLayout-RDB";
  }

private:
  tl::InputStream &m_input_stream;
  bool m_binary;
};

class StandardFormatDeclaration 
  : public FormatDeclaration
{
  virtual std::string format_name () const { return "KLayout-RDB"; }
  virtual std::string format_desc () const { return "KLayout report database format"; }
  virtual std::string file_format () const { return "KLayout RDB files (*.lyrdb *.lyrdb.gz)"; }

  virtual bool detect (tl::InputStream &stream) const
  {
    tl::TextInputStream text_stream (stream);

    // TODO: this assumes ASCII or UTF-8 files and does not consider comments containing that string ..
    int n = 0;
    while (! text_stream.at_end () && n < 100) {
      if (text_stream.get_line ().find ("<report-database>") != std::string::npos) {
        return true;
      }
      ++n;
    }

    return false;
  }

  virtual ReaderBase *create_reader (tl::InputStream &s) const 
  {
    return new StandardReader (s, false);
  }
};

static tl::RegisteredClass<rdb::FormatDeclaration> format_decl (new StandardFormatDeclaration (), 0, "KLayout-RDB");

class BinaryFormatDeclaration
  : public FormatDeclaration
{
  virtual std::string format_name () const { return "KLayout-RDB-PB"; }
  virtual std::string format_desc () const { return "KLayout binary report database format"; }
  virtual std::string file_format () const { return "KLayout binary RDB files (*.rdb *.rdb.gz)"; }

  virtual bool detect (tl::InputStream &stream) const
  {
    static const char header[] = {
      //  ProtocolBuffer wire format, LEN record with ID 0 and string "report-database".
      0x02, 0x0f, 0x72, 0x65, 0x70, 0x6f, 0x72, 0x74, 0x2d, 0x64, 0x61, 0x74, 0x61, 0x62, 0x61, 0x73, 0x65
    };

    const char *h = stream.get (sizeof (header));
    if (! h) {
      return false;
    }

    for (size_t i = 0; i < sizeof (header); ++i) {
      if (h[i] != header[i]) {
        return false;
      }
    }

    return true;
  }

  virtual ReaderBase *create_reader (tl::InputStream &s) const
  {
    return new StandardReader (s, true);
  }
};

static tl::RegisteredClass<rdb::FormatDeclaration> pb_format_decl (new BinaryFormatDeclaration (), 1, "KLayout-RDB-PB");

}
