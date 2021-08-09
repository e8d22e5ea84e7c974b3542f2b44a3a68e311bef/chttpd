#include "text.h"

size_t uri_decode (const char *src, const size_t len, char *dst)
{
  size_t i = 0, j = 0;
  while(i < len)
  {
    int copy_char = 1;
    if(src[i] == '%' && i + 2 < len)
    {
      const unsigned char v1 = hexval[ (unsigned char)src[i+1] ];
      const unsigned char v2 = hexval[ (unsigned char)src[i+2] ];

      /* skip invalid hex sequences */
      if ((v1 | v2) != 0xFF)
      {
        dst[j] = (v1 << 4) | v2;
        j++;
        i += 3;
        copy_char = 0;
      }
    }
    if (copy_char)
    {
      dst[j] = src[i];
      i++;
      j++;
    }
  }
  dst[j] = '\0';
  return j;
}

/* Strip left-coming whitespace. Increments your pointer. */
char *rwhitespace(char *hay)
{
    size_t len = 0;

    while (isspace(hay[len]))
        len++;

    return (hay+len);
}

/* Generate random text. buf must point to a mutable string. Len must not be above buf's capacity! */
char *randtext(char *buf, size_t len)
{
    srand(time(NULL));
    size_t alphabet_size = sizeof(alphabet) - 1;

    for (size_t i = 0; i < len-1; i++)
    {
        size_t index = rand() % alphabet_size;
        buf[i] = alphabet[index];
    }

    buf[len] = '\0';

    return buf;
}

/* Return the last, non-zero character in a string. Very simple. */
char lastchar(char *str)
{
    return (str[strlen(str)-1]);
}

/* Is string two equal to one, or vice versa? */
bool strequ(const char *one, const char *two)
{
    return (!strcmp(one, two));
}

/* Is a this string a subset of another? */
bool strin(const char *one, const char *two)
{
    return (strstr(one, two) != NULL);
}

/* Convert a file extension (with a dot) to an HTTP mime. */
const char *ext2mime(const char *extension)
{
    /* For text mimes. */
    for (int i = 0; i < sizeof(mime_text_types)/sizeof(char*); i++)
    {
        if (strequ(mime_text_types[i], extension))
            return mime_text;
    }

    if (strequ(extension, ".html"))
        return mime_html;
    
    if (strequ(extension, ".png"))
        return mime_png;
    
    if (strequ(extension, ".jpg") || strequ(extension, ".jpeg"))
        return mime_jpg;

    if (strequ(extension, ".gif"))
        return mime_gif;

    if (strequ(extension, ".webm"))
        return mime_webm;

    if (strequ(extension, ".mp3"))
        return mime_mp3;
    
    if (strequ(extension, ".wav"))
        return mime_wav;

    return mime_binary; // octet-stream; binary files
}