
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
  ValueConverter (rdb::Database *rdb)
    : mp_rdb (rdb)
  {
  }

  std::string to_string (const ValueWrapper &value) const
  {
    return value.to_string (mp_rdb);
  }

  void from_string (const std::string &s, ValueWrapper &value) const
  {
    value.from_string (mp_rdb, s);
  }

private:
  rdb::Database *mp_rdb;
};

static 
tl::XMLElementList categories_format = 
  tl::make_element_with_parent_ref<rdb::Category, rdb::Categories::const_iterator, rdb::Categories> (&rdb::Categories::begin, &rdb::Categories::end, &rdb::Categories::import_category, "category", 
    tl::make_member<std::string, rdb::Category> (&rdb::Category::name, &rdb::Category::set_name, "name") + 
    tl::make_member<std::string, rdb::Category> (&rdb::Category::description, &rdb::Category::set_description, "description") +
    tl::make_element_with_parent_ref<rdb::Categories, rdb::Category> (&rdb::Category::sub_categories, &rdb::Category::import_sub_categories, "categories",
      &categories_format
    ) 
  ) 
;

//  generation of the RDB file XML structure
static tl::XMLStruct <rdb::Database> 
make_rdb_structure (rdb::Database *rdb)
{
  return tl::XMLStruct <rdb::Database>("report-database", 
    tl::make_member<std::string, rdb::Database> (&rdb::Database::description, &rdb::Database::set_description, "description") +
    tl::make_member<std::string, rdb::Database> (&rdb::Database::original_file, &rdb::Database::set_original_file, "original-file") +
    tl::make_member<std::string, rdb::Database> (&rdb::Database::generator, &rdb::Database::set_generator, "generator") +
    tl::make_member<std::string, rdb::Database> (&rdb::Database::top_cell_name, &rdb::Database::set_top_cell_name, "top-cell") +
    tl::make_element<rdb::Tags, rdb::Database> (&rdb::Database::tags, &rdb::Database::import_tags, "tags",
      tl::make_element<rdb::Tag, rdb::Tags::const_iterator, rdb::Tags> (&rdb::Tags::begin_tags, &rdb::Tags::end_tags, &rdb::Tags::import_tag, "tag", 
        tl::make_member<std::string, rdb::Tag> (&rdb::Tag::name, &rdb::Tag::set_name, "name") + 
        tl::make_member<std::string, rdb::Tag> (&rdb::Tag::description, &rdb::Tag::set_description, "description")
      ) 
    ) + 
    tl::make_element_with_parent_ref<rdb::Categories, rdb::Database> (&rdb::Database::categories, &rdb::Database::import_categories, "categories",
      &categories_format
    ) +
    tl::make_element_with_parent_ref<rdb::Cells, rdb::Database> (&rdb::Database::cells, &rdb::Database::import_cells, "cells",
      // must be sorted cells (children after parents)!
      tl::make_element_with_parent_ref<rdb::Cell, rdb::Cells::const_iterator, rdb::Cells> (&rdb::Cells::begin, &rdb::Cells::end, &rdb::Cells::import_cell, "cell", 
        tl::make_member<std::string, rdb::Cell> (&rdb::Cell::name, &rdb::Cell::set_name, "name") +
        tl::make_member<std::string, rdb::Cell> (&rdb::Cell::variant, &rdb::Cell::set_variant, "variant") +
        tl::make_element_with_parent_ref<rdb::References, rdb::Cell> (&rdb::Cell::references, &rdb::Cell::import_references, "references", 
          tl::make_element_with_parent_ref<rdb::Reference, rdb::References::const_iterator, rdb::References> (&rdb::References::begin, &rdb::References::end, &rdb::References::insert, "ref", 
            tl::make_member<std::string, rdb::Reference> (&rdb::Reference::parent_cell_qname, &rdb::Reference::set_parent_cell_qname, "parent") + 
            tl::make_member<std::string, rdb::Reference> (&rdb::Reference::trans_str, &rdb::Reference::set_trans_str, "trans") 
          )
        )
      )
    ) + 
    tl::make_element_with_parent_ref<rdb::Items, rdb::Database> (&rdb::Database::items, &rdb::Database::set_items, "items",
      tl::make_element_with_parent_ref<rdb::Item, rdb::Items::const_iterator, rdb::Items> (&rdb::Items::begin, &rdb::Items::end, &rdb::Items::add_item, "item", 
        tl::make_member<std::string, rdb::Item> (&rdb::Item::tag_str, &rdb::Item::set_tag_str, "tags") + 
        tl::make_member<std::string, rdb::Item> (&rdb::Item::category_name, &rdb::Item::set_category_name, "category") + 
        tl::make_member<std::string, rdb::Item> (&rdb::Item::cell_qname, &rdb::Item::set_cell_qname, "cell") +
        tl::make_member<bool, rdb::Item> (&rdb::Item::visited, &rdb::Item::set_visited, "visited") +
        tl::make_member<size_t, rdb::Item> (&rdb::Item::multiplicity, &rdb::Item::set_multiplicity, "multiplicity") +
        tl::make_member<std::string, rdb::Item> (&rdb::Item::image_str, &rdb::Item::set_image_str, "image") +
        tl::make_element<rdb::Values, rdb::Item> (&rdb::Item::values, &rdb::Item::set_values, "values", 
          tl::make_member<rdb::ValueWrapper, rdb::Values::const_iterator, rdb::Values> (&rdb::Values::begin, &rdb::Values::end, &rdb::Values::add, "value", ValueConverter (rdb)) 
        )
      )
    )
  );
}

// -------------------------------------------------------------
//  Implementation of rdb::Database::save
//  TODO: move this somewhere else - with generalized functionality

void
rdb::Database::save (const std::string &fn)
{
  tl::OutputStream os (fn, tl::OutputStream::OM_Auto);
  make_rdb_structure (this).write (os, *this); 
  set_filename (fn);

  tl::log << "Saved RDB to " << fn;
}

// -------------------------------------------------------------
//  Implementation of rdb::Database::load and the standard file plugin

class StandardReader 
  : public ReaderBase
{
public:
  StandardReader (tl::InputStream &stream)
    : m_input_stream (stream)
  {
    // .. nothing yet ..
  }

  virtual void read (Database &db) 
  {
    tl::SelfTimer timer (tl::verbosity () >= 11, "Reading marker database file");
    tl::XMLStreamSource in (m_input_stream, tl::to_string (tr ("Reading RDB")));
    make_rdb_structure (&db).parse (in, db); 
  }

  virtual const char *format () const 
  {
    return "KLayout-RDB";
  }

private:
  tl::InputStream &m_input_stream;
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
    return new StandardReader (s);
  }
};

static tl::RegisteredClass<rdb::FormatDeclaration> format_decl (new StandardFormatDeclaration (), 0, "KLayout-RDB");

}
