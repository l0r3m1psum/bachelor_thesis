#include "csv.h"
#include <inttypes.h>
#include <syslog.h>
#include <stdlib.h> /* strtoll, strtod */
#include <string.h> /* strchr */

static const char field_sep = ',';

/* @param str null terminated string
 * @param num where the parsed number will be stored
 * @param type how str shall be read
 * @return true on successfull reading of the number else false
 */
static inline bool
try_insert_num(const char *str, csv_num *num, csv_type type) {
	assert(str && num);
	char *endptr = NULL;
	if (type == CSV_INT64) {
		num->integ = strtoll(str, &endptr, 10);
	} else /* type == CSV_DOUBLE */ {
		num->doubl = strtod(str, &endptr);
	}
	if (str == endptr) {
		syslog(LOG_ERR, "unable to parse number '%s'", str);
		return false;
	}
	return true;
}

/* @param row null terminated string
 * @param len length of nums and types
 * @param nums where the numbers read will be stored
 * @param types instructions on how to read the numbers in row
 * @return true on successfull reading of all the numbers else false
 */
bool
csv_read(char *row, uint64_t len, csv_num *nums, const csv_type *types) {
	assert(row && len && nums && types);

	for (uint64_t i = 0; i < len-1; i++) {
		char *sep = strchr(row, field_sep);
		if (!sep) {
			syslog(LOG_ERR, "wrong number of rows, expected %"PRIu64", found %"PRIu64, len, i);
			return false;
		}
		*sep = '\0';
		if (!try_insert_num(row, nums+i, types[i])) {
			return false;
		}
		row = sep+1;
	}

	char *shall_not_be_present = strchr(row, field_sep);
	if (shall_not_be_present) {
		syslog(LOG_ERR, "there are more rows than necessary");
		return false;
	}
	const uint64_t last = len-1;
	if (!try_insert_num(row, nums+last, types[last])) {
		return false;
	}

	return true;
}
