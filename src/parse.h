#ifndef PARSE_H
#define PARSE_H

#include "microstruct.h"

void parse(struct ms *res, char *path);
void pretty_print(struct ms res);
void replace_var(struct ms *res);
void replace_var_commands(struct ms *res);
void replace_var_rule(struct ms *res);

#endif /* PARSE_H */
