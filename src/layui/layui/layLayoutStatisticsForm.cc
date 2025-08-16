
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

#include "layLayoutStatisticsForm.h"
#include "layLayoutViewBase.h"
#include "tlInternational.h"
#include "tlString.h"
#include "tlExpression.h"
#include "tlTimer.h"
#include "tlUri.h"
#include "tlFileUtils.h"
#include "dbLayoutQuery.h"
#include "dbCellGraphUtils.h"

#include <set>
#include <sstream>

#include <QBuffer>
#include <QResource>
#include <QFileInfo>
#include <QDomElement>
#include <QDomDocument>
#include <QXmlStreamWriter>
#include <QTextStream>
#if QT_VERSION >= 0x050000
#  include <QUrlQuery>
#endif

namespace lay
{

/**
 *  @brief A comparison operator to sort layers by layer and datatype and then by name
 */
struct CompareLDName 
{
  CompareLDName (const db::Layout &l) 
    : mp_layout (&l)
  {
    //  .. nothing yet ..
  }

  bool operator() (unsigned int a, unsigned int b)
  {
    if (mp_layout->is_valid_layer (a) && mp_layout->is_valid_layer (b)) {
      const db::LayerProperties &lp_a = mp_layout->get_properties (a);
      const db::LayerProperties &lp_b = mp_layout->get_properties (b);
      if (lp_a.layer != lp_b.layer) {
        return lp_a.layer < lp_b.layer;
      }
      if (lp_a.datatype != lp_b.datatype) {
        return lp_a.datatype < lp_b.datatype;
      }
      if (lp_a.name != lp_b.name) {
        return lp_a.name < lp_b.name;
      }
    }
    return false;
  }

private:
  const db::Layout *mp_layout;
};

/**
 *  @brief A comparison operator to sort layers by name and then by layer and datatype 
 */
struct CompareNameLD 
{
  CompareNameLD (const db::Layout &l) 
    : mp_layout (&l)
  {
    //  .. nothing yet ..
  }

  bool operator() (unsigned int a, unsigned int b)
  {
    if (mp_layout->is_valid_layer (a) && mp_layout->is_valid_layer (b)) {
      const db::LayerProperties &lp_a = mp_layout->get_properties (a);
      const db::LayerProperties &lp_b = mp_layout->get_properties (b);
      if (lp_a.name != lp_b.name) {
        return lp_a.name < lp_b.name;
      }
      if (lp_a.layer != lp_b.layer) {
        return lp_a.layer < lp_b.layer;
      }
      if (lp_a.datatype != lp_b.datatype) {
        return lp_a.datatype < lp_b.datatype;
      }
    }
    return false;
  }

private:
  const db::Layout *mp_layout;
};

static std::string format_tech_name (const std::string &s)
{
  if (s.empty ()) {
    return s;
  } else {
    return " ('" + s + "')";
  }
}

// ------------------------------------------------------------

/**
 *  @brief A template processor for creating HTML pages from a template
 *
 *  TODO: this is just a first step and far from being complete.
 *  The template processor is used from the browser page by using an extension .stxml.
 *  It reads a XML file from a resource path ":/st/<path>" and converts it into HTML.
 */
class StatisticsTemplateProcessor 
{
public:
  StatisticsTemplateProcessor (const tl::URI &url, const db::Layout *layout)
    : mp_layout (layout)
  {
    QResource res (QString::fromUtf8 (":/st/") + QString::fromUtf8 (url.path ().c_str ()));
#if QT_VERSION >= 0x60000
      if (res.compressionAlgorithm () == QResource::ZlibCompression) {
#else
      if (res.isCompressed ()) {
#endif
      m_temp = qUncompress ((const unsigned char *)res.data (), (int)res.size ());
    } else {
      m_temp = QByteArray ((const char *)res.data (), (int)res.size ());
    }

    auto query_items = url.query ();
    for (auto q = query_items.begin (); q != query_items.end (); ++q) {
      m_top_eval.set_var (q->first, q->second);
    }
  }

  bool process ();

  const QByteArray &get () const
  {
    return m_output.buffer ();
  }

private:
  QByteArray m_temp;
  QBuffer m_output;
  tl::Eval m_top_eval;
  const db::Layout *mp_layout;

  void process (const QDomElement &element, tl::Eval &eval, QXmlStreamWriter &writer);
  void process_child_nodes (const QDomElement &element, tl::Eval &eval, QXmlStreamWriter &writer);
};

bool 
StatisticsTemplateProcessor::process ()
{
  m_output.open (QIODevice::WriteOnly);

  bool ret_value = false;

  try
  {
    tl::SelfTimer timer (tl::verbosity () > 21, "StatisticsForm: create content");

    QDomDocument doc;
    doc.setContent (m_temp, true);

    QXmlStreamWriter writer (&m_output);
    writer.writeStartDocument (QString::fromUtf8 ("1.0"));
    process (doc.documentElement (), m_top_eval, writer);
    writer.writeEndDocument ();

    ret_value = true;

  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    QTextStream writer (&m_output);
    writer << tl::to_string (QObject::tr ("ERROR: evaluating template: ")).c_str () << ex.msg ().c_str ();
  }

  m_output.close ();
  return ret_value;
}

void
StatisticsTemplateProcessor::process_child_nodes (const QDomElement &element, tl::Eval &eval, QXmlStreamWriter &writer)
{
  if (element.isNull ()) {
    return;
  }

  for (QDomNode n = element.firstChild (); ! n.isNull (); n = n.nextSibling ()) {

    if (n.isElement ()) {

      process (n.toElement (), eval, writer);

    } else if (n.isCDATASection ()) {

      writer.writeCDATA (tl::to_qstring (eval.interpolate (tl::to_string (n.toCDATASection ().data ()))));

    } else if (n.isCharacterData ()) {

      QString t; 
      QTextStream s (&t);

      while (true) {

        s << n.toCharacterData ().data ();
        QDomNode nn = n.nextSibling ();
        if (nn.isNull () || ! nn.isCharacterData ()) {
          break;
        }
        n = nn;

      }

      writer.writeCharacters (tl::to_qstring (eval.interpolate (tl::to_string (t))));

    }

  }
}

void
StatisticsTemplateProcessor::process (const QDomElement &element, tl::Eval &eval, QXmlStreamWriter &writer)
{
  static QString template_namespace_uri = QString::fromUtf8 ("www.klayout.org/layout-statistics-template");
  static QString template_cmd_if = QString::fromUtf8 ("if");
  static QString template_cmd_true = QString::fromUtf8 ("true");
  static QString template_cmd_false = QString::fromUtf8 ("false");
  static QString template_cmd_eval = QString::fromUtf8 ("eval");
  static QString template_cmd_query = QString::fromUtf8 ("query");
  static QString template_cmd_begin = QString::fromUtf8 ("begin");
  static QString template_cmd_end = QString::fromUtf8 ("end");
  static QString template_cmd_max = QString::fromUtf8 ("max");
  static QString template_cmd_each = QString::fromUtf8 ("each");
  static QString template_value_true = QString::fromUtf8 ("true");
  static QString template_value_empty_query = QString::fromUtf8 ("");
  static QString template_name_expr = QString::fromUtf8 ("expr");
  static QString template_name_max = QString::fromUtf8 ("max");

  if (element.namespaceURI () == template_namespace_uri) {

    if (element.localName () == template_cmd_eval) {

      tl::Expression expr;
      eval.parse (expr, tl::to_string (element.attribute (template_name_expr, template_value_true)));
      expr.execute ();

    } else if (element.localName () == template_cmd_if) {

      QDomElement true_node, false_node;

      for (QDomNode n = element.firstChild (); ! n.isNull (); n = n.nextSibling ()) {
        QDomElement e = n.toElement ();
        if (! e.isNull () && e.namespaceURI () == template_namespace_uri) {
          if (e.localName () == template_cmd_true) {
            true_node = e;
          } else if (e.localName () == template_cmd_false) {
            false_node = e;
          }
        }
      }

      if (true_node.isNull () && false_node.isNull ()) {
        true_node = element;
      }

      tl::Expression expr;
      eval.parse (expr, tl::to_string (element.attribute (template_name_expr, template_value_true)));
      tl::Variant value = expr.execute ();

      if (value.to_bool ()) {
        if (! true_node.isNull ()) {
          process (true_node, eval, writer);
        } 
      } else {
        if (! false_node.isNull ()) {
          process (false_node, eval, writer);
        } 
      }

    } else if (element.localName () == template_cmd_query) {

      QDomElement begin_node, end_node, each_node, max_node;

      for (QDomNode n = element.firstChild (); ! n.isNull (); n = n.nextSibling ()) {
        QDomElement e = n.toElement ();
        if (! e.isNull () && e.namespaceURI () == template_namespace_uri) {
          if (e.localName () == template_cmd_begin) {
            begin_node = e;
          } else if (e.localName () == template_cmd_end) {
            end_node = e;
          } else if (e.localName () == template_cmd_max) {
            max_node = e;
          } else if (e.localName () == template_cmd_each) {
            each_node = e;
          }
        }
      }

      if (begin_node.isNull () && end_node.isNull () && max_node.isNull () && each_node.isNull ()) {
        each_node = element;
      }

      unsigned long max_count = std::numeric_limits<unsigned long>::max ();
      QString max_attr = element.attribute (template_name_max, QString ());
      if (! max_attr.isNull ()) {
        tl::Expression expr;
        eval.parse (expr, tl::to_string (max_attr));
        tl::Variant max = expr.execute ();
        if (max.can_convert_to_ulong ()) {
          max_count = max.to_ulong ();
        }
      }

      db::LayoutQuery q (tl::to_string (element.attribute (template_name_expr, template_value_empty_query)));

      db::LayoutQueryIterator qi (q, mp_layout, 0, &eval);

      process_child_nodes (begin_node, qi.eval (), writer);

      for ( ; !qi.at_end (); ++qi) {
        if (max_count == 0) {
          process_child_nodes (max_node, qi.eval (), writer);
          break;
        } else {
          --max_count;
          process_child_nodes (each_node, qi.eval (), writer);
        }
      }

      process_child_nodes (end_node, qi.eval (), writer);

    }

  } else {

    writer.writeStartElement (element.nodeName ());

    if (element.hasAttributes ()) {
      //  Hint: attribute nodes are not children of the elements ..
      QDomNamedNodeMap attributes = element.attributes ();
      for (int i = 0; i < attributes.count (); ++i) {
        QDomAttr a = attributes.item (i).toAttr ();
        if (! a.isNull ()) {
          writer.writeAttribute (a.nodeName (), tl::to_qstring (eval.interpolate (tl::to_string (a.value ()))));
        }
      }
    }

    process_child_nodes (element, eval, writer);

    writer.writeEndElement ();

  }
}

// ------------------------------------------------------------

class ShapeStatistics
{
public:
  ShapeStatistics ()
  {
    // .. nothing yet ..
  }

  void compute (const db::Shapes &shapes)
  {
    for (db::Shapes::shape_iterator i = shapes.begin (db::ShapeIterator::All); ! i.at_end (); ++i) {
      size_t n = 1;
      if (i.in_array ()) {
        n = i.array ().array_size ();
        m_count [i.array ().type ()] += 1;
        i.finish_array ();
      }
      m_count [i->type ()] += n;
    }
  }

  void operator*= (size_t f)
  {
    for (std::map<db::Shape::object_type, size_t>::iterator c = m_count.begin (); c != m_count.end (); ++c) {
      c->second *= f;
    }
  }

  void operator+= (const ShapeStatistics &other)
  {
    for (std::map<db::Shape::object_type, size_t>::const_iterator c = other.m_count.begin (); c != other.m_count.end (); ++c) {
      m_count [c->first] += c->second;
    }
  }

  size_t count (db::Shape::object_type t) const
  {
    std::map<db::Shape::object_type, size_t>::const_iterator c = m_count.find (t);
    if (c != m_count.end ()) {
      return c->second;
    } else {
      return 0;
    }
  }

  size_t box_total () const
  {
    return count (db::Shape::Box) + count (db::Shape::BoxArrayMember) + count (db::Shape::ShortBox) + count (db::Shape::ShortBoxArrayMember);
  }

  size_t box_single () const
  {
    return count (db::Shape::Box) + count (db::Shape::ShortBox);
  }

  size_t box_array () const
  {
    return count (db::Shape::BoxArray) + count (db::Shape::ShortBoxArray);
  }

  size_t polygon_total () const
  {
    return count (db::Shape::Polygon) + count (db::Shape::PolygonRef) + count (db::Shape::PolygonPtrArrayMember)
         + count (db::Shape::SimplePolygon) + count (db::Shape::SimplePolygonRef) + count (db::Shape::SimplePolygonPtrArrayMember);
  }

  size_t polygon_single () const
  {
    return count (db::Shape::Polygon) + count (db::Shape::PolygonRef)
         + count (db::Shape::SimplePolygon) + count (db::Shape::SimplePolygonRef);
  }

  size_t polygon_array () const
  {
    return count (db::Shape::PolygonPtrArray) + count (db::Shape::SimplePolygonPtrArray);
  }

  size_t path_total () const
  {
    return count (db::Shape::Path) + count (db::Shape::PathRef) + count (db::Shape::PathPtrArrayMember);
  }

  size_t path_single () const
  {
    return count (db::Shape::Path) + count (db::Shape::PathRef);
  }

  size_t path_array () const
  {
    return count (db::Shape::PathPtrArray);
  }

  size_t text_total () const
  {
    return count (db::Shape::Text) + count (db::Shape::TextRef) + count (db::Shape::TextPtrArrayMember);
  }

  size_t text_single () const
  {
    return count (db::Shape::Text) + count (db::Shape::TextRef);
  }

  size_t text_array () const
  {
    return count (db::Shape::TextPtrArray);
  }

  size_t edge_total () const
  {
    return count (db::Shape::Edge);
  }

  size_t edge_pair_total () const
  {
    return count (db::Shape::EdgePair);
  }

  size_t user_total () const
  {
    return count (db::Shape::UserObject);
  }

  size_t all_total () const
  {
    return box_total () +
           polygon_total () +
           path_total () +
           text_total () +
           edge_total () +
           edge_pair_total () +
           user_total ();
  }

private:
  std::map<db::Shape::object_type, size_t> m_count;
};

// ------------------------------------------------------------

class StatisticsSource
  : public lay::BrowserSource
{
public:
  StatisticsSource (const lay::LayoutHandleRef &h)
    : m_h (h)
  {
    //  .. nothing yet ..
  }

  std::string get (const std::string &url)
  {
    auto p = m_page_cache.find (url);
    if (p != m_page_cache.end ()) {
      return p->second;
    } else {
      std::string t = get_impl (url);
      m_page_cache [url] = t;
      return t;
    }
  }

  void clear_cache ()
  {
    m_page_cache.clear ();
  }

private:
  const lay::LayoutHandleRef m_h;
  std::map<std::string, std::string> m_page_cache;

  std::string index_page (const tl::URI &uri) const;
  std::string per_layer_stat_page (const tl::URI &uri) const;
  std::string get_impl (const std::string &url);
};

static std::string s_per_layer_stat_path_ld = "per-layer-stat-ld";
static std::string s_per_layer_stat_path_name = "per-layer-stat-name";

std::string
StatisticsSource::per_layer_stat_page (const tl::URI &uri) const
{
  //  This is the default top level page
  //  TODO: handle other input as well

  const db::Layout &layout = m_h->layout ();

  std::ostringstream os;
  os.imbue (std::locale ("C"));

  std::vector <unsigned int> layers;
  for (unsigned int i = 0; i < layout.layers (); ++i) {
    if (layout.is_valid_layer (i)) {
      layers.push_back (i);
    }
  }

  if (tl::basename (uri.path ()) == s_per_layer_stat_path_ld) {
    std::sort (layers.begin (), layers.end (), CompareLDName (layout));
  } else {
    std::sort (layers.begin (), layers.end (), CompareNameLD (layout));
  }

  os << "<html>" << std::endl
     <<   "<body>" << std::endl
     <<     "<h2>" << tl::to_string (QObject::tr ("Detailed Layer Statistics for '")) << m_h->name () << "'</h2>" << std::endl

     <<     "<p>" << std::endl
     <<     "<table cellspacing=\"5\" cellpadding=\"5\">" << std::endl

     <<       "<tr>" << std::endl
     <<         "<th bgcolor=\"#f0f0f0\">" << tl::to_string (QObject::tr ("Layer")) << "</th>" << std::endl
     <<         "<th bgcolor=\"#f0f0f0\">" << tl::to_string (QObject::tr ("All")) << "</th>" << std::endl
     <<         "<th colspan=\"3\" bgcolor=\"#f0f0f0\">" << tl::to_string (QObject::tr ("Boxes")) << "</th>" << std::endl
     <<         "<th colspan=\"3\" bgcolor=\"#f0f0f0\">" << tl::to_string (QObject::tr ("Polygons")) << "</th>" << std::endl
     <<         "<th colspan=\"3\" bgcolor=\"#f0f0f0\">" << tl::to_string (QObject::tr ("Paths")) << "</th>" << std::endl
     <<         "<th colspan=\"3\" bgcolor=\"#f0f0f0\">" << tl::to_string (QObject::tr ("Texts")) << "</th>" << std::endl
     <<         "<th bgcolor=\"#f0f0f0\">" << tl::to_string (QObject::tr ("Edges")) << "</th>" << std::endl
     <<         "<th bgcolor=\"#f0f0f0\">" << tl::to_string (QObject::tr ("Edge Pairs")) << "</th>" << std::endl
     <<         "<th bgcolor=\"#f0f0f0\">" << tl::to_string (QObject::tr ("User objects")) << "</th>" << std::endl
     <<       "</tr>" << std::endl

     <<       "<tr>" << std::endl
     <<         "<th></th>" << std::endl
     <<         "<th>" << tl::to_string (QObject::tr ("(total)")) << "</th>" << std::endl
     <<         "<th>" << tl::to_string (QObject::tr ("(total)")) << "</th><th>" << tl::to_string (QObject::tr ("(single)")) << "</th><th>" << tl::to_string (QObject::tr ("(arrays)")) << "</th>" << std::endl
     <<         "<th>" << tl::to_string (QObject::tr ("(total)")) << "</th><th>" << tl::to_string (QObject::tr ("(single)")) << "</th><th>" << tl::to_string (QObject::tr ("(arrays)")) << "</th>" << std::endl
     <<         "<th>" << tl::to_string (QObject::tr ("(total)")) << "</th><th>" << tl::to_string (QObject::tr ("(single)")) << "</th><th>" << tl::to_string (QObject::tr ("(arrays)")) << "</th>" << std::endl
     <<         "<th>" << tl::to_string (QObject::tr ("(total)")) << "</th><th>" << tl::to_string (QObject::tr ("(single)")) << "</th><th>" << tl::to_string (QObject::tr ("(arrays)")) << "</th>" << std::endl
     <<         "<th>" << tl::to_string (QObject::tr ("(total)")) << "</th>" << std::endl
     <<         "<th>" << tl::to_string (QObject::tr ("(total)")) << "</th>" << std::endl
     <<         "<th>" << tl::to_string (QObject::tr ("(total)")) << "</th>" << std::endl
     <<       "</tr>" << std::endl
    ;

  db::CellCounter cc (&layout);

  tl::RelativeProgress progress (tl::to_string (QObject::tr ("Collecting statistics")), layers.size () * layout.cells (), 100000);
  for (std::vector <unsigned int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {

    ShapeStatistics st_hier;
    ShapeStatistics st_flat;

    for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {

      ShapeStatistics st;
      st.compute (layout.cell (*c).shapes (*l));

      st_hier += st;
      st *= cc.weight (*c);
      st_flat += st;

      ++progress;

    }

    os <<       "<tr>" << std::endl
       <<         "<td>" << tl::escaped_to_html (layout.get_properties (*l).to_string (), true) << "</td>" << std::endl
       <<         "<td>" << st_hier.all_total () << "<br></br>" << st_flat.all_total () << "</td>" << std::endl
       // Boxes (total, single, array)
       <<         "<td>" << st_hier.box_total () << "<br></br>" << st_flat.box_total () << "</td>" << std::endl
       <<         "<td>" << st_hier.box_single () << "<br></br>" << st_flat.box_single () << "</td>" << std::endl
       <<         "<td>" << st_hier.box_array () << "<br></br>" << st_flat.box_array () << "</td>" << std::endl
       // Polygons (total, single, array)
       <<         "<td>" << st_hier.polygon_total () << "<br></br>" << st_flat.polygon_total () << "</td>" << std::endl
       <<         "<td>" << st_hier.polygon_single () << "<br></br>" << st_flat.polygon_single () << "</td>" << std::endl
       <<         "<td>" << st_hier.polygon_array () << "<br></br>" << st_flat.polygon_array () << "</td>" << std::endl
       // Paths (total, single, array)
       <<         "<td>" << st_hier.path_total () << "<br></br>" << st_flat.path_total () << "</td>" << std::endl
       <<         "<td>" << st_hier.path_single () << "<br></br>" << st_flat.path_single () << "</td>" << std::endl
       <<         "<td>" << st_hier.path_array () << "<br></br>" << st_flat.path_array () << "</td>" << std::endl
       // Texts (total, single, array)
       <<         "<td>" << st_hier.text_total () << "<br></br>" << st_flat.text_total () << "</td>" << std::endl
       <<         "<td>" << st_hier.text_single () << "<br></br>" << st_flat.text_single () << "</td>" << std::endl
       <<         "<td>" << st_hier.text_array () << "<br></br>" << st_flat.text_array () << "</td>" << std::endl
       // Edges (total)
       <<         "<td>" << st_hier.edge_total () << "<br></br>" << st_flat.edge_total () << "</td>" << std::endl
       // EdgePairs (total)
       <<         "<td>" << st_hier.edge_pair_total () << "<br></br>" << st_flat.edge_pair_total () << "</td>" << std::endl
       // User objects (total)
       <<         "<td>" << st_hier.user_total () << "<br></br>" << st_flat.user_total () << "</td>" << std::endl
       // ...
       <<         "<td>" << tl::to_string (QObject::tr ("(hier)")) << "<br></br>" << tl::to_string (QObject::tr ("(flat)")) << "</td>" << std::endl
       <<       "</tr>" << std::endl
      ;

  }

  os <<     "</table>" << std::endl
     <<     "</p>" << std::endl
     <<     tl::to_string (QObject::tr ("<h4>Note</h4>"
                                        "<p>"
                                        "\"(hier)\" is the object count where each cell counts once. "
                                        "\"(flat)\" is the \"as if flat\" count where the cells count as many times as they are seen from the top cells."
                                        "</p>"
                                        "<p>"
                                        "\"(total)\" is the effective number of shapes. \"(single)\" are the single shapes. "
                                        "\"(arrays)\" is the number of shape arrays where each array counts as one, but contributes many individual shapes to \"(total)\"."
                                        "</p>"
                                       ))
     <<   "</body>" << std::endl
     << "</html>";

  return os.str ();
}

std::string
StatisticsSource::index_page (const tl::URI & /*uri*/) const
{
  //  maybe later ...
  bool with_shape_statistics = false;

  //  This is the default top level page
  //  TODO: handle other input as well

  const db::Layout &layout = m_h->layout ();

  std::ostringstream os;
  os.imbue (std::locale ("C"));

  size_t num_cells = layout.cells ();

  size_t num_layers = 0;
  for (unsigned int i = 0; i < layout.layers (); ++i) {
    if (layout.is_valid_layer (i)) {
      ++num_layers;
    }
  }

  db::CellCounter cc (&layout);

  os << "<html>" << std::endl
     <<   "<body>" << std::endl
     <<     "<h2>" << tl::to_string (QObject::tr ("Common Statistics For '")) << tl::escaped_to_html (m_h->name (), true) << "'</h2>" << std::endl
     <<     "<p>" << std::endl
     <<     "<table>" << std::endl
     <<       "<tr>"
     <<         "<td>" << tl::to_string (QObject::tr ("Path")) << ":&nbsp;</td><td>" << tl::escaped_to_html (m_h->filename (), true) << "</td>"
     <<       "</tr>" << std::endl;
  if (! m_h->save_options ().format ().empty ()) {
    os <<       "<tr>"
       <<         "<td>" << tl::to_string (QObject::tr ("Format")) << ":&nbsp;</td><td>" << tl::escaped_to_html (m_h->save_options ().format (), true) << "</td>"
       <<       "</tr>" << std::endl;
  }
  os <<       "<tr>"
     <<         "<td>" << tl::to_string (QObject::tr ("Technology")) << ":&nbsp;</td><td>" << tl::escaped_to_html (m_h->technology ()->description (), true) << tl::escaped_to_html (format_tech_name (m_h->tech_name ()), true) << "</td>"
     <<       "</tr>" << std::endl
     <<       "<tr>"
     <<         "<td>" << tl::to_string (QObject::tr ("Database unit")) << ":&nbsp;</td><td>" << tl::sprintf ("%.12g ", layout.dbu ()) << tl::to_string (QObject::tr ("micron")) << "</td>"
     <<       "</tr>" << std::endl
     <<       "<tr>"
     <<         "<td>" << tl::to_string (QObject::tr ("Number of cells")) << ":&nbsp;</td><td>" << num_cells << "</td>"
     <<       "</tr>" << std::endl
     <<       "<tr>"
     <<         "<td>" << tl::to_string (QObject::tr ("Number of layers")) << ":&nbsp;</td><td>" << num_layers << "</td>"
     <<       "</tr>" << std::endl;
  for (db::Layout::meta_info_iterator meta = layout.begin_meta (); meta != layout.end_meta (); ++meta) {
    std::string d = meta->second.description;
    if (!d.empty ()) {
      d = layout.meta_info_name (meta->first);
    }
    os <<     "<tr><td>" << tl::escaped_to_html (d, true) << "</td><td>" << tl::escaped_to_html (meta->second.value.to_string (), true) << "</td></tr>" << std::endl;
  }
  os <<     "</table>" << std::endl
     <<     "<h2>" << tl::to_string (QObject::tr ("Top Cells")) << "</h2>" << std::endl
     <<     "<table>" << std::endl;
  for (db::Layout::top_down_const_iterator tc = layout.begin_top_down (); tc != layout.end_top_cells (); ++tc) {
    os <<     "<tr><td>" << tl::escaped_to_html (layout.cell_name (*tc), true) << "</td></tr>" << std::endl;
  }
  os <<     "</table>" << std::endl;
  os <<     "</p>" << std::endl;

  std::vector <unsigned int> layers_with_oasis_names;

  std::vector <unsigned int> layers_sorted_by_ld;
  layers_sorted_by_ld.reserve (num_layers);
  for (unsigned int i = 0; i < layout.layers (); ++i) {
    if (layout.is_valid_layer (i)) {
      layers_sorted_by_ld.push_back (i);
      const db::LayerProperties &lp = layout.get_properties (i);
      if (! lp.name.empty ()) {
        layers_with_oasis_names.push_back (i);
      }
    }
  }

  std::sort (layers_sorted_by_ld.begin (), layers_sorted_by_ld.end (), CompareLDName (layout));
  std::sort (layers_with_oasis_names.begin (), layers_with_oasis_names.end (), CompareNameLD (layout));

  if (! layers_sorted_by_ld.empty ()) {

    os <<     "<h2>" << tl::to_string (QObject::tr ("Layers (sorted by layer and datatype)")) << "</h2>" << std::endl
       <<     "<p><a href=\"" << tl::escaped_to_html (tl::to_string (s_per_layer_stat_path_ld), true) << "\">Detailed layer statistics</a></p>" << std::endl
       <<     "<p>" << std::endl
       <<     "<table>" << std::endl
       <<     "<tr><td><b>" << tl::to_string (QObject::tr ("Layer/Datatype")) << "</b>&nbsp;&nbsp;</td>";
    if (! layers_with_oasis_names.empty ()) {
      os << "<td><b>" << tl::to_string (QObject::tr ("Layer name")) << "</b></td>";
    }
    if (with_shape_statistics) {
      os << "<td><b>" << tl::to_string (QObject::tr ("Shape count (hier)")) << "</b></td>";
      os << "<td><b>" << tl::to_string (QObject::tr ("Shape count (flat)")) << "</b></td>";
    }
    os << "</tr>" << std::endl;

    tl::RelativeProgress progress (tl::to_string (QObject::tr ("Collecting statistics")), layers_sorted_by_ld.size () * layout.cells (), 100000);
    for (std::vector <unsigned int>::const_iterator i = layers_sorted_by_ld.begin (); i != layers_sorted_by_ld.end (); ++i) {

      if (! layout.is_valid_layer (*i)) {
        continue;
      }

      ShapeStatistics st_hier;
      ShapeStatistics st_flat;

      if (with_shape_statistics) {
        for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {

          ShapeStatistics st;
          st.compute (layout.cell (*c).shapes (*i));

          st_hier += st;
          st *= cc.weight (*c);
          st_flat += st;

          ++progress;

        }
      }

      const db::LayerProperties &lp = layout.get_properties (*i);
      os << "<tr>"
         <<   "<td>" << tl::sprintf ("%d/%d", lp.layer, lp.datatype) << "</td>";
      if (! layers_with_oasis_names.empty ()) {
        os <<   "<td>" << tl::escaped_to_html (lp.name, true) << "</td>";
      }
      if (with_shape_statistics) {
        os <<   "<td>" << st_hier.all_total () << "</td>";
        os <<   "<td>" << st_flat.all_total () << "</td>";
      }
      os << "</tr>" << std::endl;

    }

    os <<     "</table>" << std::endl;
    os <<     "</p>" << std::endl;

  }

  if (! layers_with_oasis_names.empty ()) {

    os <<     "<h2>" << tl::to_string (QObject::tr ("Layers (sorted by layer names)")) << "</h2>" << std::endl
       <<     "<p><a href=\"" << tl::escaped_to_html (tl::to_string (s_per_layer_stat_path_name), true) << "\">Detailed layer statistics</a></p>" << std::endl
       <<     "<p>" << std::endl
       <<     "<table>" << std::endl
       <<     "<tr><td><b>" << tl::to_string (QObject::tr ("Layer name")) << "</b>&nbsp;&nbsp;</td><td><b>" << tl::to_string (QObject::tr ("Layer/Datatype")) << "</b></td></tr>" << std::endl;

    for (std::vector <unsigned int>::const_iterator i = layers_with_oasis_names.begin (); i != layers_with_oasis_names.end (); ++i) {
      if (layout.is_valid_layer (*i)) {
        const db::LayerProperties &lp = layout.get_properties (*i);
        if (! lp.name.empty ()) {
          os << "<tr>"
             <<   "<td>" << tl::escaped_to_html (lp.name, true) << "</td>"
             <<   "<td>" << tl::sprintf ("%d/%d", lp.layer, lp.datatype) << "</td>"
             << "</tr>" << std::endl;
        }
      }
    }

    os <<     "</table>" << std::endl;
    os <<     "</p>" << std::endl;

  }

  os <<   "</body>" << std::endl
     << "</html>" << std::endl;
     ;

  return os.str ();
}

std::string
StatisticsSource::get_impl (const std::string &url)
{
  tl::URI uri (url);
  std::string page = tl::basename (uri.path ());

  if (tl::extension (page) == "stxml") {

    StatisticsTemplateProcessor tp (uri, &m_h->layout ());
    tp.process ();
    std::string r = tp.get ().constData ();
    return r;

  } else if (page == s_per_layer_stat_path_ld || page == s_per_layer_stat_path_name) {

    return per_layer_stat_page (uri);

  } else {

    return index_page (uri);

  }
}

// ------------------------------------------------------------

LayoutStatisticsForm::LayoutStatisticsForm (QWidget *parent, lay::LayoutViewBase *view, const char *name)
  : QDialog (parent), Ui::LayoutStatisticsForm (), mp_source (0)
{
  setObjectName (QString::fromUtf8 (name));

  Ui::LayoutStatisticsForm::setupUi (this);

  //  collect the distinct layout handles 
  std::set <lay::LayoutHandle *> handles;
  for (unsigned int n = 0; n < view->cellviews (); ++n) {
    handles.insert (view->cellview (n).operator-> ());
  }
  
  m_handles.reserve (handles.size ());
  for (unsigned int n = 0; n < view->cellviews (); ++n) {
    lay::LayoutHandle *h = view->cellview (n).operator-> ();
    if (handles.find (h) != handles.end ()) {
      m_handles.push_back (h);
      handles.erase (h);
      layout_cbx->addItem (tl::to_qstring (h->name ()));
    }
  }

  layout_cbx->setCurrentIndex (view->active_cellview_index ());

  connect (layout_cbx, SIGNAL (activated (int)), this, SLOT (layout_selected (int)));

  layout_selected (layout_cbx->currentIndex ());
}

LayoutStatisticsForm::~LayoutStatisticsForm ()
{
  browser->set_source (0);
  if (mp_source != 0) {
    delete mp_source;
    mp_source = 0;
  }
}

void
LayoutStatisticsForm::layout_selected (int index)
{
  if (index >= int (m_handles.size ()) || index < 0) {
    return;
  }

  browser->set_source (0);
  if (mp_source != 0) {
    delete mp_source;
  }

  mp_source = new StatisticsSource (m_handles [index]);
  browser->set_source (mp_source);
  browser->set_home ("int:index");
  browser->home ();
}

}

#endif
