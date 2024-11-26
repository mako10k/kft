#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdlib.h>

#define KFT_ENVNAME_PREFIX "KFT_"
#define KFT_ENVNAME_SHELL KFT_ENVNAME_PREFIX "SHELL"
#define KFT_ENVNAME_SHELL_RAW "SHELL"
#define KFT_ENVNAME_ESCAPE KFT_ENVNAME_PREFIX "ESCAPE"
#define KFT_ENVNAME_BEGIN KFT_ENVNAME_PREFIX "BEGIN"
#define KFT_ENVNAME_END KFT_ENVNAME_PREFIX "END"

#define KFT_OPTDEF_SHELL "/bin/sh"
#define KFT_OPTDEF_ESCAPE '\\'
#define KFT_OPTDEF_BEGIN "{{"
#define KFT_OPTDEF_END "}}"

#define KFT_VARNAME_INPUT "INPUT"
#define KFT_VARNAME_OUTPUT "OUTPUT"

#define KFT_FAILURE (-1)
#define KFT_SUCCESS 0
#define KFT_EOL 1