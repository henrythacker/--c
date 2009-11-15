#ifndef __ARITH_H
#define __ARITH_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "C.tab.h"
#include "nodes.h"
#include "token.h"
#include "environment.h"
#include "conversion.h"

value *arithmetic(environment *, int, value *, value *);

#endif