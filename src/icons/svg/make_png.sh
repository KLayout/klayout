#!/bin/bash -e

output=../images

for svg_file in *.svg; do

  png_file=${output}/${svg_file/.svg/.png}
  png_file_2x=${output}/${svg_file/.svg/@2x.png}

  echo "Converting $svg_file to $png_file .."
  inkscape -o ${png_file} ${svg_file} -d 96 -C

  echo "Converting $svg_file to $png_file_2x .."
  inkscape -o ${png_file_2x} ${svg_file} -d 192 -C

done

