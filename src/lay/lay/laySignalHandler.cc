
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

#include "laySignalHandler.h"
#include "layCrashMessage.h"
#include "layVersion.h"
#include "layApplication.h"
#include "tlException.h"
#include "tlString.h"
#include "tlLog.h"
#include "tlFileUtils.h"
#include "tlStream.h"

#ifdef _WIN32
#  include <windows.h>
#  include <DbgHelp.h>
#  include <Psapi.h>
//  get rid of these - we have std::min/max ..
#  ifdef min
#    undef min
#  endif
#  ifdef max
#    undef max
#  endif
#else
#  include <dlfcn.h>
#  include <execinfo.h>
#  include <unistd.h>
#endif

#include <signal.h>
#include <cstdio>
#include <QFileInfo>
#include <QMessageBox>

namespace lay
{

static bool s_sh_has_gui = false;

void enable_signal_handler_gui (bool en)
{
  s_sh_has_gui = en;
}

#if defined(WIN32)

static QString
addr2symname (DWORD64 addr)
{
  const int max_symbol_length = 255;

  SYMBOL_INFO *symbol = (SYMBOL_INFO *) calloc (sizeof (SYMBOL_INFO) + (max_symbol_length + 1) * sizeof (char), 1);
  symbol->MaxNameLen = max_symbol_length;
  symbol->SizeOfStruct = sizeof (SYMBOL_INFO);

  HANDLE process = GetCurrentProcess ();

  QString sym_name;
  DWORD64 d;
  bool has_symbol = false;
  DWORD64 disp = addr;
  if (SymFromAddr(process, addr, &d, symbol)) {
    //  Symbols taken from the export table seem to be unreliable - skip these
    //  and report the module name + offset.
    if (! (symbol->Flags & SYMFLAG_EXPORT)) {
      sym_name = QString::fromLocal8Bit (symbol->Name);
      disp = d;
      has_symbol = true;
    }
  }

  //  find the module name from the module base address

  HMODULE modules[1024];
  DWORD modules_size = 0;
  if (! EnumProcessModules (process, modules, sizeof (modules), &modules_size)) {
    modules_size = 0;
  }

  QString mod_name;
  for (unsigned int i = 0; i < (modules_size / sizeof (HMODULE)); i++) {
    TCHAR mn[MAX_PATH];
    if (GetModuleFileName (modules[i], mn, sizeof (mn) / sizeof (TCHAR))) {
      MODULEINFO mi;
      if (GetModuleInformation (process, modules[i], &mi, sizeof (mi))) {
        if ((DWORD64) mi.lpBaseOfDll <= addr && (DWORD64) mi.lpBaseOfDll + mi.SizeOfImage > addr) {
          mod_name = QFileInfo (QString::fromUtf16 ((unsigned short *) mn)).fileName ();
          if (! has_symbol) {
            disp -= (DWORD64) mi.lpBaseOfDll;
          }
          break;
        }
      }
    }
  }

  if (! mod_name.isNull ()) {
    mod_name = QString::fromUtf8 ("(") + mod_name + QString::fromUtf8 (") ");
  }

  free (symbol);

  return QString::fromUtf8 ("0x%1 - %2%3+%4").
            arg (addr, 0, 16).
            arg (mod_name).
            arg (sym_name).
            arg (disp);
}

QString
get_symbol_name_from_address (const QString &mod_name, size_t addr)
{
  HANDLE process = GetCurrentProcess ();

  DWORD64 mod_base = 0;
  if (! mod_name.isEmpty ()) {

    //  find the module name from the module base address
    HMODULE modules[1024];
    DWORD modules_size = 0;
    if (! EnumProcessModules (process, modules, sizeof (modules), &modules_size)) {
      modules_size = 0;
    }

    for (unsigned int i = 0; i < (modules_size / sizeof (HMODULE)); i++) {
      TCHAR mn[MAX_PATH];
      if (GetModuleFileName (modules[i], mn, sizeof (mn) / sizeof (TCHAR))) {
        if (mod_name == QFileInfo (QString::fromUtf16 ((unsigned short *) mn)).fileName ()) {
          MODULEINFO mi;
          if (GetModuleInformation (process, modules[i], &mi, sizeof (mi))) {
            mod_base = (DWORD64) mi.lpBaseOfDll;
          }
        }
      }
    }

    if (mod_base == 0) {
      throw tl::Exception (tl::to_string (QObject::tr ("Unknown module name: ") + mod_name));
    }

  }

  SymInitialize (process, NULL, TRUE);
  QString res = addr2symname (mod_base + (DWORD64) addr);
  SymCleanup (process);

  return res;
}

LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
  HANDLE process = GetCurrentProcess ();
  SymInitialize (process, NULL, TRUE);

  QString text;
  text += QObject::tr ("Exception code: 0x%1\n").arg (pExceptionInfo->ExceptionRecord->ExceptionCode, 0, 16);
  text += QObject::tr ("Program Version: ") +
          QString::fromUtf8 (lay::Version::name ()) +
          QString::fromUtf8 (" ") +
          QString::fromUtf8 (lay::Version::version ()) +
          QString::fromUtf8 (" (") +
          QString::fromUtf8 (lay::Version::subversion ()) +
          QString::fromUtf8 (")");
#if defined(_WIN64)
  text += QString::fromUtf8 (" AMD64");
#else
  text += QString::fromUtf8 (" x86");
#endif
  text += QString::fromUtf8 ("\n");
  text += QObject::tr ("\nBacktrace:\n");

  CONTEXT context_record = *pExceptionInfo->ContextRecord;

  // Initialize stack walking.
  STACKFRAME64 stack_frame;
  memset(&stack_frame, 0, sizeof(stack_frame));

#if defined(_WIN64)
  int machine_type = IMAGE_FILE_MACHINE_AMD64;
  stack_frame.AddrPC.Offset = context_record.Rip;
  stack_frame.AddrFrame.Offset = context_record.Rbp;
  stack_frame.AddrStack.Offset = context_record.Rsp;
#else
  int machine_type = IMAGE_FILE_MACHINE_I386;
  stack_frame.AddrPC.Offset = context_record.Eip;
  stack_frame.AddrFrame.Offset = context_record.Ebp;
  stack_frame.AddrStack.Offset = context_record.Esp;
#endif
  stack_frame.AddrPC.Mode = AddrModeFlat;
  stack_frame.AddrFrame.Mode = AddrModeFlat;
  stack_frame.AddrStack.Mode = AddrModeFlat;

  while (StackWalk64 (machine_type,
                      GetCurrentProcess(),
                      GetCurrentThread(),
                      &stack_frame,
                      &context_record,
                      NULL,
                      &SymFunctionTableAccess64,
                      &SymGetModuleBase64,
                      NULL)) {
    text += addr2symname (stack_frame.AddrPC.Offset);
    text += QString::fromUtf8 ("\n");
  }

  SymCleanup (process);

  bool has_gui = s_sh_has_gui && lay::ApplicationBase::instance () && lay::ApplicationBase::instance ()->has_gui ();
  if (has_gui) {

    //  YES! I! KNOW!
    //  In a signal handler you shall not do fancy stuff (in particular not
    //  open dialogs) nor shall you throw exceptions! But that scheme appears to
    //  be working since in most cases the signal is raised from our code (hence
    //  from our stack frames) and everything is better than just showing
    //  the "application stopped working" dialog.
    //  Isn't it?

    CrashMessage msg (0, true, text);
    if (! msg.exec ()) {
      //  terminate unconditionally
      return EXCEPTION_EXECUTE_HANDLER;
    } else {
      throw tl::CancelException ();
    }

  } else {

    tl::error << text << tl::noendl;
    return EXCEPTION_EXECUTE_HANDLER;

  }
}

static void handle_signal (int signo)
{
  signal (signo, handle_signal);
  int user_base = (1 << 29);
  RaiseException(signo + user_base, 0, 0, NULL);
}

void install_signal_handlers ()
{
  //  disable any signal handlers that Ruby might have installed.
  signal (SIGSEGV, SIG_DFL);
  signal (SIGILL, SIG_DFL);
  signal (SIGFPE, SIG_DFL);

  signal (SIGABRT, handle_signal);

#if 0
  //  TODO: not available to MinGW - linking against msvc100 would help
  //  but then the app crashes.
  _set_abort_behavior( 0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT );
#endif

  SetUnhandledExceptionFilter(ExceptionHandler);
}

#else

QString get_symbol_name_from_address (const QString &, size_t)
{
  return QString::fromUtf8 ("n/a");
}

void signal_handler (int signo, siginfo_t *si, void *)
{
  void *array [100];

  bool can_resume = (signo != SIGILL);

  size_t nptrs = backtrace (array, sizeof (array) / sizeof (array[0]));

  std::string text;
  text += tl::sprintf ("Signal number: %d\n", signo);
  text += tl::sprintf ("Address: 0x%lx\n", (unsigned long) si->si_addr);
  text += std::string ("Program Version: ") +
            lay::Version::name () + " " +
            lay::Version::version () + " (" + lay::Version::subversion () + ")\n";

  text += std::string ("\nBacktrace:\n");

#if 0

  //  the approach with backtrace_symbols - this does not resolve shared object symbols
  char **symbols = backtrace_symbols (array, nptrs);
  if (symbols == NULL) {
    text += "-- Unable to obtain stack trace --\n";
  } else {
    for (size_t i = 2; i < nptrs; i++) {
      text += std::string (symbols [i]) + "\n";
    }
  }
  free(symbols);

#else

  //  the more elaborate approach using the addr2line external tool to obtain debug information
  //  (if available)

  const char *addr2line_call = "addr2line -C -s -f -e '%s' 0x%lx";

  bool has_addr2line = true;
  for (size_t i = 0; i < nptrs; ++i) {

    Dl_info info;
    dladdr (array [i], &info);

    if (info.dli_fname) {

      char sym [1024], source [1024];
      sym[0] = 0;
      source[0] = 0;

      if (has_addr2line) {

        //  two tries: one with the relative address (for shared object) and one with
        //  absolute address.
        //  TODO: is there a better way to decide how to use addr2line (with executables)?
        for (int abs_addr = 0; abs_addr < 2; ++abs_addr) {

          std::string cmd = tl::sprintf (addr2line_call, info.dli_fname, size_t (array[i]) - (abs_addr ? 0 : size_t (info.dli_fbase)));
          FILE *addr2line_out = popen (cmd.c_str (), "r");
          if (! addr2line_out) {
            has_addr2line = false;
          }

          if (has_addr2line && ! fgets (sym, sizeof (sym) - 1, addr2line_out)) {
            has_addr2line = false;
          }
          if (has_addr2line && ! fgets (source, sizeof (source) - 1, addr2line_out)) {
            has_addr2line = false;
          }

          int l;
          l = strlen (sym);
          if (l > 0 && sym[l - 1] == '\n') {
            sym[l - 1] = 0;
          }
          l = strlen (source);
          if (l > 0 && source[l - 1] == '\n') {
            source[l - 1] = 0;
          }

          if (addr2line_out) {
            fclose (addr2line_out);
          }

          //  addr2line returns '??' on missing symbol - in that case use absolute address mode
          if (sym[0] != '?') {
            break;
          }

        }

      }

      if (has_addr2line) {
        text += tl::sprintf ("%s +0x%lx %s [%s]\n", info.dli_fname, size_t (array[i]) - size_t (info.dli_fbase), (const char *) sym, (const char *) source);
      } else if (info.dli_sname) {
        text += tl::sprintf ("%s +0x%lx %s\n", info.dli_fname, size_t (array[i]) - size_t (info.dli_fbase), info.dli_sname);
      } else {
        text += tl::sprintf ("%s +0x%lx\n", info.dli_fname, size_t (array[i]) - size_t (info.dli_fbase));
      }

    } else {
      text += tl::sprintf ("0x%lx\n", (unsigned long)array[i]);
    }
  }

#endif

  try {

    //  write crash log

    std::string crash_log = tl::combine_path (lay::ApplicationBase::instance () ? lay::ApplicationBase::instance ()->appdata_path () : ".", "klayout_crash.log");

    tl::OutputStream os (crash_log, tl::OutputStream::OM_Plain, true);
    os << text;

    text += "\nCrash log written to " + crash_log;

  } catch (...) {
    //  .. ignore errors
  }

  tl::error << text << tl::noendl;

  bool has_gui = s_sh_has_gui && lay::ApplicationBase::instance () && lay::ApplicationBase::instance ()->has_gui ();
  if (has_gui) {

    //  YES! I! KNOW!
    //  In a signal handler you shall not do fancy stuff (in particular not
    //  open dialogs) nor shall you throw exceptions! But that scheme appears to
    //  be working since in most cases the signal is raised from our code (hence
    //  from our stack frames) and everything is better than just core dumping.
    //  Isn't it?

    lay::ApplicationBase::instance ()->qapp_gui ()->setOverrideCursor (QCursor ());

    std::unique_ptr<CrashMessage> msg;
    msg.reset (new CrashMessage (0, can_resume, tl::to_qstring (text)));

    if (! msg->exec ()) {

      _exit (signo);

    } else {

      sigset_t x;
      sigemptyset (&x);
      sigaddset(&x, signo);
      sigprocmask(SIG_UNBLOCK, &x, NULL);

      throw tl::CancelException ();

    }

  } else {

    _exit (signo);

  }
}

void install_signal_handlers ()
{
  struct sigaction act;
  memset(&act, 0, sizeof(struct sigaction));
  act.sa_sigaction = signal_handler;
  sigemptyset (&act.sa_mask);
  act.sa_flags = SA_SIGINFO;

  sigaction (SIGSEGV, &act, NULL);
  sigaction (SIGILL, &act, NULL);
  sigaction (SIGFPE, &act, NULL);
  sigaction (SIGABRT, &act, NULL);
  sigaction (SIGBUS, &act, NULL);
}

#endif

}
