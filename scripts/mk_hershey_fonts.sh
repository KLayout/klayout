#!/bin/bash -e

inst_dir=$(dirname $(which $0))

fonts=(rowmant timesr timesi futural futuram gothiceng rowmand)

for font in ${fonts[@]}; do

  svn cat https://github.com/kamalmostafa/hershey-fonts.git/trunk/hershey-fonts/$font.jhf >$font.jhf
  $inst_dir/jhf2c.pl $font
  rm $font.jhf

done

