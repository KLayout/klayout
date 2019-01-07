
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


#ifndef HDR_dbBoxScanner
#define HDR_dbBoxScanner

#include "dbCommon.h"

#include "dbBoxConvert.h"
#include "tlProgress.h"

#include <list>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <memory>

namespace db
{

/**
 *  @brief A utility class for the box scanner implementation
 */
template <class BoxConvert, class Obj, class Prop, class SideOp>
struct bs_side_compare_func
  : std::binary_function<std::pair<const Obj *, Prop>, std::pair<const Obj *, Prop>, bool> 
{
  typedef typename BoxConvert::box_type box_type;

  bs_side_compare_func (const BoxConvert &bc)
    : m_bc (bc)
  {
    //  .. nothing yet ..
  }

  bool operator() (const std::pair<const Obj *, Prop> &a, const std::pair<const Obj *, Prop> &b) const
  {
    SideOp sideop;
    return sideop (m_bc (*a.first)) < sideop (m_bc (*b.first));
  }

private:
  BoxConvert m_bc;
};

/**
 *  @brief A utility class for the box scanner implementation
 */
template <class BoxConvert, class Obj, class Prop, class SideOp>
struct bs_side_compare_vs_const_func
  : std::unary_function<std::pair<const Obj *, Prop>, bool> 
{
  typedef typename BoxConvert::box_type box_type;
  typedef typename box_type::coord_type coord_type;

  bs_side_compare_vs_const_func (const BoxConvert &bc, coord_type c)
    : m_bc (bc), m_c (c)
  {
    //  .. nothing yet ..
  }

  bool operator() (const std::pair<const Obj *, Prop> &a) const
  {
    SideOp sideop;
    return sideop (m_bc (*a.first)) < m_c;
  }

private:
  BoxConvert m_bc;
  coord_type m_c;
};

/**
 *  @brief A predicate the checks two boxes for overlap while applying an enlargement to right and top
 */
template <class Box>
bool bs_boxes_overlap (const Box &b1, const Box &b2, typename Box::coord_type enl)
{
  if (b1.empty () || b2.empty ()) {
    return false;
  } else {
    return (b1.p1 ().x () < b2.p2 ().x () + enl && b2.p1 ().x () < b1.p2 ().x () + enl) &&
           (b1.p1 ().y () < b2.p2 ().y () + enl && b2.p1 ().y () < b1.p2 ().y () + enl);
  }
}

/**
 *  @brief A template for the box scanner output receiver
 *
 *  This template specifies the methods or provides a default implementation for them 
 *  for use as the output receiver of the box scanner.
 */
template <class Obj, class Prop>
struct box_scanner_receiver
{
  /**
   *  @brief Indicates that the given object is no longer used
   *
   *  The finish method is called when an object is no longer in the queue and can be
   *  discarded.
   */
  void finish (const Obj * /*obj*/, const Prop & /*prop*/) { }

  /*
   *  @brief Callback for an interaction of o1 with o2.
   *
   *  This method is called when the object o1 interacts with o2 within the current 
   *  definition.
   */
  void add (const Obj * /*o1*/, const Prop & /*p1*/, const Obj * /*o2*/, const Prop & /*p2*/) { }
};

/**
 *  @brief A box scanner framework
 *
 *  A box scanner receives a series of objects of type Obj which can be converted to boxes and associated
 *  properties (of type Prop). It will store pointers to these objects, so they lifetime of these objects
 *  must exceed that of the box scanner.
 *
 *  The basic function of the box scanner is to derive interactions. This is done in the process 
 *  method. After the box scanner has been filled with object pointers, the process method can be called.
 *  The process method will derive all interactions and report these to the Rec argument of the 
 *  process method. 
 *
 *  "Rec" is the interaction receiver. It will receive events when an interaction is encountered.
 *  See the box_scanner_receiver template for a description of the methods this object must provide.
 *
 *  See the process method for the description of the options of the interaction test.
 */
template <class Obj, class Prop>
class box_scanner 
{
public:
  typedef Obj object_type;
  typedef Prop property_type;
  typedef std::vector<std::pair<const Obj *, Prop> > container_type;
  typedef typename container_type::iterator iterator_type;

  /**
   *  @brief Default ctor
   */
  box_scanner (bool report_progress = false, const std::string &progress_desc = std::string ())
    : m_fill_factor (2), m_scanner_thr (100), 
      m_report_progress (report_progress), m_progress_desc (progress_desc)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Sets the scanner threshold
   *
   *  This value determines for how many elements the implementation switches to the scanner 
   *  implementation instead of the plain element-by-element interaction test.
   *  The default value is 100.
   */
  void set_scanner_threshold (size_t n)
  {
    m_scanner_thr = n;
  }

  /**
   *  @brief Gets the scanner threshold
   */
  size_t scanner_threshold () const
  {
    return m_scanner_thr;
  }

  /**
   *  @brief Sets the fill factor
   *
   *  The fill factor determines how many new entries will be collected for a band. 
   *  A fill factor of 2 means that the number of elements in the band will be 
   *  doubled after elements outside of the band have been removed.
   *  The default fill factor is 2.
   */
  void set_fill_factor (double ff)
  {
    m_fill_factor = ff;
  }

  /**
   *  @brief Gets the fill factor
   */
  double fill_factor () const
  {
    return m_fill_factor;
  }

  /**
   *  @brief Reserve for n elements
   */
  void reserve (size_t n)
  {
    m_pp.reserve (n);
  }

  /**
   *  @brief Clears the container
   */
  void clear ()
  {
    m_pp.clear ();
  }

  /**
   *  @brief Inserts a new object into the scanner
   *
   *  The object's pointer is stored, so the object must remain valid until the 
   *  scanner does not need it any longer. An additional property can be attached to 
   *  the object which will be stored along with the object.
   */
  void insert (const Obj *obj, const Prop &prop)
  {
    m_pp.push_back (std::make_pair (obj, prop));
  }

  /**
   *  @brief Get the interactions between the stored objects
   *
   *  Two objects interact if the boxes of the objects enlarged by the given value overlap.
   *  The enlargement is specified in units of width and height, i.e. half of the enlargement
   *  is applied to one side before the overlap check.
   *
   *  An enlargement of 1 means that boxes have to touch only in order to get an interaction.
   *
   *  The box scanner will report all interactions to the receiver object. See box_scanner_receiver
   *  for details about the methods that this object must provide. 
   *
   *  The box converter must be capable of converting the Obj object into a box. 
   *  It must provide a box_type typedef.
   */
  template <class Rec, class BoxConvert>
  void process (Rec &rec, typename BoxConvert::box_type::coord_type enl, const BoxConvert &bc = BoxConvert ())
  {
    typedef typename BoxConvert::box_type box_type;
    typedef typename box_type::coord_type coord_type;
    typedef bs_side_compare_func<BoxConvert, Obj, Prop, box_bottom<Box> > bottom_side_compare_func;
    typedef bs_side_compare_func<BoxConvert, Obj, Prop, box_left<Box> > left_side_compare_func;
    typedef bs_side_compare_vs_const_func<BoxConvert, Obj, Prop, box_top<Box> > below_func;
    typedef bs_side_compare_vs_const_func<BoxConvert, Obj, Prop, box_right<Box> > left_func;

    if (m_pp.size () <= m_scanner_thr) {

      //  below m_scanner_thr elements use the brute force approach which is faster in that case

      for (iterator_type i = m_pp.begin (); i != m_pp.end (); ++i) {
        for (iterator_type j = i + 1; j != m_pp.end (); ++j) {
          if (bs_boxes_overlap (bc (*i->first), bc (*j->first), enl)) {
            rec.add (i->first, i->second, j->first, j->second);
          }
        }
      }

      for (iterator_type i = m_pp.begin (); i != m_pp.end (); ++i) {
        rec.finish (i->first, i->second);
      }

    } else {

      std::set<std::pair<const Obj *, const Obj *> > seen;

      std::sort (m_pp.begin (), m_pp.end (), bottom_side_compare_func (bc));

      coord_type y = bc (*m_pp.front ().first).bottom ();

      iterator_type current = m_pp.begin ();
      iterator_type future = m_pp.begin ();

      std::auto_ptr<tl::RelativeProgress> progress (0);
      if (m_report_progress) {
        if (m_progress_desc.empty ()) {
          progress.reset (new tl::RelativeProgress (tl::to_string (tr ("Processing")), m_pp.size (), 1000));
        } else {
          progress.reset (new tl::RelativeProgress (m_progress_desc, m_pp.size (), 1000));
        }
      }

      while (future != m_pp.end ()) {

        iterator_type cc = current;
        current = std::partition (current, future, below_func (bc, y + 1 - enl));

        while (cc != current) {
          rec.finish (cc->first, cc->second);
          typename std::set<std::pair<const Obj *, const Obj *> >::iterator s;
          s = seen.lower_bound (std::make_pair (cc->first, (const Obj *)0));
          while (s != seen.end () && s->first == cc->first) {
            seen.erase (s++);
          }
          s = seen.lower_bound (std::make_pair ((const Obj *)0, cc->first));
          while (s != seen.end () && s->second == cc->first) {
            seen.erase (s++);
          }
          ++cc;
        }

        //  add at least the required items per band
        typename std::iterator_traits<iterator_type>::difference_type min_band_size = size_t ((future - current) * m_fill_factor);
        coord_type yy = y;
        do {
          yy = bc (*future->first).bottom ();
          do {
            ++future;
          } while (future != m_pp.end () && bc (*future->first).bottom () == yy);
        } while (future != m_pp.end () && future - current < min_band_size);

        std::sort (current, future, left_side_compare_func (bc));

        iterator_type c = current;
        iterator_type f = current;

        coord_type x = bc (*c->first).left ();

        while (f != future) {

          c = std::partition (c, f, left_func (bc, x + 1 - enl));

          iterator_type f0 = f;

          //  add at least the required items per band
          typename std::iterator_traits<iterator_type>::difference_type min_box_size = size_t ((f - c) * m_fill_factor);
          coord_type xx = x;
          do {
            xx = bc (*f->first).left ();
            do {
              ++f;
            } while (f != future && bc (*f->first).left () == xx);
          } while (f != future && f - c < min_box_size);

          for (iterator_type i = f0; i != f; ++i) {
            for (iterator_type j = c; j < i; ++j) {
              if (bs_boxes_overlap (bc (*i->first), bc (*j->first), enl)) {
                if (seen.insert (std::make_pair (i->first, j->first)).second) {
                  seen.insert (std::make_pair (j->first, i->first));
                  rec.add (i->first, i->second, j->first, j->second);
                }
              }
            }
          }

          x = xx;

          if (m_report_progress) {
            progress->set (f - m_pp.begin ());
          }

        }

        y = yy;

      }

      while (current != m_pp.end ()) {
        rec.finish (current->first, current->second);
        ++current;
      }

    }

  }

private:
  container_type m_pp;
  double m_fill_factor;
  size_t m_scanner_thr;
  bool m_report_progress;
  std::string m_progress_desc;
};

/**
 *  @brief A cluster template that stores properties
 *
 *  This template provides the definitions of the methods required for the
 *  cluster collector. It should be used as the base class because it provides 
 *  a storage for the object pointers inside the cluster.
 *
 *  One requirement for implementations of the cluster class is that they
 *  provide a copy constructor since the cluster objects are derived from
 *  a seed (initial template) cluster.
 *
 *  This cluster template also stores properties along with the 
 *  object pointers.
 */
template <class Obj, class Prop>
class cluster
{
public:
  typedef typename std::vector<std::pair<const Obj *, Prop> >::const_iterator iterator;

  /**
   *  @brief Adds an object to the cluster
   *
   *  The implementation of this method is supposed to add the given object to
   *  the cluster's data structure.
   */
  void add (const Obj *obj, const Prop &prop) 
  { 
    m_objects.push_back (std::make_pair (obj, prop));
  }

  /**
   *  @brief Joins the cluster with another one
   *
   *  The implementation of this method is supposed to import
   *  the data from the other cluster. After that the other cluster is 
   *  deleted.
   *
   *  The actual implementation shall use it's own class for the
   *  Argument.
   */
  void join (const cluster<Obj, Prop> &other) 
  {
    m_objects.insert (m_objects.end (), other.begin (), other.end ());
  }

  /**
   *  @brief Finishes this cluster
   *
   *  This method is called after the last member has been added to the cluster.
   *  After the cluster has been finished it is deleted.
   */
  void finish () { }

  /**
   *  @brief Begin iterator the objects in this cluster
   */
  iterator begin () const
  {
    return m_objects.begin ();
  }

  /**
   *  @brief End iterator the objects in this cluster
   */
  iterator end () const
  {
    return m_objects.end ();
  }

  /**
   *  @brief Fetch the object from the iterator
   */
  static inline const std::pair<const Obj *, Prop> &key_from_iter (iterator i)
  {
    return *i;
  }

  /**
   *  @brief Clears the results for this cluster
   */
  void clear ()
  {
    m_objects.clear ();
  }

private:
  std::vector<std::pair<const Obj *, Prop> > m_objects;
};

/**
 *  @brief A box scanner receiver that clusters the results
 *
 *  "Clustering" means that all objects interacting are grouped into one
 *  cluster.
 *
 *  For this, the cluster collector requires a cluster object which 
 *  provides the methods described by the cluster template.
 *
 *  It is important that the cluster object has a copy constructor
 *  since clusters are derived from it by copying a single instance
 *  provided in the cluster collector's constructor.
 *
 *  Whenever the cluster receiver gets noticed of an interaction, it will
 *  create new clusters or extend or join existing clusters. When a cluster
 *  is finished, it's finish method is called. That allows to take any
 *  final actions on the cluster.
 */
template <class Obj, class Prop, class Cluster>
class cluster_collector
  : public box_scanner_receiver<Obj, Prop>
{
public:
  typedef Obj object_type;
  typedef Prop property_type;
  typedef Cluster cluster_type;
  typedef std::list<std::pair<size_t, Cluster> > cl_type;
  typedef typename std::list<std::pair<size_t, Cluster> >::iterator cl_iterator_type;
  typedef std::pair<const Obj *, Prop> om_key_type;
  typedef std::map<om_key_type, cl_iterator_type> om_type;
  typedef typename om_type::iterator om_iterator_type;

  /**
   *  @brief The constructor 
   *
   *  It is important to provide a cluster seed (template) which is used to derive 
   *  new clusters by copying this one.
   */
  cluster_collector (const Cluster &cl_template, bool report_single = true)
    : m_cl_template (cl_template), m_report_single (report_single)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Implementation of the box scanner receiver class
   */
  void finish (const Obj *obj, const Prop &prop)
  {
    om_iterator_type omi = m_om.find (om_key_type (obj, prop));
    if (omi != m_om.end ()) {

      cl_iterator_type cli = omi->second;
      m_om.erase (omi);

      if (--cli->first == 0) {
        cli->second.finish ();
        m_cl.erase (cli);
      }

    } else if (m_report_single) {

      //  single-object entry: create a cluster and feed it a single-object signature
      Cluster cl (m_cl_template);
      cl.add (obj, prop);
      cl.finish ();

    }
  }

  void add_asymm (const Obj *o1, const Prop &p1, const Obj *o2, const Prop &p2)
  {
    om_iterator_type om1 = m_om.find (om_key_type (o1, p1));

    if (om1 == m_om.end ()) {

      //  first is new: create a new cluster
      m_cl.push_front (std::make_pair (size_t (1), m_cl_template));
      m_cl.front ().second.add (o1, p1);
      m_cl.front ().second.add (o2, p2);
      m_om.insert (std::make_pair (om_key_type (o1, p1), m_cl.begin ()));

    } else {

      //  second one is new: add to existing cluster
      om1->second->second.add (o2, p2);

    }
  }

  /**
   *  @brief Implementation of the box scanner receiver class
   */
  void add (const Obj *o1, const Prop &p1, const Obj *o2, const Prop &p2)
  {
    om_iterator_type om1 = m_om.find (om_key_type (o1, p1));
    om_iterator_type om2 = m_om.find (om_key_type (o2, p2));

    if (om1 == m_om.end () && om2 == m_om.end ()) {

      //  both are new: create a new cluster
      m_cl.push_front (std::make_pair (size_t (2), m_cl_template));
      m_cl.front ().second.add (o1, p1);
      m_cl.front ().second.add (o2, p2);
      m_om.insert (std::make_pair (om_key_type (o1, p1), m_cl.begin ()));
      m_om.insert (std::make_pair (om_key_type (o2, p2), m_cl.begin ()));

    } else if (om1 != m_om.end () && om2 == m_om.end ()) {

      //  second one is new: add to existing cluster
      ++om1->second->first;
      om1->second->second.add (o2, p2);
      m_om.insert (std::make_pair (om_key_type (o2, p2), om1->second));

    } else if (om1 == m_om.end () && om2 != m_om.end ()) {

      //  first one is new: add to existing cluster
      ++om2->second->first;
      om2->second->second.add (o1, p1);
      m_om.insert (std::make_pair (om_key_type (o1, p1), om2->second));

    } else if (om1->second != om2->second) {

      //  need to join clusters: use the first one
      om1->second->first += om2->second->first;
      om1->second->second.join (om2->second->second);

      //  remap the other entries
      cl_iterator_type c2 = om2->second; 
      for (typename Cluster::iterator o = c2->second.begin (); o != c2->second.end (); ++o) {
        om_iterator_type omi = m_om.find (Cluster::key_from_iter (o));
        if (omi != m_om.end ()) {
          omi->second = om1->second;
        }
      }

      //  and erase the other cluster
      m_cl.erase (c2);

    }
  }

private:
  Cluster m_cl_template;
  bool m_report_single;
  cl_type m_cl;
  om_type m_om;
};

}

#endif

