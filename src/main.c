#include "csv.h"
#include <stdalign.h>
#include <string.h> /* strerror */

int
main(void) {
	FILE *fp = fopen("../res/turano.csv", "r");
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

			printf("printing row %llu in col %llu: \"%s\"\n",
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
