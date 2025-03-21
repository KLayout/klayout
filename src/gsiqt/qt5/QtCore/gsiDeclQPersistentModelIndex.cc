
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

/**
*  @file gsiDeclQPersistentModelIndex.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QPersistentModelIndex>
#include <QAbstractItemModel>
#include <QModelIndex>
#include "gsiQt.h"
#include "gsiQtCoreCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QPersistentModelIndex
#if QT_VERSION < 0x60000
  static const QModelIndex &castToQModelIndex(const QPersistentModelIndex *m)
  {
    return m->operator const QModelIndex &();
  }
#else
  static QModelIndex castToQModelIndex(const QPersistentModelIndex *m)
  {
    return m->operator QModelIndex();
  }
#endif

//  Constructor QPersistentModelIndex::QPersistentModelIndex()


static void _init_ctor_QPersistentModelIndex_0 (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return_new<QPersistentModelIndex> ();
}

static void _call_ctor_QPersistentModelIndex_0 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QPersistentModelIndex *> (new QPersistentModelIndex ());
}


//  Constructor QPersistentModelIndex::QPersistentModelIndex(const QModelIndex &index)


static void _init_ctor_QPersistentModelIndex_2395 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("index");
  decl->add_arg<const QModelIndex & > (argspec_0);
  decl->set_return_new<QPersistentModelIndex> ();
}

static void _call_ctor_QPersistentModelIndex_2395 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QModelIndex &arg1 = gsi::arg_reader<const QModelIndex & >() (args, heap);
  ret.write<QPersistentModelIndex *> (new QPersistentModelIndex (arg1));
}


//  Constructor QPersistentModelIndex::QPersistentModelIndex(const QPersistentModelIndex &other)


static void _init_ctor_QPersistentModelIndex_3468 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QPersistentModelIndex & > (argspec_0);
  decl->set_return_new<QPersistentModelIndex> ();
}

static void _call_ctor_QPersistentModelIndex_3468 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QPersistentModelIndex &arg1 = gsi::arg_reader<const QPersistentModelIndex & >() (args, heap);
  ret.write<QPersistentModelIndex *> (new QPersistentModelIndex (arg1));
}


// QModelIndex QPersistentModelIndex::child(int row, int column)


static void _init_f_child_c1426 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("row");
  decl->add_arg<int > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("column");
  decl->add_arg<int > (argspec_1);
  decl->set_return<QModelIndex > ();
}

static void _call_f_child_c1426 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  int arg2 = gsi::arg_reader<int >() (args, heap);
  ret.write<QModelIndex > ((QModelIndex)((QPersistentModelIndex *)cls)->child (arg1, arg2));
}


// int QPersistentModelIndex::column()


static void _init_f_column_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_column_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QPersistentModelIndex *)cls)->column ());
}


// QVariant QPersistentModelIndex::data(int role)


static void _init_f_data_c767 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("role", true, "Qt::DisplayRole");
  decl->add_arg<int > (argspec_0);
  decl->set_return<QVariant > ();
}

static void _call_f_data_c767 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = args ? gsi::arg_reader<int >() (args, heap) : gsi::arg_maker<int >() (Qt::DisplayRole, heap);
  ret.write<QVariant > ((QVariant)((QPersistentModelIndex *)cls)->data (arg1));
}


// QFlags<Qt::ItemFlag> QPersistentModelIndex::flags()


static void _init_f_flags_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QFlags<Qt::ItemFlag> > ();
}

static void _call_f_flags_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QFlags<Qt::ItemFlag> > ((QFlags<Qt::ItemFlag>)((QPersistentModelIndex *)cls)->flags ());
}


// quintptr QPersistentModelIndex::internalId()


static void _init_f_internalId_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<quintptr > ();
}

static void _call_f_internalId_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<quintptr > ((quintptr)((QPersistentModelIndex *)cls)->internalId ());
}


// void *QPersistentModelIndex::internalPointer()


static void _init_f_internalPointer_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<void * > ();
}

static void _call_f_internalPointer_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<void * > ((void *)((QPersistentModelIndex *)cls)->internalPointer ());
}


// bool QPersistentModelIndex::isValid()


static void _init_f_isValid_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_f_isValid_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QPersistentModelIndex *)cls)->isValid ());
}


// const QAbstractItemModel *QPersistentModelIndex::model()


static void _init_f_model_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<const QAbstractItemModel * > ();
}

static void _call_f_model_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<const QAbstractItemModel * > ((const QAbstractItemModel *)((QPersistentModelIndex *)cls)->model ());
}


// bool QPersistentModelIndex::operator!=(const QPersistentModelIndex &other)


static void _init_f_operator_excl__eq__c3468 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QPersistentModelIndex & > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_operator_excl__eq__c3468 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QPersistentModelIndex &arg1 = gsi::arg_reader<const QPersistentModelIndex & >() (args, heap);
  ret.write<bool > ((bool)((QPersistentModelIndex *)cls)->operator!= (arg1));
}


// bool QPersistentModelIndex::operator!=(const QModelIndex &other)


static void _init_f_operator_excl__eq__c2395 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QModelIndex & > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_operator_excl__eq__c2395 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QModelIndex &arg1 = gsi::arg_reader<const QModelIndex & >() (args, heap);
  ret.write<bool > ((bool)((QPersistentModelIndex *)cls)->operator!= (arg1));
}


// bool QPersistentModelIndex::operator<(const QPersistentModelIndex &other)


static void _init_f_operator_lt__c3468 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QPersistentModelIndex & > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_operator_lt__c3468 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QPersistentModelIndex &arg1 = gsi::arg_reader<const QPersistentModelIndex & >() (args, heap);
  ret.write<bool > ((bool)((QPersistentModelIndex *)cls)->operator< (arg1));
}


// QPersistentModelIndex &QPersistentModelIndex::operator=(const QPersistentModelIndex &other)


static void _init_f_operator_eq__3468 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QPersistentModelIndex & > (argspec_0);
  decl->set_return<QPersistentModelIndex & > ();
}

static void _call_f_operator_eq__3468 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QPersistentModelIndex &arg1 = gsi::arg_reader<const QPersistentModelIndex & >() (args, heap);
  ret.write<QPersistentModelIndex & > ((QPersistentModelIndex &)((QPersistentModelIndex *)cls)->operator= (arg1));
}


// QPersistentModelIndex &QPersistentModelIndex::operator=(const QModelIndex &other)


static void _init_f_operator_eq__2395 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QModelIndex & > (argspec_0);
  decl->set_return<QPersistentModelIndex & > ();
}

static void _call_f_operator_eq__2395 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QModelIndex &arg1 = gsi::arg_reader<const QModelIndex & >() (args, heap);
  ret.write<QPersistentModelIndex & > ((QPersistentModelIndex &)((QPersistentModelIndex *)cls)->operator= (arg1));
}


// bool QPersistentModelIndex::operator==(const QPersistentModelIndex &other)


static void _init_f_operator_eq__eq__c3468 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QPersistentModelIndex & > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_operator_eq__eq__c3468 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QPersistentModelIndex &arg1 = gsi::arg_reader<const QPersistentModelIndex & >() (args, heap);
  ret.write<bool > ((bool)((QPersistentModelIndex *)cls)->operator== (arg1));
}


// bool QPersistentModelIndex::operator==(const QModelIndex &other)


static void _init_f_operator_eq__eq__c2395 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QModelIndex & > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_operator_eq__eq__c2395 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QModelIndex &arg1 = gsi::arg_reader<const QModelIndex & >() (args, heap);
  ret.write<bool > ((bool)((QPersistentModelIndex *)cls)->operator== (arg1));
}


// QModelIndex QPersistentModelIndex::parent()


static void _init_f_parent_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QModelIndex > ();
}

static void _call_f_parent_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QModelIndex > ((QModelIndex)((QPersistentModelIndex *)cls)->parent ());
}


// int QPersistentModelIndex::row()


static void _init_f_row_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_row_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QPersistentModelIndex *)cls)->row ());
}


// QModelIndex QPersistentModelIndex::sibling(int row, int column)


static void _init_f_sibling_c1426 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("row");
  decl->add_arg<int > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("column");
  decl->add_arg<int > (argspec_1);
  decl->set_return<QModelIndex > ();
}

static void _call_f_sibling_c1426 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  int arg2 = gsi::arg_reader<int >() (args, heap);
  ret.write<QModelIndex > ((QModelIndex)((QPersistentModelIndex *)cls)->sibling (arg1, arg2));
}


// void QPersistentModelIndex::swap(QPersistentModelIndex &other)


static void _init_f_swap_2773 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<QPersistentModelIndex & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_swap_2773 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QPersistentModelIndex &arg1 = gsi::arg_reader<QPersistentModelIndex & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QPersistentModelIndex *)cls)->swap (arg1);
}



namespace gsi
{

static gsi::Methods methods_QPersistentModelIndex () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QPersistentModelIndex::QPersistentModelIndex()\nThis method creates an object of class QPersistentModelIndex.", &_init_ctor_QPersistentModelIndex_0, &_call_ctor_QPersistentModelIndex_0);
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QPersistentModelIndex::QPersistentModelIndex(const QModelIndex &index)\nThis method creates an object of class QPersistentModelIndex.", &_init_ctor_QPersistentModelIndex_2395, &_call_ctor_QPersistentModelIndex_2395);
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QPersistentModelIndex::QPersistentModelIndex(const QPersistentModelIndex &other)\nThis method creates an object of class QPersistentModelIndex.", &_init_ctor_QPersistentModelIndex_3468, &_call_ctor_QPersistentModelIndex_3468);
  methods += new qt_gsi::GenericMethod ("child", "@brief Method QModelIndex QPersistentModelIndex::child(int row, int column)\n", true, &_init_f_child_c1426, &_call_f_child_c1426);
  methods += new qt_gsi::GenericMethod ("column", "@brief Method int QPersistentModelIndex::column()\n", true, &_init_f_column_c0, &_call_f_column_c0);
  methods += new qt_gsi::GenericMethod ("data", "@brief Method QVariant QPersistentModelIndex::data(int role)\n", true, &_init_f_data_c767, &_call_f_data_c767);
  methods += new qt_gsi::GenericMethod ("flags", "@brief Method QFlags<Qt::ItemFlag> QPersistentModelIndex::flags()\n", true, &_init_f_flags_c0, &_call_f_flags_c0);
  methods += new qt_gsi::GenericMethod ("internalId", "@brief Method quintptr QPersistentModelIndex::internalId()\n", true, &_init_f_internalId_c0, &_call_f_internalId_c0);
  methods += new qt_gsi::GenericMethod ("internalPointer", "@brief Method void *QPersistentModelIndex::internalPointer()\n", true, &_init_f_internalPointer_c0, &_call_f_internalPointer_c0);
  methods += new qt_gsi::GenericMethod ("isValid?", "@brief Method bool QPersistentModelIndex::isValid()\n", true, &_init_f_isValid_c0, &_call_f_isValid_c0);
  methods += new qt_gsi::GenericMethod ("model", "@brief Method const QAbstractItemModel *QPersistentModelIndex::model()\n", true, &_init_f_model_c0, &_call_f_model_c0);
  methods += new qt_gsi::GenericMethod ("!=", "@brief Method bool QPersistentModelIndex::operator!=(const QPersistentModelIndex &other)\n", true, &_init_f_operator_excl__eq__c3468, &_call_f_operator_excl__eq__c3468);
  methods += new qt_gsi::GenericMethod ("!=", "@brief Method bool QPersistentModelIndex::operator!=(const QModelIndex &other)\n", true, &_init_f_operator_excl__eq__c2395, &_call_f_operator_excl__eq__c2395);
  methods += new qt_gsi::GenericMethod ("<", "@brief Method bool QPersistentModelIndex::operator<(const QPersistentModelIndex &other)\n", true, &_init_f_operator_lt__c3468, &_call_f_operator_lt__c3468);
  methods += new qt_gsi::GenericMethod ("assign", "@brief Method QPersistentModelIndex &QPersistentModelIndex::operator=(const QPersistentModelIndex &other)\n", false, &_init_f_operator_eq__3468, &_call_f_operator_eq__3468);
  methods += new qt_gsi::GenericMethod ("assign", "@brief Method QPersistentModelIndex &QPersistentModelIndex::operator=(const QModelIndex &other)\n", false, &_init_f_operator_eq__2395, &_call_f_operator_eq__2395);
  methods += new qt_gsi::GenericMethod ("==", "@brief Method bool QPersistentModelIndex::operator==(const QPersistentModelIndex &other)\n", true, &_init_f_operator_eq__eq__c3468, &_call_f_operator_eq__eq__c3468);
  methods += new qt_gsi::GenericMethod ("==", "@brief Method bool QPersistentModelIndex::operator==(const QModelIndex &other)\n", true, &_init_f_operator_eq__eq__c2395, &_call_f_operator_eq__eq__c2395);
  methods += new qt_gsi::GenericMethod ("parent", "@brief Method QModelIndex QPersistentModelIndex::parent()\n", true, &_init_f_parent_c0, &_call_f_parent_c0);
  methods += new qt_gsi::GenericMethod ("row", "@brief Method int QPersistentModelIndex::row()\n", true, &_init_f_row_c0, &_call_f_row_c0);
  methods += new qt_gsi::GenericMethod ("sibling", "@brief Method QModelIndex QPersistentModelIndex::sibling(int row, int column)\n", true, &_init_f_sibling_c1426, &_call_f_sibling_c1426);
  methods += new qt_gsi::GenericMethod ("swap", "@brief Method void QPersistentModelIndex::swap(QPersistentModelIndex &other)\n", false, &_init_f_swap_2773, &_call_f_swap_2773);
  return methods;
}

gsi::Class<QPersistentModelIndex> decl_QPersistentModelIndex ("QtCore", "QPersistentModelIndex",
  gsi::method_ext("castToQModelIndex", &castToQModelIndex, "@brief Binding for \"operator const QModelIndex &\".")
+
  methods_QPersistentModelIndex (),
  "@qt\n@brief Binding of QPersistentModelIndex");


GSI_QTCORE_PUBLIC gsi::Class<QPersistentModelIndex> &qtdecl_QPersistentModelIndex () { return decl_QPersistentModelIndex; }

}

