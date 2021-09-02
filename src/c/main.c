#include "csv.h"
#include <stdalign.h>
#include <string.h> /* strerror */
#include <inttypes.h>

int
main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "error: missing file name argument\n");
		return 1;
	}
	FILE *fp = fopen(argv[1], "r");
	if (!fp) {
		perror("fopen");
		return 1;
	}
	alignas(64) csv_t csv = {0};
	uint64_t col_num = 8;
	csv_init(fp, col_num, &csv);

	csv_t *it = &csv;
	int err = 0;
	int i = 0;
	while (csv_nextrow(it, &err) && i++ < 5) {
		for (size_t i = 0; i < col_num; i++) {
			csv_err_t csv_err = csv_nextfield(it);
			if (csv_err != CSV_ERR_OK) {
				fprintf(stderr, "csv_nextfield: %s\n", csv_strerr(csv_err));
				csv_close(it);
				return 1;
			}

			printf("printing row %" PRIu64 " in col %" PRIu64 ": \"%s\"\n",
				it->row_idx, it->col_idx, it->field_start);
		}
	}
	if (err != 0) {
		fprintf(stderr, "csv_nextrow: %s\n", strerror(err));
		csv_close(it);
		return 1;
	}
	puts("finished successfully!");
	csv_close(it);
}