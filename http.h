#ifndef _TINY_WEB_DISK_HTTP_H__
#define _TINY_WEB_DISK_HTTP_H__
#define _GNU_SOURCE

#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define FORBIDDEN 403

void *handle_conn(void* vfd);

#endif
