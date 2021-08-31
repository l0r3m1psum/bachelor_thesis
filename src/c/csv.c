#include "csv.h"
#include <errno.h>
#include <string.h> /* strchr */
#include <stdlib.h> /* free */

static void
clear(csv_t *csv) {
	assert(csv);
	csv->fp = NULL;
	csv->row = csv->field_start = csv->field_end = NULL;
	csv->col_num = csv->col_idx = csv->row_idx = csv->row_len = 0;
}

static bool
is_valid(csv_t *csv) {
	if (!csv) {
		return false;
	}
	bool ok = csv->fp
		&& csv->field_start <= csv->field_end
		&& csv->field_start >= csv->row
		&& csv->col_idx <= csv->col_num;
	return ok;
}

void
csv_init(FILE *fp, uint64_t col_num, csv_t *csv) {
	assert(fp && csv && col_num);

	clear(csv);
	csv->fp = fp;
	csv->col_num = col_num;
}

void
csv_close(csv_t *csv) {
	assert(is_valid(csv));

	(void) fclose(csv->fp);
	free((void *) csv->row);
	clear(csv);
}

bool
csv_nextrow(csv_t *csv, int *err) {
	assert(is_valid(csv));

	errno = 0;
	size_t linecap = 0;
	ssize_t len = getline((char **) &csv->row, &linecap, csv->fp);
	if (len == -1) {
		*err = errno;
		return false;
	}

	/* To avoid problems with is_valid */
	csv->field_start = csv->field_end = csv->row;
	csv->row_idx++;
	csv->col_idx = 0;
	csv->row_len = (uint64_t) len;
	return true;
}

/* NOTE: The sepataror should be a single character or an entire string? In the
 * later case strstr is the function to use.
 */
csv_err_t
csv_nextfield(csv_t *csv) {
	assert(is_valid(csv));

	if (csv->col_num == 1) {
		csv->field_start = csv->row;
		csv->field_end = csv->row + csv->row_len;
		assert(*csv->field_end == '\0');
		csv->col_idx++;
		assert(csv->col_idx == csv->col_num);
		return CSV_ERR_OK;
	}

	if (csv->col_idx == 0) {
		char *p = strchr(csv->row, FIELD_SEP);
		if (!p) {
			return CSV_ERR_LESSCOL;
		}
		csv->field_start = csv->row;
		csv->field_end = p;
		*p = '\0';
		csv->col_idx++;
		return CSV_ERR_OK;
	} else if (csv->col_idx < csv->col_num - 1) {
		char *p = strchr(csv->field_end + 1, FIELD_SEP);
		if (!p) {
			return CSV_ERR_LESSCOL;
		}
		csv->field_start = csv->field_end + 1;
		csv->field_end = p;
		*p = '\0';
		csv->col_idx++;
		return CSV_ERR_OK;
	} else /* is the last field */ {
		char *p = strchr(csv->field_end+1, FIELD_SEP);
		if (p) {
			return CSV_ERR_MORECOL;
		}
		csv->field_start = csv->field_end + 1;
		csv-> field_end = csv->row + csv->row_len;
		assert(*csv->field_end == '\0');
		csv->col_idx++;
		assert(csv->col_idx == csv->col_num);
		return CSV_ERR_OK;
	}
}

const char *
csv_strerr(csv_err_t err) {
	switch (err) {
		case CSV_ERR_OK:
			return "No error";
		case CSV_ERR_LESSCOL:
			return "The current row has lass column than it should";
		case CSV_ERR_MORECOL:
			return "The current row has more column than it should";
		default:
			return "Bad error code";
	}
}

#ifdef TEST
#include <stdalign.h>
#include <inttypes.h>

static void
print_csv(csv_t *it, uint64_t col_num) {
	int err = 0;
	while (csv_nextrow(it, &err)) {
		for (size_t i = 0; i < col_num; i++) {
			csv_err_t csv_err = csv_nextfield(it);
			if (csv_err != CSV_ERR_OK) {
				fprintf(stderr, "csv_nextfield: %s\n", csv_strerr(csv_err));
				csv_close(it);
				exit(EXIT_FAILURE);
			}

			printf("printing row %" PRIu64 " in col %" PRIu64 ": \"%s\"\n",
				it->row_idx, it->col_idx, it->field_start);
		}
	}
	if (err != 0) {
		fprintf(stderr, "csv_nextrow: %s\n", strerror(err));
		csv_close(it);
		exit(EXIT_FAILURE);
	}
	puts("finished successfully!");
	csv_close(it);
}

int
main(void) {
	alignas(64) csv_t csv = {0};
	{
		char test[] =
			"1,2,3,4,5.1,6\n"
			"1232  ,  456,   34  ,12.34,89,1\n" /* spaced records */
			",,,,,\n"; /* empty records */

		FILE *fp = fmemopen(test, sizeof test - 1, "r");
		if (!fp) {
			perror("fmemopen");
			return 1;
		}
		uint64_t col_num = 6;
		csv_init(fp, col_num, &csv);
		print_csv(&csv, col_num);
	}
	{
		char test[] = "1\n2\n3"; /* no new line at the end */
		uint64_t col_num = 1;
		FILE *fp = fmemopen(test, sizeof test - 1, "r");
		if (!fp) {
			perror("fmemopen");
			return 1;
		}
		csv_init(fp, col_num, &csv);
		print_csv(&csv, col_num);
	}
	{
		char test[] = "1,3 4 5\n2,\n3,  0\r\n"; /* windows newline */
		uint64_t col_num = 2;
		FILE *fp = fmemopen(test, sizeof test - 1, "r");
		if (!fp) {
			perror("fmemopen");
			return 1;
		}
		csv_init(fp, col_num, &csv);
		print_csv(&csv, col_num);
	}
}

#endif
