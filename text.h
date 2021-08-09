#ifndef MIME_H
#define MIME_H

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

char *rwhitespace(char *hay);
char *randtext(char *buf, size_t len);
char lastchar(char *str);
bool strequ(const char *one, const char *two);
bool strin(const char *one, const char *two);

const char *ext2mime(const char *extension);
size_t uri_decode (const char *src, const size_t len, char *dst);

/* I don't know why it works, but it works! */
/* I will write my own percent-encoding decoder later, but this is magic. */

#define __ 0xFF
static const unsigned char hexval[0x100] = {
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 00-0F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 10-1F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 20-2F */
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,__,__,__,__,__,__, /* 30-3F */
  __,10,11,12,13,14,15,__,__,__,__,__,__,__,__,__, /* 40-4F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 50-5F */
  __,10,11,12,13,14,15,__,__,__,__,__,__,__,__,__, /* 60-6F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 70-7F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 80-8F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* 90-9F */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* A0-AF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* B0-BF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* C0-CF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* D0-DF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* E0-EF */
  __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__, /* F0-FF */
};
#undef __

static const char alphabet[] = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM";

/* These extensions will always be shown as text. */
static const char *mime_text_types[] = {
    ".txt",
    ".c",
    ".cc",
    ".cxx"
    ".py",
    ".cpp",
    ".php",
    ".rs",
    ".h",
    ".hxx",
    ".hpp",
    ".s"
};

static const char mime_text[] = "text/plain";
static const char mime_html[] = "text/html";


static const char mime_png[] = "image/png";
static const char mime_jpg[] = "image/jpeg";
static const char mime_gif[] = "image/gif";

static const char mime_mp3[] = "audio/mpeg";
static const char mime_wav[] = "audio/wav";

static const char mime_webm[] = "video/webm";

static const char mime_binary[] = "application/octet-stream";

#endif
