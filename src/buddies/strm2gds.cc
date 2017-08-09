
#include "dbLayout.h"
#include "dbReader.h"
#include "dbGDS2Writer.h"

int 
main (int argc, char *argv [])
{
  if (argc != 3) {
    printf ("Syntax: strm2gds <infile> <outfile>\n");
    return 1;
  }

  std::string infile (argv[1]);
  std::string outfile (argv[2]);

  try {

    db::Manager m;
    db::Layout layout (&m);
    db::LayerMap map;

    {
      tl::InputStream stream (infile);
      db::Reader reader (stream);
      map = reader.read (layout);
    }

    {
      tl::OutputStream stream (outfile);
      db::GDS2Writer writer;
      writer.write (layout, stream, db::SaveLayoutOptions ());
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


