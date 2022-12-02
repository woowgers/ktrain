#include "keep_event_loop.h"


bool keep_event_loop()
{
  return _keep_event_loop;
}

void finish_event_loop()
{
  _keep_event_loop = false;
}
