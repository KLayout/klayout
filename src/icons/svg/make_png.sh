#!/bin/bash -e

output=../images

for svg_file in *.svg; do

  png_file=${output}/${svg_file/.svg/.png}
  png_file_2x=${output}/${svg_file/.svg/@2x.png}

  if ! [ -e ${png_file} ] || [ ${svg_file} -nt ${png_file} ]; then
    echo "Converting $svg_file to $png_file .."
    inkscape -o ${png_file} ${svg_file} -d 96 -C
  fi

  if ! [ -e ${png_file_2x} ] || [ ${svg_file} -nt ${png_file_2x} ]; then
    echo "Converting $svg_file to $png_file_2x .."
    inkscape -o ${png_file_2x} ${svg_file} -d 192 -C
  fi

done

