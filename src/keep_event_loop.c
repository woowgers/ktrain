#include "keep_event_loop.h"


static bool g_keep_event_loop = true;


bool keep_event_loop()
{
  return g_keep_event_loop;
}

void stop_event_loop()
{
  g_keep_event_loop = false;
}

void continue_event_loop()
{
  g_keep_event_loop = true;
}
