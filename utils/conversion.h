#ifndef __CONV_H
#define __CONV_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "C.tab.h"
#include "nodes.h"
#include "token.h"
#include "environment.h"

#define UNDEFINED INT_MIN + 3

char *to_string(value *);
int to_int(environment *, value *);
TOKEN * cast_from_node(NODE *);
value *int_value(int);
value *int_param(char *, int);
char *correct_string_rep(value *);
value *string_value(char *);
value *join(value *, value *);
int type_of(NODE *);
int vt_type_convert(int);

#endif