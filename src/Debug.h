#ifndef __AgoraDEBUG_INCLUDED__
#define __AgoraDEBUG_INCLUDED__

#include <stdarg.h>
#include "Arduino.h"

#define AGORA_DEBUG_LEVEL_NONE 0
#define AGORA_DEBUG_LEVEL_ERROR 1
#define AGORA_DEBUG_LEVEL_INFO 2
#define AGORA_DEBUG_LEVEL_VERBOSE 3

int AGORA_DEBUG_level = AGORA_DEBUG_LEVEL_VERBOSE;

void DEBUG(int level, char c);
void DEBUG(int level, char c, int i);
void DEBUG(int level, int i);
/* void DEBUG(int level, char *ch);
void DEBUG(int level, const char *ch); */
void DEBUG(int level, const char *format, ...);

void DEBUG(int level, char c)
{
  if (level > AGORA_DEBUG_level)
    return;

  Serial.print(c);
}

void DEBUG(int level, char c, int i)
{
  if (level > AGORA_DEBUG_level)
    return;

  Serial.print(c, i);
}

void DEBUG(int level, int i)
{
  if (level > AGORA_DEBUG_level)
    return;

  Serial.print(i);
}
/* void DEBUG(int level, char *ch)
{
  if (level > AGORA_DEBUG_level)
    return;
  Serial.print(ch);
} */
/* void DEBUG(int level, const char *ch)
{
  if (level > AGORA_DEBUG_level)
    return;

  Serial.print(ch);
}
 */
void DEBUG(int level, const char *format, ...)
{
  if (level > AGORA_DEBUG_level)
    return;

  va_list args;
  va_start(args, format);
  Serial.vprintf(format, args);
  va_end(args);
  Serial.println();
}
#endif