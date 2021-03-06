// example1.cc
// simple point tracking program 

#include <stdio.h>

#include "owl.h"

// change these to match your configuration

#define MARKER_COUNT 32
#define SERVER_NAME "169.229.222.231"
#define INIT_FLAGS 0

void owl_print_error(const char *s, int n);

int main()
{
  OWLMarker markers[32];
  int tracker;

  if(owlInit(SERVER_NAME, INIT_FLAGS) < 0) return 0;

  // create tracker 0
  tracker = 0;
  owlTrackeri(tracker, OWL_CREATE, OWL_POINT_TRACKER);

  // set markers
  for(int i = 0; i < MARKER_COUNT; i++)
    owlMarkeri(MARKER(tracker, i), OWL_SET_LED, i);

  // activate tracker
  owlTracker(tracker, OWL_ENABLE);

  // flush requests and check for errors
  if(!owlGetStatus())
    {
      owl_print_error("error in point tracker setup", owlGetError());
      return 0;
    }

  // set default frequency
  owlSetFloat(OWL_FREQUENCY, OWL_MAX_FREQUENCY);
  
  // start streaming
  owlSetInteger(OWL_STREAMING, OWL_ENABLE);

  // main loop
  while(1)
    {
      int err;

      // get some markers
      int n = owlGetMarkers(markers, 32);
      
      // check for error
      if((err = owlGetError()) != OWL_NO_ERROR)
	{
	  owl_print_error("error", err);
	  break;
	}

      // no data yet
      if(n == 0)
	{
	  continue;
	}

      if(n > 0)
	{
	  printf("%d marker(s):\n", n);
	  for(int i = 0; i < n; i++)
	    if(markers[i].cond > 0)
	      printf("%d) %f %f %f\n", i, markers[i].x, markers[i].y, markers[i].z);
	  printf("\n");
	}
    }
  
  // cleanup
  owlDone();
}

void owl_print_error(const char *s, int n)
{
  if(n < 0) printf("%s: %d\n", s, n);
  else if(n == OWL_NO_ERROR) printf("%s: No Error\n", s);
  else if(n == OWL_INVALID_VALUE) printf("%s: Invalid Value\n", s);
  else if(n == OWL_INVALID_ENUM) printf("%s: Invalid Enum\n", s);
  else if(n == OWL_INVALID_OPERATION) printf("%s: Invalid Operation\n", s);
  else printf("%s: 0x%x\n", s, n);
}
