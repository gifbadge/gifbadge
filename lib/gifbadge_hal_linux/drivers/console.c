/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

/*-----------------------------------------------------------
* Example console I/O wrappers.
*----------------------------------------------------------*/

#include <stdarg.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <malloc.h>
#include <string.h>

SemaphoreHandle_t xStdioMutex;
StaticSemaphore_t xStdioMutexBuffer;

void console_init( void )
{
#if( configSUPPORT_STATIC_ALLOCATION == 1 )
  {
        xStdioMutex = xSemaphoreCreateMutexStatic( &xStdioMutexBuffer );
    }
#else /* if( configSUPPORT_STATIC_ALLOCATION == 1 ) */
  {
    xStdioMutex = xSemaphoreCreateMutex( );
  }
#endif /* if( configSUPPORT_STATIC_ALLOCATION == 1 ) */
}

void console_print(const char * tag, const char * fmt,
                    ... )
{
  va_list vargs;

  va_start( vargs, fmt );

  xSemaphoreTake( xStdioMutex, portMAX_DELAY );
  char *fmt_tmp = malloc(strlen(fmt)+strlen(tag)+3);
  sprintf(fmt_tmp, "%s %s\n",tag, fmt);

  vprintf( fmt_tmp, vargs );
  free(fmt_tmp);
  xSemaphoreGive( xStdioMutex );

  va_end( vargs );
}