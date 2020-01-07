#include <gtk/gtk.h>
#define HANDY_USE_UNSTABLE_API
#include <handy.h>

int
main (int    argc,
      char **argv)
{
   hdy_keypad_new (TRUE, FALSE);
}
