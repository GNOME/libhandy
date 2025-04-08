#!/bin/bash

DOC_DIR=public/doc
REFS="
main
"

mkdir -p $DOC_DIR

for REF in $REFS; do
  API_VERSION=`echo $REF | sed 's/libhandy-\([0-9][0-9]*\)-\([0-9][0-9]*\)/\1.\2/'`

  curl -L --output "$REF.zip" "https://gitlab.gnome.org/GNOME/libhandy/-/jobs/artifacts/$REF/download?job=doc"
  unzip -d "$REF" "$REF.zip"
  mv "$REF/_doc" $DOC_DIR/$API_VERSION

  rm "$REF.zip"
  rm -rf "$REF"
done

ln -s main $DOC_DIR/1-latest
ln -s main $DOR_DIR/1.8
ln -s main $DOR_DIR/1.6
ln -s main $DOR_DIR/1.4
ln -s main $DOR_DIR/1.2
ln -s main $DOR_DIR/1.0
ln -s main $DOC_DIR/master
