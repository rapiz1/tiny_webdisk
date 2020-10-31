#ifndef _TINY_WEB_DISK_LOGGER_H__
#define _TINY_WEB_DISK_LOGGER_H__

#include <stdio.h>

#define info(format_string, ...) printf("[info] %s:%d:\t" format_string "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define warn(format_string, ...) printf("[warn] %s:%d:\t" format_string "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define error(format_string, ...) printf("[error] %s:%d:\t" format_string "\n", __FILE__, __LINE__, ##__VA_ARGS__)

void log_perror(const char* msg);

#endif
