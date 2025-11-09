
TEMPLATE = subdirs

SUBDIRS = \
  bd \
  strm2cif \
  strm2dxf \
  strm2gds \
  strm2gdstxt \
  strm2oas \
  strm2lstr \
  strm2mag \
  strm2txt \
  strmclip \
  strmcmp \
  strmxor \
  strmrun \

strm2cif.depends += bd
strm2dxf.depends += bd
strm2gds.depends += bd
strm2gdstxt.depends += bd
strm2oas.depends += bd
strm2lstr.depends += bd
strm2mag.depends += bd
strm2txt.depends += bd
strmclip.depends += bd
strmcmp.depends += bd
strmxor.depends += bd
strmrun.depends += bd
