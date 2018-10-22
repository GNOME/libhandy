#!/bin/sh

for var in "$@"
do
  echo "#include \"$var\""
done

echo '#include "hdy-main-private.h"

void
hdy_init_public_types (void)
{'

sed -ne 's/^#define \+\(HDY_TYPE_[A-Z0-9_]\+\) \+.*/  g_type_ensure (\1);/p' "$@" | sort

echo '}
'
