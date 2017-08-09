
#include "dbLayout.h"
#include "dbReader.h"
#include "dbOASISWriter.h"
#include "tlTimer.h"

#include <memory>

void
syntax ()
{
  printf ("Syntax: strm2oas [-o <optimization-level>] [-c] <infile> <outfile>\n");
  printf ("\n");
  printf ("  -o n   Specify optimization level (0..10, default is 2)\n");
  printf ("  -c     Use CBLOCK compression\n");
  printf ("  -s     Use strict mode\n");
  printf ("  -r     Recompression (ignore existing arrays)\n");
  printf ("  -v     Verbose - print timing information\n");
}

int 
main (int argc, char *argv [])
{
  std::string infile, outfile;
  bool verbose = false;

  try {

    db::OASISWriterOptions writer_options;

    for (int i = 1; i < argc; ++i) {
      std::string o (argv[i]);
      if (o == "-o") {
        if (i < argc - 1) {
          ++i;
          tl::from_string (argv[i], writer_options.compression_level);
        }
      } else if (o == "-v") {
        verbose = true;
      } else if (o == "-c") {
        writer_options.write_cblocks = true;
      } else if (o == "-s") {
        writer_options.strict_mode = true;
      } else if (o == "-r") {
        writer_options.recompress = true;
      } else if (o == "-h" || o == "-help" || o == "--help") {
        syntax ();
        return 0;
      } else if (argv[i][0] == '-') {
        throw tl::Exception ("Unknown option: %s - use '-h' for help", (const char *) argv[i]);
      } else if (infile.empty ()) {
        infile = argv[i];
      } else if (outfile.empty ()) {
        outfile = argv[i];
      } else {
        throw tl::Exception ("Superfluous argument: %s - use '-h' for help", (const char *) argv[i]);
      }
    }

    if (infile.empty ()) {
        throw tl::Exception ("Input file not given");
    }
    if (outfile.empty ()) {
        throw tl::Exception ("Output file not given");
    }

    db::Manager m;
    db::Layout layout (false, &m);
    db::LayerMap map;

    {
      std::auto_ptr<tl::SelfTimer> timer;
      if (verbose) {
        timer.reset (new tl::SelfTimer ("Reading input layout"));
      }
      tl::InputStream stream (infile);
      db::Reader reader (stream);
      map = reader.read (layout);
    }

    {
      db::SaveLayoutOptions options;
      options.set_specific_options (writer_options);

      std::auto_ptr<tl::SelfTimer> timer;
      if (verbose) {
        timer.reset (new tl::SelfTimer ("Writing OAS"));
      }
      tl::OutputStream stream (outfile);
      db::OASISWriter writer;
      writer.write (layout, stream, options);
    }

  } catch (std::exception &ex) {
    fprintf (stderr, "*** ERROR: %s\n", ex.what ());
    return 1;
  } catch (tl::Exception &ex) {
    fprintf (stderr, "*** ERROR: %s\n", ex.msg ().c_str ());
    return 1;
  } catch (...) {
    fprintf (stderr, "*** ERROR: unspecific error\n");
    return 1;
  }

  return 0;
}


