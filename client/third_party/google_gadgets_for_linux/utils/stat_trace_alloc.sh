#!/bin/sh
awk '
flag == 1 && ! /valgrind/ && ! /small_object/ {
  count[$0]++;
  flag = 0;
}

/TRACE_ALLOC:/ {
  flag = 1;
}

END {
  for (i in count) {
    print count[i], i;
  }
}
' < $1 | sort -n -r

