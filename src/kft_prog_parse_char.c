#include "kft_prog_parse_char.h"
#include <ctype.h>

int kft_parse_char_escoct(kft_parse_context_t *ppc, size_t *pnaccepted,
                          int *pch) {
  int naccepted = *pnaccepted;

  int och = 0;
  int ich = KFT_GETLASTC(ppc);
  if (!isodigit(ich)) {
    *pch = och;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }
  och = ich - '0';
  ich = KFT_ACCEPT(ppc, &naccepted); // o
  if (!isodigit(ich)) {
    *pch = och;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }
  och <<= 3;
  och += ich - '0';
  ich = KFT_ACCEPT(ppc, &naccepted); // oo
  if (!isodigit(ich) || och >= 4) {
    *pch = och;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }
  KFT_ACCEPT(ppc, &naccepted); // ooo
  och <<= 3;
  och += ich - '0';

  *pch = och;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

int kft_parse_char_eschex(kft_parse_context_t *ppc, size_t *pnaccepted,
                          int *pch) {
  int naccepted = *pnaccepted;

  int och = 0;
  int ich = KFT_GETLASTC(ppc);
  if (!isxdigit(ich)) {
    *pch = och;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }
  if (isdigit(ich)) {
    och = ich - '0';
  } else {
    och = tolower(ich) - 'a' + 10;
  }
  ich = KFT_ACCEPT(ppc, &naccepted); // x
  if (!isxdigit(ich)) {
    *pch = och;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }
  och <<= 4;
  if (isdigit(ich)) {
    och += ich - '0';
  } else {
    och += tolower(ich) - 'a' + 10;
  }

  *pch = och;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

int kft_parse_char_esc(kft_parse_context_t *ppc, size_t *pnaccepted, int *pch) {
  size_t naccepted = *pnaccepted;

  int och;
  int ich = KFT_GETLASTC(ppc);
  switch (ich) {
  case EOF:
    return KFT_PARSE_ERROR;

  case 'n':
    och = '\n';
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case 'r':
    och = '\r';
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case 't':
    och = '\t';
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case 'v':
    och = '\v';
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case 'b':
    och = '\b';
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case 'f':
    och = '\f';
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case 'a':
    och = '\a';
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case '\\':
    och = '\\';
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case '\'':
    och = '\'';
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case '"':
    och = '"';
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case '0':
    KFT_ACCEPT(ppc, &naccepted);
    KFT_PARSE(char_escoct, ppc, &naccepted, &och);
    break;

  case 'x':
  case 'X':
    KFT_ACCEPT(ppc, &naccepted);
    KFT_PARSE(char_eschex, ppc, &naccepted, &och);
    break;

  case '?':
    och = '?';
    KFT_ACCEPT(ppc, &naccepted);
    break;

  default:
    return KFT_PARSE_ERROR;
  }

  *pch = och;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

int kft_parse_char(kft_parse_context_t *ppc, size_t *pnaccepted, int *pch) {
  size_t naccepted = *pnaccepted;

  int ch = KFT_GETLASTC(ppc);
  if (ch == EOF) {
    return KFT_PARSE_ERROR;
  }

  if (ch == '\\') {
    KFT_ACCEPT(ppc, &naccepted);
    KFT_PARSE(char_esc, ppc, &naccepted, &ch);
  }
  KFT_ACCEPT(ppc, &naccepted);

  *pch = ch;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}
