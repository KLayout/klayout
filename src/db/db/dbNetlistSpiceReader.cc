
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

#include "dbNetlistSpiceReader.h"
#include "dbNetlistSpiceReaderExpressionParser.h"
#include "dbNetlistSpiceReaderDelegate.h"
#include "dbNetlist.h"

#include "tlUri.h"
#include "tlFileUtils.h"
#include "tlLog.h"
#include "tlTimer.h"

#include <string>
#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <cmath>

namespace db
{

// ------------------------------------------------------------------------------------------------------

static const char *allowed_name_chars = "_.:,!+$/&\\#[]|<>";

// ------------------------------------------------------------------------------------------------------

void
read_param_card (tl::Extractor &ex, const db::Netlist *netlist, std::map<std::string, tl::Variant> &variables)
{
  //  Syntax is:
  //    .param <name> = <value> [ <name> = <value> ... ]
  //  taken from:
  //    https://nmg.gitlab.io/ngspice-manual/circuitdescription/paramparametricnetlists/paramline.html

  while (! ex.at_end ()) {

    std::string name;
    ex.read_word (name);

    name = netlist->normalize_name (name);

    ex.test ("=");

    tl::Variant value = NetlistSpiceReaderExpressionParser (&variables).read (ex);
    variables [name] = value;

  }
}

// ------------------------------------------------------------------------------------------------------

class SpiceReaderStream
{
public:
  SpiceReaderStream ();
  SpiceReaderStream (const std::string &lib);
  ~SpiceReaderStream ();

  void set_stream (tl::InputStream &stream);
  void set_stream (tl::InputStream *stream);
  void close ();

  std::pair<std::string, bool> get_line();
  int line_number () const;
  std::string source () const;
  bool at_end () const;
  const std::string &lib () const { return m_lib; }

  void swap (SpiceReaderStream &other)
  {
    std::swap (m_lib, other.m_lib);
    std::swap (mp_stream, other.mp_stream);
    std::swap (m_owns_stream, other.m_owns_stream);
    std::swap (mp_text_stream, other.mp_text_stream);
    std::swap (m_line_number, other.m_line_number);
    std::swap (m_stored_line, other.m_stored_line);
    std::swap (m_has_stored_line, other.m_has_stored_line);
  }

private:
  tl::InputStream *mp_stream;
  bool m_owns_stream;
  tl::TextInputStream *mp_text_stream;
  int m_line_number;
  std::string m_stored_line;
  bool m_has_stored_line;
  std::string m_lib;
};


SpiceReaderStream::SpiceReaderStream ()
  : mp_stream (0), m_owns_stream (false), mp_text_stream (0), m_line_number (0), m_stored_line (), m_has_stored_line (false)
{
  //  .. nothing yet ..
}

SpiceReaderStream::SpiceReaderStream (const std::string &lib)
  : mp_stream (0), m_owns_stream (false), mp_text_stream (0), m_line_number (0), m_stored_line (), m_has_stored_line (false), m_lib (lib)
{
  //  .. nothing yet ..
}

SpiceReaderStream::~SpiceReaderStream ()
{
  close ();
}

void
SpiceReaderStream::close ()
{
  delete mp_text_stream;
  mp_text_stream = 0;

  if (m_owns_stream) {
    delete mp_stream;
    mp_stream = 0;
    m_owns_stream = false;
  }
}

std::pair<std::string, bool>
SpiceReaderStream::get_line ()
{
  if (at_end ()) {
    return std::make_pair (std::string (), false);
  }

  ++m_line_number;

  std::string l = m_has_stored_line ? m_stored_line : mp_text_stream->get_line ();

  m_has_stored_line = false;
  m_stored_line.clear ();

  while (! mp_text_stream->at_end ()) {

    std::string ll = mp_text_stream->get_line ();

    tl::Extractor ex (ll.c_str ());
    if (! ex.test ("+")) {
      m_stored_line = ll;
      m_has_stored_line = true;
      break;
    } else {
      ++m_line_number;
      l += " ";
      l += ex.get ();
    }

  }

  return std::make_pair (l, true);
}

int
SpiceReaderStream::line_number () const
{
  return m_line_number;
}

std::string
SpiceReaderStream::source () const
{
  return mp_stream->source ();
}

bool
SpiceReaderStream::at_end () const
{
  return !m_has_stored_line && mp_text_stream->at_end ();
}

void
SpiceReaderStream::set_stream (tl::InputStream &stream)
{
  close ();
  mp_stream = &stream;
  mp_text_stream = new tl::TextInputStream (stream);
  m_owns_stream = false;
  m_has_stored_line = false;
  m_line_number = 0;
}

void
SpiceReaderStream::set_stream (tl::InputStream *stream)
{
  close ();
  mp_stream = stream;
  mp_text_stream = new tl::TextInputStream (*stream);
  m_owns_stream = true;
  m_has_stored_line = false;
  m_line_number = 0;
}

// ------------------------------------------------------------------------------------------------------

struct SpiceCard
{
  SpiceCard (int _file_id, int _line, const std::string &_text)
    : file_id (_file_id), line (_line), text (_text)
  { }

  int file_id;
  int line;
  std::string text;
};

class SpiceCachedCircuit
{
public:
  typedef std::list<SpiceCard> cards_type;
  typedef cards_type::const_iterator cards_iterator;
  typedef NetlistSpiceReader::parameters_type parameters_type;
  typedef std::vector<std::string> pin_list_type;
  typedef pin_list_type::const_iterator pin_const_iterator;

  SpiceCachedCircuit (const std::string &name)
    : m_name (name), m_anonymous (false)
  {
    //  .. nothing yet ..
  }

  bool is_anonymous () const
  {
    return m_anonymous;
  }

  void set_anonymous (bool f)
  {
    m_anonymous = f;
  }

  const std::string &name () const
  {
    return m_name;
  }

  void set_parameters (const parameters_type &pv)
  {
    m_parameters = pv;
  }

  const parameters_type &parameters () const
  {
    return m_parameters;
  }

  void make_parameter (const std::string &name, const tl::Variant &value)
  {
    for (auto p = m_pins.begin (); p != m_pins.end (); ++p) {
      if (*p == name) {
        //  remove pin and make parameter
        m_pins.erase (p);
        break;
      }
    }

    m_parameters [name] = value;
  }

  cards_iterator begin_cards () const
  {
    return m_cards.begin ();
  }

  cards_iterator end_cards () const
  {
    return m_cards.end ();
  }

  void add_card (const SpiceCard &card)
  {
    m_cards.push_back (card);
  }

  size_t pin_count () const
  {
    return m_pins.size ();
  }

  pin_const_iterator begin_pins () const
  {
    return m_pins.begin ();
  }

  pin_const_iterator end_pins () const
  {
    return m_pins.end ();
  }

  void set_pins (const pin_list_type &pins)
  {
    m_pins = pins;
  }

  void set_pins (pin_list_type &&pins)
  {
    m_pins = std::move (pins);
  }

private:
  std::string m_name;
  parameters_type m_parameters;
  pin_list_type m_pins;
  cards_type m_cards;
  bool m_anonymous;
};

static std::string
read_name (tl::Extractor &ex, const db::Netlist *netlist)
{
  std::string n;
  ex.read_word_or_quoted (n, allowed_name_chars);
  return netlist->normalize_name (n);
}

class SpiceCircuitDict
{
public:
  typedef NetlistSpiceReader::parameters_type parameters_type;
  typedef std::map<std::string, SpiceCachedCircuit *> circuits_type;
  typedef circuits_type::const_iterator circuits_iterator;
  typedef std::vector<std::string> global_nets_type;
  typedef global_nets_type::const_iterator global_nets_iterator;

  SpiceCircuitDict (NetlistSpiceReader *reader, Netlist *netlist, NetlistSpiceReaderDelegate *delegate);
  ~SpiceCircuitDict ();

  void read (tl::InputStream &stream);
  void finish ();

  circuits_iterator begin_circuits () const
  {
    return m_cached_circuits.begin ();
  }

  circuits_iterator end_circuits () const
  {
    return m_cached_circuits.end ();
  }

  bool is_top_circuit (const std::string &name) const
  {
    return m_called_circuits.find (name) == m_called_circuits.end ();
  }

  const SpiceCachedCircuit *anonymous_top_level_circuit () const
  {
    return mp_anonymous_top_level_circuit;
  }

  global_nets_iterator begin_global_nets () const
  {
    return m_global_nets.begin ();
  }

  global_nets_iterator end_global_nets () const
  {
    return m_global_nets.end ();
  }

  const std::string &file_path (int file_id) const;

  const SpiceCachedCircuit *cached_circuit (const std::string &name) const;
  SpiceCachedCircuit *create_cached_circuit (const std::string &name);

private:
  NetlistSpiceReader *mp_reader;
  Netlist *mp_netlist;
  tl::weak_ptr<NetlistSpiceReaderDelegate> mp_delegate;
  std::vector<std::string> m_paths;
  std::map<std::string, int> m_file_id_per_path;
  std::list<SpiceReaderStream> m_streams;
  std::list<std::string> m_in_lib;
  SpiceReaderStream m_stream;
  int m_file_id;
  std::map<std::string, SpiceCachedCircuit *> m_cached_circuits;
  SpiceCachedCircuit *mp_circuit;
  SpiceCachedCircuit *mp_anonymous_top_level_circuit;
  std::set<std::string> m_called_circuits;
  NetlistSpiceReader::parameters_type m_variables;
  std::set<std::string> m_global_net_names;
  std::vector<std::string> m_global_nets;

  void push_stream (const std::string &path, const std::string &lib = std::string ());
  void pop_stream ();
  bool at_end ();
  void read_subcircuit (const std::string &sc_name, const std::string &nc_name, const std::vector<db::Net *> &nets);
  void read_circuit (tl::Extractor &ex, const std::string &name);
  bool read_card ();
  void read_options (tl::Extractor &ex);
  void ensure_circuit ();
  std::string get_line ();
  void error (const std::string &msg);
  void warn (const std::string &msg);
  int file_id (const std::string &path);
};

SpiceCircuitDict::SpiceCircuitDict (NetlistSpiceReader *reader, Netlist *netlist, NetlistSpiceReaderDelegate *delegate)
  : mp_reader (reader), mp_netlist (netlist), mp_delegate (delegate)
{
  m_file_id = -1;
  mp_circuit = mp_anonymous_top_level_circuit = 0;
}

SpiceCircuitDict::~SpiceCircuitDict ()
{
  for (auto c = m_cached_circuits.begin (); c != m_cached_circuits.end (); ++c) {
    delete c->second;
  }
  m_cached_circuits.clear ();

  mp_reader = 0;
  mp_delegate = 0;
}

const std::string &
SpiceCircuitDict::file_path (int file_id) const
{
  if (file_id < 0 || file_id > int (m_paths.size ())) {
    static std::string empty;
    return empty;
  } else {
    return m_paths [file_id];
  }
}

int
SpiceCircuitDict::file_id (const std::string &path)
{
  auto ip = m_file_id_per_path.find (path);
  if (ip != m_file_id_per_path.end ()) {
    return ip->second;
  }

  int id = int (m_paths.size ());
  m_file_id_per_path.insert (std::make_pair (path, id));
  m_paths.push_back (path);
  return id;
}

const SpiceCachedCircuit *
SpiceCircuitDict::cached_circuit (const std::string &name) const
{
  auto c = m_cached_circuits.find (name);
  return c == m_cached_circuits.end () ? 0 : c->second;
}

SpiceCachedCircuit *
SpiceCircuitDict::create_cached_circuit (const std::string &name)
{
  auto c = m_cached_circuits.find (name);
  if (c != m_cached_circuits.end ()) {
    return c->second;
  }

  SpiceCachedCircuit *cc = new SpiceCachedCircuit (name);
  m_cached_circuits.insert (std::make_pair (name, cc));
  return cc;
}

void
SpiceCircuitDict::read (tl::InputStream &stream)
{
  try {

    m_stream.set_stream (stream);

    mp_circuit = 0;
    mp_anonymous_top_level_circuit = 0;
    m_called_circuits.clear ();
    m_variables.clear ();
    m_global_net_names.clear ();
    m_global_nets.clear ();

    m_file_id = file_id (stream.source ());

    while (! at_end ()) {
      read_card ();
    }

  } catch (tl::Exception &ex) {

    //  Add a location to the exception
    std::string fmt_msg = ex.msg () + tl::sprintf (tl::to_string (tr (" in %s, line %d")), m_stream.source (), m_stream.line_number ());
    throw tl::Exception (fmt_msg);

  }
}

void
SpiceCircuitDict::push_stream (const std::string &path, const std::string &lib)
{
  tl::URI current_uri (m_stream.source ());
  tl::URI new_uri (path);

  tl::InputStream *istream;
  if (current_uri.scheme ().empty () && new_uri.scheme ().empty ()) {
    if (tl::is_absolute (path)) {
      istream = new tl::InputStream (path);
    } else {
      istream = new tl::InputStream (tl::combine_path (tl::dirname (m_stream.source ()), path));
    }
  } else {
    istream = new tl::InputStream (current_uri.resolved (new_uri).to_abstract_path ());
  }

  m_streams.push_back (SpiceReaderStream (lib));
  m_streams.back ().swap (m_stream);
  m_stream.set_stream (istream);

  m_file_id = file_id (m_stream.source ());
}

void
SpiceCircuitDict::pop_stream ()
{
  if (! m_streams.empty ()) {

    m_stream.swap (m_streams.back ());
    m_streams.pop_back ();

    m_file_id = file_id (m_stream.source ());

  }
}

bool
SpiceCircuitDict::at_end ()
{
  return m_stream.at_end () && m_streams.empty ();
}

void
SpiceCircuitDict::error (const std::string &msg)
{
  throw tl::Exception (msg);
}

void
SpiceCircuitDict::warn (const std::string &msg)
{
  std::string fmt_msg = tl::sprintf ("%s in %s, line %d", msg, m_stream.source (), m_stream.line_number ());
  tl::warn << fmt_msg;
}

std::string
SpiceCircuitDict::get_line ()
{
  std::pair<std::string, bool> lp;

  while (true) {

    lp = m_stream.get_line ();
    if (! lp.second) {

      if (m_streams.empty ()) {
        break;
      } else {
        if (! m_stream.lib ().empty ()) {
          m_in_lib.pop_back ();
        }
        pop_stream ();
      }

    } else {

      bool consider_line = m_in_lib.empty () || (! m_stream.lib ().empty () && m_stream.lib () == m_in_lib.back ());

      tl::Extractor ex (lp.first.c_str ());
      if (ex.test_without_case (".include") || ex.test_without_case (".inc")) {

        std::string path;
        ex.read_word_or_quoted (path, allowed_name_chars);

        if (consider_line) {
          std::string libname = m_stream.lib ();
          push_stream (path, libname);
          if (! libname.empty ()) {
            m_in_lib.push_back (libname);
          }
        }

        ex.expect_end ();

      } else if (ex.test_without_case (".lib")) {

        std::string path_or_libname;

        ex.read_word_or_quoted (path_or_libname, allowed_name_chars);
        if (! ex.at_end ()) {

          std::string libname;
          ex.read_word_or_quoted (libname, allowed_name_chars);

          if (consider_line) {
            libname = mp_netlist->normalize_name (libname);
            push_stream (path_or_libname, libname);
            if (! libname.empty ()) {
              m_in_lib.push_back (std::string ());
            }
          }

        } else {

          std::string libname = mp_netlist->normalize_name (path_or_libname);
          m_in_lib.push_back (libname);
          ex.expect_end ();

        }

      } else if (ex.test_without_case (".endl")) {

        if (! m_in_lib.empty ()) {
          m_in_lib.pop_back ();
        } else {
          warn (tl::to_string (tr ("Ignoring .endl without .lib")));
        }

        ex.expect_end ();

      } else if (ex.at_end () || ex.test ("*")) {

        //  skip empty and comment lines

      } else if (consider_line) {
        break;
      }

    }

  }

  return lp.first;
}

bool
SpiceCircuitDict::read_card ()
{
  std::string l = get_line ();
  if (l.empty ()) {
    return false;
  }

  tl::Extractor ex (l.c_str ());
  std::string name;

  if (ex.test_without_case (".model")) {

    //  ignore model statements

  } else if (ex.test_without_case (".global")) {

    while (! ex.at_end ()) {
      std::string n = mp_delegate->translate_net_name (read_name (ex, mp_netlist));
      if (m_global_net_names.find (n) == m_global_net_names.end ()) {
        m_global_nets.push_back (n);
        m_global_net_names.insert (n);
      }
    }

  } else if (ex.test_without_case (".subckt")) {

    std::string nc = read_name (ex, mp_netlist);
    read_circuit (ex, nc);

  } else if (ex.test_without_case (".options")) {

    read_options (ex);

  } else if (ex.test_without_case (".ends")) {

    return true;

  } else if (ex.test_without_case (".end")) {

    //  ignore end statements

  } else if (ex.test_without_case (".param")) {

    read_param_card (ex, mp_netlist, m_variables);

    ensure_circuit ();
    mp_circuit->add_card (SpiceCard (m_file_id, m_stream.line_number (), l));

  } else if (ex.test (".")) {

    if (! mp_delegate->control_statement (l)) {

      std::string s;
      ex.read_word (s);
      s = tl::to_lower_case (s);
      warn (tl::to_string (tr ("Control statement ignored: ")) + s);

    }

  } else if (ex.try_read_word (name)) {

    ensure_circuit ();

    if (ex.test ("=")) {

      name = mp_netlist->normalize_name (name);

      tl::Variant value = NetlistSpiceReaderDelegate::read_value (ex, m_variables);
      m_variables [name] = value;

      mp_circuit->make_parameter (name, value);

    } else if (name[0] == 'X') {

      //  register circuit calls so we can figure out the top level circuits

      tl::Extractor ex2 (l.c_str ());
      ex2.skip ();
      ++ex2;

      std::vector<std::string> nn;
      parameters_type pv;
      mp_delegate->parse_element_components (ex2.get (), nn, pv, m_variables);

      if (! nn.empty ()) {
        m_called_circuits.insert (nn.back ());
      }

    }

    mp_circuit->add_card (SpiceCard (m_file_id, m_stream.line_number (), l));

  } else {
    warn (tl::to_string (tr ("Line ignored")));
  }

  return false;
}

void
SpiceCircuitDict::read_options (tl::Extractor &ex)
{
  while (! ex.at_end ()) {

    std::string n;
    ex.read_word_or_quoted (n, allowed_name_chars);
    n = tl::to_lower_case (n);

    double v = 0.0;
    std::string w;
    if (ex.test ("=")) {
      if (ex.try_read (v)) {
        //  take value
      } else {
        //  skip until end or next space
        ex.skip ();
        while (! ex.at_end () && ! isspace (*ex)) {
          ++ex;
        }
      }
    }

    //  TODO: further options?
    const double min_value = 1e-18;
    if (n == "scale") {
      if (v > min_value) {
        mp_delegate->options ().scale = v;
      }
    } else if (n == "defad") {
      if (v > min_value) {
        mp_delegate->options ().defad = v;
      }
    } else if (n == "defas") {
      if (v > min_value) {
        mp_delegate->options ().defas = v;
      }
    } else if (n == "defl") {
      if (v > min_value) {
        mp_delegate->options ().defl = v;
      }
    } else if (n == "defw") {
      if (v > min_value) {
        mp_delegate->options ().defw = v;
      }
    }

  }
}

void
SpiceCircuitDict::ensure_circuit ()
{
  if (! mp_circuit) {

    //  TODO: make top name configurable
    mp_circuit = new SpiceCachedCircuit (".TOP");
    m_cached_circuits.insert (std::make_pair (mp_circuit->name (), mp_circuit));

    mp_anonymous_top_level_circuit = mp_circuit;

  }
}

void
SpiceCircuitDict::read_circuit (tl::Extractor &ex, const std::string &nc)
{
  std::vector<std::string> nn;
  NetlistSpiceReader::parameters_type pv;
  mp_delegate->parse_element_components (ex.skip (), nn, pv, m_variables);

  if (cached_circuit (nc)) {
    error (tl::sprintf (tl::to_string (tr ("Redefinition of circuit %s")), nc));
  }

  SpiceCachedCircuit *cc = create_cached_circuit (nc);
  cc->set_pins (nn);
  cc->set_parameters (pv);

  std::swap (cc, mp_circuit);
  NetlistSpiceReader::parameters_type vars = pv;
  m_variables.swap (vars);

  while (! at_end ()) {
    if (read_card ()) {
      break;
    }
  }

  std::swap (cc, mp_circuit);
  m_variables.swap (vars);
}

void
SpiceCircuitDict::finish ()
{
  m_streams.clear ();
  m_stream.close ();
}

// ------------------------------------------------------------------------------------------------------

class SpiceNetlistBuilder
{
public:
  typedef NetlistSpiceReader::parameters_type parameters_type;

  SpiceNetlistBuilder (SpiceCircuitDict *dict, Netlist *netlist, NetlistSpiceReaderDelegate *delegate);

  void set_strict (bool s)
  {
    m_strict = s;
  }

  void build ();

private:
  SpiceCircuitDict *mp_dict;
  tl::weak_ptr<NetlistSpiceReaderDelegate> mp_delegate;
  Netlist *mp_netlist;
  bool m_strict;
  const SpiceCachedCircuit *mp_circuit;
  std::map<const SpiceCachedCircuit *, std::map<parameters_type, db::Circuit *> > m_circuits;
  db::Circuit *mp_netlist_circuit;
  db::Circuit *mp_anonymous_top_level_netlist_circuit;
  std::unique_ptr<std::map<std::string, db::Net *> > mp_nets_by_name;
  std::map<std::string, bool> m_captured;
  NetlistSpiceReader::parameters_type m_variables;
  const SpiceCard *mp_current_card;

  db::Circuit *circuit_for (const SpiceCachedCircuit *cached_circuit, const parameters_type &pv);
  void register_circuit_for (const SpiceCachedCircuit *cc, const parameters_type &pv, db::Circuit *circuit, bool anonymous_top_level);
  Circuit *build_circuit (const SpiceCachedCircuit *circuit, const parameters_type &pv, bool anonymous_top_level = false);
  std::string get_line ();
  void error (const std::string &msg);
  void warn (const std::string &msg);
  Net *make_net(const std::string &name);
  void process_card (const SpiceCard &card);
  bool subcircuit_captured (const std::string &nc_name);
  bool process_element (tl::Extractor &ex, const std::string &prefix, const std::string &name);
  void build_global_nets ();
};

SpiceNetlistBuilder::SpiceNetlistBuilder (SpiceCircuitDict *dict, Netlist *netlist, NetlistSpiceReaderDelegate *delegate)
  : mp_dict (dict), mp_delegate (delegate), mp_netlist (netlist), m_strict (true)
{
  mp_circuit = 0;
  mp_netlist_circuit = 0;
  mp_anonymous_top_level_netlist_circuit = 0;
  mp_current_card = 0;
}

void
SpiceNetlistBuilder::error (const std::string &msg)
{
  throw tl::Exception (msg);
}

void
SpiceNetlistBuilder::warn (const std::string &msg)
{
  if (mp_current_card) {
    std::string fmt_msg = tl::sprintf ("%s in %s, line %d", msg, mp_dict->file_path (mp_current_card->file_id), mp_current_card->line);
    tl::warn << fmt_msg;
  } else {
    tl::warn << msg;
  }
}

db::Circuit *
SpiceNetlistBuilder::circuit_for (const SpiceCachedCircuit *cc, const parameters_type &pv)
{
  auto c = m_circuits.find (cc);
  if (c == m_circuits.end ()) {
    return 0;
  }
  auto cp = c->second.find (pv);
  if (cp == c->second.end ()) {
    return 0;
  }
  return cp->second;
}

void
SpiceNetlistBuilder::register_circuit_for (const SpiceCachedCircuit *cc, const parameters_type &pv, db::Circuit *circuit, bool anonymous_top_level)
{
  m_circuits [cc][pv] = circuit;
  if (anonymous_top_level) {
    mp_anonymous_top_level_netlist_circuit = circuit;
  }
}

void
SpiceNetlistBuilder::build ()
{
  try {

    m_variables.clear ();
    mp_netlist_circuit = 0;
    mp_anonymous_top_level_netlist_circuit = 0;
    mp_circuit = 0;
    mp_current_card = 0;
    m_captured.clear ();

    mp_delegate->do_start ();

    for (auto c = mp_dict->begin_circuits (); c != mp_dict->end_circuits (); ++c) {
      if (mp_dict->is_top_circuit (c->first) && ! subcircuit_captured (c->first)) {
        //  we have a top circuit candidate
        build_circuit (c->second, c->second->parameters (), c->second == mp_dict->anonymous_top_level_circuit ());
      }
    }

    build_global_nets ();
    mp_delegate->do_finish ();

  } catch (tl::Exception &ex) {

    //  add a source location to the exception
    if (mp_current_card) {
      std::string fmt_msg = ex.msg () + tl::sprintf (tl::to_string (tr (" in %s, line %d")), mp_dict->file_path (mp_current_card->file_id), mp_current_card->line);
      throw tl::Exception (fmt_msg);
    } else {
      throw;
    }

  }
}

static std::string
make_circuit_name (const std::string &name, const NetlistSpiceReader::parameters_type &pv)
{
  std::string res = name;

  res += "(";
  for (auto p = pv.begin (); p != pv.end (); ++p) {
    if (p != pv.begin ()) {
      res += ",";
    }
    res += p->first;
    res += "=";
    if (p->second.can_convert_to_double()) {
      double v = p->second.to_double ();
      double va = fabs (v);
      if (va < 1e-15) {
        res += tl::sprintf ("%g", v);
      } else if (va < 0.1e-12) {
        res += tl::sprintf ("%gF", v * 1e15);
      } else if (va < 0.1e-9) {
        res += tl::sprintf ("%gP", v * 1e12);
      } else if (va < 0.1e-6) {
        res += tl::sprintf ("%gN", v * 1e9);
      } else if (va < 0.1e-3) {
        res += tl::sprintf ("%gU", v * 1e6);
      } else if (va< 0.1) {
        res += tl::sprintf ("%gM", v * 1e3);
      } else if (va < 0.1e3) {
        res += tl::sprintf ("%g", v);
      } else if (va < 0.1e6) {
        res += tl::sprintf ("%gK", v * 1e-3);
      } else if (va < 0.1e9) {
        res += tl::sprintf ("%gMEG", v * 1e-6);
      } else if (va < 0.1e12) {
        res += tl::sprintf ("%gG", v * 1e-9);
      } else {
        res += tl::sprintf ("%g", v);
      }
    } else {
      res += p->second.to_string ();
    }
  }
  res += ")";

  return res;
}

db::Circuit *
SpiceNetlistBuilder::build_circuit (const SpiceCachedCircuit *cc, const parameters_type &pv, bool anonymous_top_level)
{
  db::Circuit *c = circuit_for (cc, pv);
  if (c) {
    return c;
  }

  c = new db::Circuit ();
  mp_netlist->add_circuit (c);
  if (pv.empty ()) {
    c->set_name (cc->name ());
  } else {
    c->set_name (make_circuit_name (cc->name (), pv));
  }

  register_circuit_for (cc, pv, c, anonymous_top_level);

  std::unique_ptr<std::map<std::string, db::Net *> > n2n (mp_nets_by_name.release ());
  mp_nets_by_name.reset (0);

  NetlistSpiceReader::parameters_type vars = cc->parameters ();
  for (auto p = pv.begin (); p != pv.end (); ++p) {
    vars [p->first] = p->second;
  }

  std::swap (vars, m_variables);
  std::swap (c, mp_netlist_circuit);
  std::swap (cc, mp_circuit);

  //  produce the explicit pins
  for (auto i = mp_circuit->begin_pins (); i != mp_circuit->end_pins (); ++i) {
    std::string net_name = mp_delegate->translate_net_name (mp_netlist->normalize_name (*i));
    db::Net *net = make_net (net_name);
    //  use the net name to name the pin (otherwise SPICE pins are always unnamed)
    size_t pin_id = i - mp_circuit->begin_pins ();
    if (! i->empty ()) {
      mp_netlist_circuit->add_pin (net->name ());
    } else {
      mp_netlist_circuit->add_pin (std::string ());
    }
    mp_netlist_circuit->connect_pin (pin_id, net);
  }

  for (auto card = mp_circuit->begin_cards (); card != mp_circuit->end_cards (); ++card) {
    mp_current_card = card.operator-> ();
    process_card (*card);
  }

  mp_current_card = 0;
  mp_nets_by_name.reset (n2n.release ());

  std::swap (cc, mp_circuit);
  std::swap (c, mp_netlist_circuit);
  std::swap (vars, m_variables);

  return c;
}

db::Net *
SpiceNetlistBuilder::make_net (const std::string &name)
{
  if (! mp_nets_by_name.get ()) {
    mp_nets_by_name.reset (new std::map<std::string, db::Net *> ());
  }

  std::map<std::string, db::Net *>::const_iterator n2n = mp_nets_by_name->find (name);

  db::Net *net = 0;
  if (n2n == mp_nets_by_name->end ()) {

    net = new db::Net ();
    net->set_name (name);
    mp_netlist_circuit->add_net (net);

    mp_nets_by_name->insert (std::make_pair (name, net));

  } else {
    net = n2n->second;
  }

  return net;
}

void
SpiceNetlistBuilder::process_card (const SpiceCard &card)
{
  tl::Extractor ex (card.text.c_str ());

  std::string name;
  if (ex.try_read_word (name) && ex.test ("=")) {

    m_variables.insert (std::make_pair (mp_netlist->normalize_name (name), NetlistSpiceReaderDelegate::read_value (ex, m_variables)));

  } else {

    ex = tl::Extractor (card.text.c_str ());
    ex.skip ();

    if (ex.test_without_case (".param")) {

      read_param_card (ex, mp_netlist, m_variables);

    } else if (isalpha (*ex)) {

      std::string prefix;
      prefix.push_back (toupper (*ex));

      ++ex;
      name = read_name (ex, mp_netlist);

      if (! process_element (ex, prefix, name)) {
        warn (tl::sprintf (tl::to_string (tr ("Element type '%s' ignored")), prefix));
      }

    } else {
      warn (tl::to_string (tr ("Line ignored")));
    }

  }
}

bool
SpiceNetlistBuilder::subcircuit_captured (const std::string &nc_name)
{
  std::map<std::string, bool>::const_iterator c = m_captured.find (nc_name);
  if (c != m_captured.end ()) {
    return c->second;
  } else {
    bool cap = mp_delegate->wants_subcircuit (nc_name);
    m_captured.insert (std::make_pair (nc_name, cap));
    return cap;
  }
}

bool
SpiceNetlistBuilder::process_element (tl::Extractor &ex, const std::string &prefix, const std::string &name)
{
  //  generic parse
  std::vector<std::string> nn;
  NetlistSpiceReader::parameters_type pv;
  std::string model;
  double value = 0.0;

  mp_delegate->parse_element (ex.skip (), prefix, model, value, nn, pv, m_variables);

  model = mp_netlist->normalize_name (model);

  std::vector<db::Net *> nets;
  for (std::vector<std::string>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
    nets.push_back (make_net (mp_delegate->translate_net_name (*i)));
  }

  if (prefix == "X" && ! subcircuit_captured (model)) {

    const db::SpiceCachedCircuit *cc = mp_dict->cached_circuit (model);
    if (! cc) {
      if (m_strict) {
        error (tl::sprintf (tl::to_string (tr ("Subcircuit '%s' not found in netlist")), model));
      } else {
        db::SpiceCachedCircuit *cc_nc = mp_dict->create_cached_circuit (model);
        cc_nc->set_anonymous (true);
        cc = cc_nc;
        std::vector<std::string> pins;
        pins.resize (nn.size ());
        cc_nc->set_pins (pins);
      }
    }

    if (! cc->is_anonymous ()) {
      //  issue warnings on unknown parameters which are skipped otherwise
      for (auto p = pv.begin (); p != pv.end (); ++p) {
        if (cc->parameters ().find (p->first) == cc->parameters ().end ()) {
          warn (tl::sprintf (tl::to_string (tr ("Not a known parameter for circuit '%s': '%s'")), cc->name (), p->first));
        }
      }
    }

    if (cc->pin_count () != nn.size ()) {
      error (tl::sprintf (tl::to_string (tr ("Pin count mismatch between circuit definition and circuit call: %d expected, got %d")), int (cc->pin_count ()), int (nets.size ())));
    }

    db::Circuit *c = build_circuit (cc, pv);

    db::SubCircuit *sc = new db::SubCircuit (c, name);
    mp_netlist_circuit->add_subcircuit (sc);

    for (std::vector<db::Net *>::const_iterator i = nets.begin (); i != nets.end (); ++i) {
      sc->connect_pin (i - nets.begin (), *i);
    }

    return true;

  } else {
    return mp_delegate->element (mp_netlist_circuit, prefix, name, model, value, nets, pv);
  }
}

void
SpiceNetlistBuilder::build_global_nets ()
{
  for (auto gn = mp_dict->begin_global_nets (); gn != mp_dict->end_global_nets (); ++gn) {

    for (auto c = mp_netlist->begin_bottom_up (); c != mp_netlist->end_bottom_up (); ++c) {

      if (c.operator-> () == mp_anonymous_top_level_netlist_circuit) {
        //  no pins for the anonymous top circuit
        continue;
      }

      db::Net *net = c->net_by_name (*gn);
      if (! net || net->pin_count () > 0) {
        //  only add a pin for a global net if there is a net with this name
        //  don't add a pin if it already has one
        continue;
      }

      const db::Pin &pin = c->add_pin (*gn);
      c->connect_pin (pin.id (), net);

      for (db::Circuit::refs_iterator r = c->begin_refs (); r != c->end_refs (); ++r) {

        db::SubCircuit &sc = *r;

        db::Net *pnet = sc.circuit ()->net_by_name (*gn);
        if (! pnet) {
          pnet = new db::Net ();
          pnet->set_name (*gn);
          sc.circuit ()->add_net (pnet);
        }

        sc.connect_pin (pin.id (), pnet);

      }

    }

  }
}

// ------------------------------------------------------------------------------------------------------

NetlistSpiceReader::NetlistSpiceReader (NetlistSpiceReaderDelegate *delegate)
  : mp_delegate (delegate), m_strict (false)
{
  if (! delegate) {
    mp_default_delegate.reset (new NetlistSpiceReaderDelegate ());
    mp_delegate.reset (mp_default_delegate.get ());
  }
}

NetlistSpiceReader::~NetlistSpiceReader ()
{
  //  .. nothing yet ..
}

void NetlistSpiceReader::read (tl::InputStream &stream, db::Netlist &netlist)
{
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Reading netlist ")) + stream.source ());

  try {

    mp_delegate->set_netlist (&netlist);

    //  SPICE netlists are case insensitive
    netlist.set_case_sensitive (false);

    SpiceCircuitDict dict (this, &netlist, mp_delegate.get ());
    try {
      dict.read (stream);
      dict.finish ();
    } catch (...) {
      dict.finish ();
      throw;
    }

    SpiceNetlistBuilder builder (&dict, &netlist, mp_delegate.get ());
    builder.set_strict (m_strict);
    builder.build ();

    mp_delegate->set_netlist (0);

  } catch (...) {
    mp_delegate->set_netlist (0);
    throw;
  }
}

}
