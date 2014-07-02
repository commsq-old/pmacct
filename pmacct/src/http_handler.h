#ifndef _HTTP_HANDLER_H_
#define _HTTP_HANDLER_H_

#include <ctype.h>

#if (!defined __HTTP_HANDLER_C)
#define EXT extern
#else
#define EXT
#endif

/* prototypes */
EXT char *locate_http_host(char *buf);

#undef EXT

#endif // _HTTP_HANDLER_H_
