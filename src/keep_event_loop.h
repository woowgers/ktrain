#ifndef EVENT_LOOP
#define EVENT_LOOP


#include <stdbool.h>


bool keep_event_loop();
void stop_event_loop();
void continue_event_loop();


#endif /* EVENT_LOOP */
