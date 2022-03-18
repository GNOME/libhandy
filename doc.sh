#!/bin/bash

DOC_DIR=public/doc
REFS="
main
libhandy-1-6
"

LEGACY_REFS="
libhandy-1-0
libhandy-1-2
libhandy-1-4
"

LATEST_STABLE_1=1.6

IFS='
'

mkdir -p $DOC_DIR

for REF in $REFS; do
  API_VERSION=`echo $REF | sed 's/libhandy-\([0-9][0-9]*\)-\([0-9][0-9]*\)/\1.\2/'`

  curl -L --output "$REF.zip" "https://gitlab.gnome.org/GNOME/libhandy/-/jobs/artifacts/$REF/download?job=doc"
  unzip -d "$REF" "$REF.zip"
  mv "$REF/_doc" $DOC_DIR/$API_VERSION

  rm "$REF.zip"
  rm -rf "$REF"
done

for REF in $LEGACY_REFS; do
  API_VERSION=`echo $REF | sed 's/libhandy-\([0-9][0-9]*\)-\([0-9][0-9]*\)/\1.\2/'`

  curl -L --output "$REF.zip" "https://gitlab.gnome.org/GNOME/libhandy/-/jobs/artifacts/$REF/download?job=build-gtkdoc"
  unzip -d "$REF" "$REF.zip"
  mv "$REF/_reference" $DOC_DIR/$API_VERSION

  rm "$REF.zip"
  rm -rf "$REF"

  find $DOC_DIR/$API_VERSION -type f -print0 | xargs -0 sed -i 's|\.\./gdk3/|https://developer.gnome.org/gdk3/stable/|g'
  find $DOC_DIR/$API_VERSION -type f -print0 | xargs -0 sed -i 's|\.\./gdk-pixbuf/|https://developer.gnome.org/gdk-pixbuf/stable/|g'
  find $DOC_DIR/$API_VERSION -type f -print0 | xargs -0 sed -i 's|\.\./gio/|https://developer.gnome.org/gio/stable/|g'
  find $DOC_DIR/$API_VERSION -type f -print0 | xargs -0 sed -i 's|\.\./glib/|https://developer.gnome.org/glib/stable/|g'
  find $DOC_DIR/$API_VERSION -type f -print0 | xargs -0 sed -i 's|\.\./gobject/|https://developer.gnome.org/gobject/stable/|g'
  find $DOC_DIR/$API_VERSION -type f -print0 | xargs -0 sed -i 's|\.\./gtk3/|https://developer.gnome.org/gtk3/stable/|g'
  find $DOC_DIR/$API_VERSION -type f -print0 | xargs -0 sed -i 's|\.\./pango/|https://developer.gnome.org/pango/stable/|g'
done

ln -s $LATEST_STABLE_1 $DOC_DIR/1-latest
ln -s main $DOC_DIR/master
