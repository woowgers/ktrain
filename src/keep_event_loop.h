#ifndef EVENT_LOOP
#define EVENT_LOOP


#include <stdbool.h>


static bool _keep_event_loop = true;


bool keep_event_loop();
void finish_event_loop();


#endif /* EVENT_LOOP */
