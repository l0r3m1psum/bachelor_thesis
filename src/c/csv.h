#ifndef CSV_INCLUDE
#define CSV_INCLUDE

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/* NOTE: maybe I should add an "_t" to csv_type and csv_num */

enum {
	CSV_INT64,
	CSV_DOUBLE
} typedef csv_type;

union {
	int64_t integ;
	double doubl;
} typedef csv_num;
static_assert(sizeof (csv_num) == 8, "bad size");

bool
csv_read(char *row, uint64_t len, csv_num *nums, csv_type const *types);

#endif /* CSV_INCLUDE */
