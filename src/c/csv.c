#include "csv.h"
#include <inttypes.h>
#include <syslog.h>
#include <stdlib.h> /* strtoll, strtod */
#include <string.h> /* strchr */

static const char field_sep = ',';

static inline bool
try_insert_num(const char *str, uint64_t where, csv_num *nums, const csv_type *types) {
	char *endptr = NULL;
	if (types[where] == CSV_INT64) {
		nums[where].integ = strtoll(str, &endptr, 10);
	} else /* types[where] == CSV_DOUBLE */ {
		nums[where].doubl = strtod(str, &endptr);
	}
	if (str == endptr) {
		syslog(LOG_ERR, "unable to parse number '%s'", str);
		return false;
	}
	return true;
}

bool
csv_read(char *row, uint64_t len, csv_num *nums, const csv_type *types) {
	assert(len && nums && types);

	for (uint64_t i = 0; i < len-1; i++) {
		char *sep = strchr(row, field_sep);
		if (!sep) {
			syslog(LOG_ERR, "wrong number of rows, expected %"PRIu64", found %"PRIu64, len, i);
			return false;
		}
		*sep = '\0';
		if (!try_insert_num(row, i, nums, types)) {
			return false;
		}
		row = sep+1;
	}

	char *shall_not_be_present = strchr(row, field_sep);
	if (shall_not_be_present) {
		syslog(LOG_ERR, "there are more rows than necessary");
		return false;
	}
	if (!try_insert_num(row, len-1, nums, types)) {
		return false;
	}

	return true;
}
