#include "version.h"

static const char OUTCURSES_VERSION[] = 
 Outcurses_VERSION_MAJOR "."
 Outcurses_VERSION_MINOR "."
 Outcurses_VERSION_PATCH;

const char* outcurses_version(void){
  return OUTCURSES_VERSION;
}
