
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
*  @file gsiDeclQNetworkCookieJar.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QNetworkCookieJar>
#include <QChildEvent>
#include <QEvent>
#include <QNetworkCookie>
#include <QObject>
#include <QThread>
#include <QTimerEvent>
#include <QUrl>
#include "gsiQt.h"
#include "gsiQtNetworkCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QNetworkCookieJar

//  get static meta object

static void _init_smo (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return<const QMetaObject &> ();
}

static void _call_smo (const qt_gsi::GenericStaticMethod *, gsi::SerialArgs &, gsi::SerialArgs &ret) 
{
  ret.write<const QMetaObject &> (QNetworkCookieJar::staticMetaObject);
}


// QList<QNetworkCookie> QNetworkCookieJar::cookiesForUrl(const QUrl &url)


static void _init_f_cookiesForUrl_c1701 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("url");
  decl->add_arg<const QUrl & > (argspec_0);
  decl->set_return<QList<QNetworkCookie> > ();
}

static void _call_f_cookiesForUrl_c1701 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QUrl &arg1 = gsi::arg_reader<const QUrl & >() (args, heap);
  ret.write<QList<QNetworkCookie> > ((QList<QNetworkCookie>)((QNetworkCookieJar *)cls)->cookiesForUrl (arg1));
}


// bool QNetworkCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)


static void _init_f_setCookiesFromUrl_4950 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("cookieList");
  decl->add_arg<const QList<QNetworkCookie> & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("url");
  decl->add_arg<const QUrl & > (argspec_1);
  decl->set_return<bool > ();
}

static void _call_f_setCookiesFromUrl_4950 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QList<QNetworkCookie> &arg1 = gsi::arg_reader<const QList<QNetworkCookie> & >() (args, heap);
  const QUrl &arg2 = gsi::arg_reader<const QUrl & >() (args, heap);
  ret.write<bool > ((bool)((QNetworkCookieJar *)cls)->setCookiesFromUrl (arg1, arg2));
}


// static QString QNetworkCookieJar::tr(const char *s, const char *c)


static void _init_f_tr_3354 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("s");
  decl->add_arg<const char * > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("c", true, "0");
  decl->add_arg<const char * > (argspec_1);
  decl->set_return<QString > ();
}

static void _call_f_tr_3354 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const char *arg1 = gsi::arg_reader<const char * >() (args, heap);
  const char *arg2 = args ? gsi::arg_reader<const char * >() (args, heap) : gsi::arg_maker<const char * >() (0, heap);
  ret.write<QString > ((QString)QNetworkCookieJar::tr (arg1, arg2));
}


// static QString QNetworkCookieJar::tr(const char *s, const char *c, int n)


static void _init_f_tr_4013 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("s");
  decl->add_arg<const char * > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("c");
  decl->add_arg<const char * > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("n");
  decl->add_arg<int > (argspec_2);
  decl->set_return<QString > ();
}

static void _call_f_tr_4013 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const char *arg1 = gsi::arg_reader<const char * >() (args, heap);
  const char *arg2 = gsi::arg_reader<const char * >() (args, heap);
  int arg3 = gsi::arg_reader<int >() (args, heap);
  ret.write<QString > ((QString)QNetworkCookieJar::tr (arg1, arg2, arg3));
}


// static QString QNetworkCookieJar::trUtf8(const char *s, const char *c)


static void _init_f_trUtf8_3354 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("s");
  decl->add_arg<const char * > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("c", true, "0");
  decl->add_arg<const char * > (argspec_1);
  decl->set_return<QString > ();
}

static void _call_f_trUtf8_3354 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const char *arg1 = gsi::arg_reader<const char * >() (args, heap);
  const char *arg2 = args ? gsi::arg_reader<const char * >() (args, heap) : gsi::arg_maker<const char * >() (0, heap);
  ret.write<QString > ((QString)QNetworkCookieJar::trUtf8 (arg1, arg2));
}


// static QString QNetworkCookieJar::trUtf8(const char *s, const char *c, int n)


static void _init_f_trUtf8_4013 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("s");
  decl->add_arg<const char * > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("c");
  decl->add_arg<const char * > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("n");
  decl->add_arg<int > (argspec_2);
  decl->set_return<QString > ();
}

static void _call_f_trUtf8_4013 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const char *arg1 = gsi::arg_reader<const char * >() (args, heap);
  const char *arg2 = gsi::arg_reader<const char * >() (args, heap);
  int arg3 = gsi::arg_reader<int >() (args, heap);
  ret.write<QString > ((QString)QNetworkCookieJar::trUtf8 (arg1, arg2, arg3));
}


namespace gsi
{

static gsi::Methods methods_QNetworkCookieJar () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("staticMetaObject", "@brief Obtains the static MetaObject for this class.", &_init_smo, &_call_smo);
  methods += new qt_gsi::GenericMethod ("cookiesForUrl", "@brief Method QList<QNetworkCookie> QNetworkCookieJar::cookiesForUrl(const QUrl &url)\n", true, &_init_f_cookiesForUrl_c1701, &_call_f_cookiesForUrl_c1701);
  methods += new qt_gsi::GenericMethod ("setCookiesFromUrl", "@brief Method bool QNetworkCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)\n", false, &_init_f_setCookiesFromUrl_4950, &_call_f_setCookiesFromUrl_4950);
  methods += gsi::qt_signal<QObject * > ("destroyed(QObject *)", "destroyed", gsi::arg("arg1"), "@brief Signal declaration for QNetworkCookieJar::destroyed(QObject *)\nYou can bind a procedure to this signal.");
  methods += new qt_gsi::GenericStaticMethod ("tr", "@brief Static method QString QNetworkCookieJar::tr(const char *s, const char *c)\nThis method is static and can be called without an instance.", &_init_f_tr_3354, &_call_f_tr_3354);
  methods += new qt_gsi::GenericStaticMethod ("tr", "@brief Static method QString QNetworkCookieJar::tr(const char *s, const char *c, int n)\nThis method is static and can be called without an instance.", &_init_f_tr_4013, &_call_f_tr_4013);
  methods += new qt_gsi::GenericStaticMethod ("trUtf8", "@brief Static method QString QNetworkCookieJar::trUtf8(const char *s, const char *c)\nThis method is static and can be called without an instance.", &_init_f_trUtf8_3354, &_call_f_trUtf8_3354);
  methods += new qt_gsi::GenericStaticMethod ("trUtf8", "@brief Static method QString QNetworkCookieJar::trUtf8(const char *s, const char *c, int n)\nThis method is static and can be called without an instance.", &_init_f_trUtf8_4013, &_call_f_trUtf8_4013);
  return methods;
}

gsi::Class<QObject> &qtdecl_QObject ();

qt_gsi::QtNativeClass<QNetworkCookieJar> decl_QNetworkCookieJar (qtdecl_QObject (), "QtNetwork", "QNetworkCookieJar_Native",
  methods_QNetworkCookieJar (),
  "@hide\n@alias QNetworkCookieJar");

GSI_QTNETWORK_PUBLIC gsi::Class<QNetworkCookieJar> &qtdecl_QNetworkCookieJar () { return decl_QNetworkCookieJar; }

}


class QNetworkCookieJar_Adaptor : public QNetworkCookieJar, public qt_gsi::QtObjectBase
{
public:

  virtual ~QNetworkCookieJar_Adaptor();

  //  [adaptor ctor] QNetworkCookieJar::QNetworkCookieJar(QObject *parent)
  QNetworkCookieJar_Adaptor() : QNetworkCookieJar()
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor ctor] QNetworkCookieJar::QNetworkCookieJar(QObject *parent)
  QNetworkCookieJar_Adaptor(QObject *parent) : QNetworkCookieJar(parent)
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [expose] QList<QNetworkCookie> QNetworkCookieJar::allCookies()
  QList<QNetworkCookie> fp_QNetworkCookieJar_allCookies_c0 () const {
    return QNetworkCookieJar::allCookies();
  }

  //  [expose] int QNetworkCookieJar::receivers(const char *signal)
  int fp_QNetworkCookieJar_receivers_c1731 (const char *signal) const {
    return QNetworkCookieJar::receivers(signal);
  }

  //  [expose] QObject *QNetworkCookieJar::sender()
  QObject * fp_QNetworkCookieJar_sender_c0 () const {
    return QNetworkCookieJar::sender();
  }

  //  [expose] void QNetworkCookieJar::setAllCookies(const QList<QNetworkCookie> &cookieList)
  void fp_QNetworkCookieJar_setAllCookies_3357 (const QList<QNetworkCookie> &cookieList) {
    QNetworkCookieJar::setAllCookies(cookieList);
  }

  //  [adaptor impl] QList<QNetworkCookie> QNetworkCookieJar::cookiesForUrl(const QUrl &url)
  QList<QNetworkCookie> cbs_cookiesForUrl_c1701_0(const QUrl &url) const
  {
    return QNetworkCookieJar::cookiesForUrl(url);
  }

  virtual QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const
  {
    if (cb_cookiesForUrl_c1701_0.can_issue()) {
      return cb_cookiesForUrl_c1701_0.issue<QNetworkCookieJar_Adaptor, QList<QNetworkCookie>, const QUrl &>(&QNetworkCookieJar_Adaptor::cbs_cookiesForUrl_c1701_0, url);
    } else {
      return QNetworkCookieJar::cookiesForUrl(url);
    }
  }

  //  [adaptor impl] bool QNetworkCookieJar::event(QEvent *)
  bool cbs_event_1217_0(QEvent *arg1)
  {
    return QNetworkCookieJar::event(arg1);
  }

  virtual bool event(QEvent *arg1)
  {
    if (cb_event_1217_0.can_issue()) {
      return cb_event_1217_0.issue<QNetworkCookieJar_Adaptor, bool, QEvent *>(&QNetworkCookieJar_Adaptor::cbs_event_1217_0, arg1);
    } else {
      return QNetworkCookieJar::event(arg1);
    }
  }

  //  [adaptor impl] bool QNetworkCookieJar::eventFilter(QObject *, QEvent *)
  bool cbs_eventFilter_2411_0(QObject *arg1, QEvent *arg2)
  {
    return QNetworkCookieJar::eventFilter(arg1, arg2);
  }

  virtual bool eventFilter(QObject *arg1, QEvent *arg2)
  {
    if (cb_eventFilter_2411_0.can_issue()) {
      return cb_eventFilter_2411_0.issue<QNetworkCookieJar_Adaptor, bool, QObject *, QEvent *>(&QNetworkCookieJar_Adaptor::cbs_eventFilter_2411_0, arg1, arg2);
    } else {
      return QNetworkCookieJar::eventFilter(arg1, arg2);
    }
  }

  //  [adaptor impl] bool QNetworkCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
  bool cbs_setCookiesFromUrl_4950_0(const QList<QNetworkCookie> &cookieList, const QUrl &url)
  {
    return QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
  }

  virtual bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
  {
    if (cb_setCookiesFromUrl_4950_0.can_issue()) {
      return cb_setCookiesFromUrl_4950_0.issue<QNetworkCookieJar_Adaptor, bool, const QList<QNetworkCookie> &, const QUrl &>(&QNetworkCookieJar_Adaptor::cbs_setCookiesFromUrl_4950_0, cookieList, url);
    } else {
      return QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
    }
  }

  //  [adaptor impl] void QNetworkCookieJar::childEvent(QChildEvent *)
  void cbs_childEvent_1701_0(QChildEvent *arg1)
  {
    QNetworkCookieJar::childEvent(arg1);
  }

  virtual void childEvent(QChildEvent *arg1)
  {
    if (cb_childEvent_1701_0.can_issue()) {
      cb_childEvent_1701_0.issue<QNetworkCookieJar_Adaptor, QChildEvent *>(&QNetworkCookieJar_Adaptor::cbs_childEvent_1701_0, arg1);
    } else {
      QNetworkCookieJar::childEvent(arg1);
    }
  }

  //  [adaptor impl] void QNetworkCookieJar::customEvent(QEvent *)
  void cbs_customEvent_1217_0(QEvent *arg1)
  {
    QNetworkCookieJar::customEvent(arg1);
  }

  virtual void customEvent(QEvent *arg1)
  {
    if (cb_customEvent_1217_0.can_issue()) {
      cb_customEvent_1217_0.issue<QNetworkCookieJar_Adaptor, QEvent *>(&QNetworkCookieJar_Adaptor::cbs_customEvent_1217_0, arg1);
    } else {
      QNetworkCookieJar::customEvent(arg1);
    }
  }

  //  [emitter impl] void QNetworkCookieJar::destroyed(QObject *)
  void emitter_QNetworkCookieJar_destroyed_1302(QObject *arg1)
  {
    emit QNetworkCookieJar::destroyed(arg1);
  }

  //  [adaptor impl] void QNetworkCookieJar::disconnectNotify(const char *signal)
  void cbs_disconnectNotify_1731_0(const char *signal)
  {
    QNetworkCookieJar::disconnectNotify(signal);
  }

  virtual void disconnectNotify(const char *signal)
  {
    if (cb_disconnectNotify_1731_0.can_issue()) {
      cb_disconnectNotify_1731_0.issue<QNetworkCookieJar_Adaptor, const char *>(&QNetworkCookieJar_Adaptor::cbs_disconnectNotify_1731_0, signal);
    } else {
      QNetworkCookieJar::disconnectNotify(signal);
    }
  }

  //  [adaptor impl] void QNetworkCookieJar::timerEvent(QTimerEvent *)
  void cbs_timerEvent_1730_0(QTimerEvent *arg1)
  {
    QNetworkCookieJar::timerEvent(arg1);
  }

  virtual void timerEvent(QTimerEvent *arg1)
  {
    if (cb_timerEvent_1730_0.can_issue()) {
      cb_timerEvent_1730_0.issue<QNetworkCookieJar_Adaptor, QTimerEvent *>(&QNetworkCookieJar_Adaptor::cbs_timerEvent_1730_0, arg1);
    } else {
      QNetworkCookieJar::timerEvent(arg1);
    }
  }

  gsi::Callback cb_cookiesForUrl_c1701_0;
  gsi::Callback cb_event_1217_0;
  gsi::Callback cb_eventFilter_2411_0;
  gsi::Callback cb_setCookiesFromUrl_4950_0;
  gsi::Callback cb_childEvent_1701_0;
  gsi::Callback cb_customEvent_1217_0;
  gsi::Callback cb_disconnectNotify_1731_0;
  gsi::Callback cb_timerEvent_1730_0;
};

QNetworkCookieJar_Adaptor::~QNetworkCookieJar_Adaptor() { }

//  Constructor QNetworkCookieJar::QNetworkCookieJar(QObject *parent) (adaptor class)

static void _init_ctor_QNetworkCookieJar_Adaptor_1302 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("parent", true, "0");
  decl->add_arg<QObject * > (argspec_0);
  decl->set_return_new<QNetworkCookieJar_Adaptor> ();
}

static void _call_ctor_QNetworkCookieJar_Adaptor_1302 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QObject *arg1 = args ? gsi::arg_reader<QObject * >() (args, heap) : gsi::arg_maker<QObject * >() (0, heap);
  ret.write<QNetworkCookieJar_Adaptor *> (new QNetworkCookieJar_Adaptor (arg1));
}


// exposed QList<QNetworkCookie> QNetworkCookieJar::allCookies()

static void _init_fp_allCookies_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QList<QNetworkCookie> > ();
}

static void _call_fp_allCookies_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QList<QNetworkCookie> > ((QList<QNetworkCookie>)((QNetworkCookieJar_Adaptor *)cls)->fp_QNetworkCookieJar_allCookies_c0 ());
}


// void QNetworkCookieJar::childEvent(QChildEvent *)

static void _init_cbs_childEvent_1701_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<QChildEvent * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_cbs_childEvent_1701_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QChildEvent *arg1 = args.read<QChildEvent * > (heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QNetworkCookieJar_Adaptor *)cls)->cbs_childEvent_1701_0 (arg1);
}

static void _set_callback_cbs_childEvent_1701_0 (void *cls, const gsi::Callback &cb)
{
  ((QNetworkCookieJar_Adaptor *)cls)->cb_childEvent_1701_0 = cb;
}


// QList<QNetworkCookie> QNetworkCookieJar::cookiesForUrl(const QUrl &url)

static void _init_cbs_cookiesForUrl_c1701_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("url");
  decl->add_arg<const QUrl & > (argspec_0);
  decl->set_return<QList<QNetworkCookie> > ();
}

static void _call_cbs_cookiesForUrl_c1701_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QUrl &arg1 = args.read<const QUrl & > (heap);
  ret.write<QList<QNetworkCookie> > ((QList<QNetworkCookie>)((QNetworkCookieJar_Adaptor *)cls)->cbs_cookiesForUrl_c1701_0 (arg1));
}

static void _set_callback_cbs_cookiesForUrl_c1701_0 (void *cls, const gsi::Callback &cb)
{
  ((QNetworkCookieJar_Adaptor *)cls)->cb_cookiesForUrl_c1701_0 = cb;
}


// void QNetworkCookieJar::customEvent(QEvent *)

static void _init_cbs_customEvent_1217_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<QEvent * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_cbs_customEvent_1217_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QEvent *arg1 = args.read<QEvent * > (heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QNetworkCookieJar_Adaptor *)cls)->cbs_customEvent_1217_0 (arg1);
}

static void _set_callback_cbs_customEvent_1217_0 (void *cls, const gsi::Callback &cb)
{
  ((QNetworkCookieJar_Adaptor *)cls)->cb_customEvent_1217_0 = cb;
}


// emitter void QNetworkCookieJar::destroyed(QObject *)

static void _init_emitter_destroyed_1302 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1", true, "0");
  decl->add_arg<QObject * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_emitter_destroyed_1302 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs & /*ret*/) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QObject *arg1 = args ? gsi::arg_reader<QObject * >() (args, heap) : gsi::arg_maker<QObject * >() (0, heap);
  ((QNetworkCookieJar_Adaptor *)cls)->emitter_QNetworkCookieJar_destroyed_1302 (arg1);
}


// void QNetworkCookieJar::disconnectNotify(const char *signal)

static void _init_cbs_disconnectNotify_1731_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("signal");
  decl->add_arg<const char * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_cbs_disconnectNotify_1731_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const char *arg1 = args.read<const char * > (heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QNetworkCookieJar_Adaptor *)cls)->cbs_disconnectNotify_1731_0 (arg1);
}

static void _set_callback_cbs_disconnectNotify_1731_0 (void *cls, const gsi::Callback &cb)
{
  ((QNetworkCookieJar_Adaptor *)cls)->cb_disconnectNotify_1731_0 = cb;
}


// bool QNetworkCookieJar::event(QEvent *)

static void _init_cbs_event_1217_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<QEvent * > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_cbs_event_1217_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QEvent *arg1 = args.read<QEvent * > (heap);
  ret.write<bool > ((bool)((QNetworkCookieJar_Adaptor *)cls)->cbs_event_1217_0 (arg1));
}

static void _set_callback_cbs_event_1217_0 (void *cls, const gsi::Callback &cb)
{
  ((QNetworkCookieJar_Adaptor *)cls)->cb_event_1217_0 = cb;
}


// bool QNetworkCookieJar::eventFilter(QObject *, QEvent *)

static void _init_cbs_eventFilter_2411_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<QObject * > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("arg2");
  decl->add_arg<QEvent * > (argspec_1);
  decl->set_return<bool > ();
}

static void _call_cbs_eventFilter_2411_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QObject *arg1 = args.read<QObject * > (heap);
  QEvent *arg2 = args.read<QEvent * > (heap);
  ret.write<bool > ((bool)((QNetworkCookieJar_Adaptor *)cls)->cbs_eventFilter_2411_0 (arg1, arg2));
}

static void _set_callback_cbs_eventFilter_2411_0 (void *cls, const gsi::Callback &cb)
{
  ((QNetworkCookieJar_Adaptor *)cls)->cb_eventFilter_2411_0 = cb;
}


// exposed int QNetworkCookieJar::receivers(const char *signal)

static void _init_fp_receivers_c1731 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("signal");
  decl->add_arg<const char * > (argspec_0);
  decl->set_return<int > ();
}

static void _call_fp_receivers_c1731 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const char *arg1 = gsi::arg_reader<const char * >() (args, heap);
  ret.write<int > ((int)((QNetworkCookieJar_Adaptor *)cls)->fp_QNetworkCookieJar_receivers_c1731 (arg1));
}


// exposed QObject *QNetworkCookieJar::sender()

static void _init_fp_sender_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QObject * > ();
}

static void _call_fp_sender_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QObject * > ((QObject *)((QNetworkCookieJar_Adaptor *)cls)->fp_QNetworkCookieJar_sender_c0 ());
}


// exposed void QNetworkCookieJar::setAllCookies(const QList<QNetworkCookie> &cookieList)

static void _init_fp_setAllCookies_3357 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("cookieList");
  decl->add_arg<const QList<QNetworkCookie> & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_fp_setAllCookies_3357 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QList<QNetworkCookie> &arg1 = gsi::arg_reader<const QList<QNetworkCookie> & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QNetworkCookieJar_Adaptor *)cls)->fp_QNetworkCookieJar_setAllCookies_3357 (arg1);
}


// bool QNetworkCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)

static void _init_cbs_setCookiesFromUrl_4950_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("cookieList");
  decl->add_arg<const QList<QNetworkCookie> & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("url");
  decl->add_arg<const QUrl & > (argspec_1);
  decl->set_return<bool > ();
}

static void _call_cbs_setCookiesFromUrl_4950_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QList<QNetworkCookie> &arg1 = args.read<const QList<QNetworkCookie> & > (heap);
  const QUrl &arg2 = args.read<const QUrl & > (heap);
  ret.write<bool > ((bool)((QNetworkCookieJar_Adaptor *)cls)->cbs_setCookiesFromUrl_4950_0 (arg1, arg2));
}

static void _set_callback_cbs_setCookiesFromUrl_4950_0 (void *cls, const gsi::Callback &cb)
{
  ((QNetworkCookieJar_Adaptor *)cls)->cb_setCookiesFromUrl_4950_0 = cb;
}


// void QNetworkCookieJar::timerEvent(QTimerEvent *)

static void _init_cbs_timerEvent_1730_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<QTimerEvent * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_cbs_timerEvent_1730_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QTimerEvent *arg1 = args.read<QTimerEvent * > (heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QNetworkCookieJar_Adaptor *)cls)->cbs_timerEvent_1730_0 (arg1);
}

static void _set_callback_cbs_timerEvent_1730_0 (void *cls, const gsi::Callback &cb)
{
  ((QNetworkCookieJar_Adaptor *)cls)->cb_timerEvent_1730_0 = cb;
}


namespace gsi
{

gsi::Class<QNetworkCookieJar> &qtdecl_QNetworkCookieJar ();

static gsi::Methods methods_QNetworkCookieJar_Adaptor () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QNetworkCookieJar::QNetworkCookieJar(QObject *parent)\nThis method creates an object of class QNetworkCookieJar.", &_init_ctor_QNetworkCookieJar_Adaptor_1302, &_call_ctor_QNetworkCookieJar_Adaptor_1302);
  methods += new qt_gsi::GenericMethod ("*allCookies", "@brief Method QList<QNetworkCookie> QNetworkCookieJar::allCookies()\nThis method is protected and can only be called from inside a derived class.", true, &_init_fp_allCookies_c0, &_call_fp_allCookies_c0);
  methods += new qt_gsi::GenericMethod ("*childEvent", "@brief Virtual method void QNetworkCookieJar::childEvent(QChildEvent *)\nThis method can be reimplemented in a derived class.", false, &_init_cbs_childEvent_1701_0, &_call_cbs_childEvent_1701_0);
  methods += new qt_gsi::GenericMethod ("*childEvent", "@hide", false, &_init_cbs_childEvent_1701_0, &_call_cbs_childEvent_1701_0, &_set_callback_cbs_childEvent_1701_0);
  methods += new qt_gsi::GenericMethod ("cookiesForUrl", "@brief Virtual method QList<QNetworkCookie> QNetworkCookieJar::cookiesForUrl(const QUrl &url)\nThis method can be reimplemented in a derived class.", true, &_init_cbs_cookiesForUrl_c1701_0, &_call_cbs_cookiesForUrl_c1701_0);
  methods += new qt_gsi::GenericMethod ("cookiesForUrl", "@hide", true, &_init_cbs_cookiesForUrl_c1701_0, &_call_cbs_cookiesForUrl_c1701_0, &_set_callback_cbs_cookiesForUrl_c1701_0);
  methods += new qt_gsi::GenericMethod ("*customEvent", "@brief Virtual method void QNetworkCookieJar::customEvent(QEvent *)\nThis method can be reimplemented in a derived class.", false, &_init_cbs_customEvent_1217_0, &_call_cbs_customEvent_1217_0);
  methods += new qt_gsi::GenericMethod ("*customEvent", "@hide", false, &_init_cbs_customEvent_1217_0, &_call_cbs_customEvent_1217_0, &_set_callback_cbs_customEvent_1217_0);
  methods += new qt_gsi::GenericMethod ("emit_destroyed", "@brief Emitter for signal void QNetworkCookieJar::destroyed(QObject *)\nCall this method to emit this signal.", false, &_init_emitter_destroyed_1302, &_call_emitter_destroyed_1302);
  methods += new qt_gsi::GenericMethod ("*disconnectNotify", "@brief Virtual method void QNetworkCookieJar::disconnectNotify(const char *signal)\nThis method can be reimplemented in a derived class.", false, &_init_cbs_disconnectNotify_1731_0, &_call_cbs_disconnectNotify_1731_0);
  methods += new qt_gsi::GenericMethod ("*disconnectNotify", "@hide", false, &_init_cbs_disconnectNotify_1731_0, &_call_cbs_disconnectNotify_1731_0, &_set_callback_cbs_disconnectNotify_1731_0);
  methods += new qt_gsi::GenericMethod ("event", "@brief Virtual method bool QNetworkCookieJar::event(QEvent *)\nThis method can be reimplemented in a derived class.", false, &_init_cbs_event_1217_0, &_call_cbs_event_1217_0);
  methods += new qt_gsi::GenericMethod ("event", "@hide", false, &_init_cbs_event_1217_0, &_call_cbs_event_1217_0, &_set_callback_cbs_event_1217_0);
  methods += new qt_gsi::GenericMethod ("eventFilter", "@brief Virtual method bool QNetworkCookieJar::eventFilter(QObject *, QEvent *)\nThis method can be reimplemented in a derived class.", false, &_init_cbs_eventFilter_2411_0, &_call_cbs_eventFilter_2411_0);
  methods += new qt_gsi::GenericMethod ("eventFilter", "@hide", false, &_init_cbs_eventFilter_2411_0, &_call_cbs_eventFilter_2411_0, &_set_callback_cbs_eventFilter_2411_0);
  methods += new qt_gsi::GenericMethod ("*receivers", "@brief Method int QNetworkCookieJar::receivers(const char *signal)\nThis method is protected and can only be called from inside a derived class.", true, &_init_fp_receivers_c1731, &_call_fp_receivers_c1731);
  methods += new qt_gsi::GenericMethod ("*sender", "@brief Method QObject *QNetworkCookieJar::sender()\nThis method is protected and can only be called from inside a derived class.", true, &_init_fp_sender_c0, &_call_fp_sender_c0);
  methods += new qt_gsi::GenericMethod ("*setAllCookies", "@brief Method void QNetworkCookieJar::setAllCookies(const QList<QNetworkCookie> &cookieList)\nThis method is protected and can only be called from inside a derived class.", false, &_init_fp_setAllCookies_3357, &_call_fp_setAllCookies_3357);
  methods += new qt_gsi::GenericMethod ("setCookiesFromUrl", "@brief Virtual method bool QNetworkCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)\nThis method can be reimplemented in a derived class.", false, &_init_cbs_setCookiesFromUrl_4950_0, &_call_cbs_setCookiesFromUrl_4950_0);
  methods += new qt_gsi::GenericMethod ("setCookiesFromUrl", "@hide", false, &_init_cbs_setCookiesFromUrl_4950_0, &_call_cbs_setCookiesFromUrl_4950_0, &_set_callback_cbs_setCookiesFromUrl_4950_0);
  methods += new qt_gsi::GenericMethod ("*timerEvent", "@brief Virtual method void QNetworkCookieJar::timerEvent(QTimerEvent *)\nThis method can be reimplemented in a derived class.", false, &_init_cbs_timerEvent_1730_0, &_call_cbs_timerEvent_1730_0);
  methods += new qt_gsi::GenericMethod ("*timerEvent", "@hide", false, &_init_cbs_timerEvent_1730_0, &_call_cbs_timerEvent_1730_0, &_set_callback_cbs_timerEvent_1730_0);
  return methods;
}

gsi::Class<QNetworkCookieJar_Adaptor> decl_QNetworkCookieJar_Adaptor (qtdecl_QNetworkCookieJar (), "QtNetwork", "QNetworkCookieJar",
  methods_QNetworkCookieJar_Adaptor (),
  "@qt\n@brief Binding of QNetworkCookieJar");

}

