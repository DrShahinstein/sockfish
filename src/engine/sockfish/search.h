#pragma once

/* forward declarations */
typedef struct Move Move;
typedef struct SF_Context SF_Context;
//

#include "sockfish.h"

Move sf_search(const SF_Context *ctx);