#ifndef CSV_INCLUDE
#define CSV_INCLUDE

#ifdef __linux__
	#define _GNU_SOURCE
#endif

#include <stdio.h> /* FILE */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> /* size_t */
#include <assert.h>

typedef enum {
	CSV_ERR_OK,
	CSV_ERR_LESSCOL,
	CSV_ERR_MORECOL,
} csv_err_t;

#define FIELD_SEP (',')

/* All the fields in this struct shall not be modified by the user of the
 * library, but only by che provided functions. The contract is that the client
 * frees the resources after an error or it finished using the iterator. Also
 * the client should pay attendion to not overflow (even though improbable)
 * row_idx.
 */
typedef struct {
	FILE *fp;
	/* A copy of a line in the FILE allocated dinamically. '\0' are inserted in
	 * place of FIELD_SEP the string to divide fields, so it should not be used
	 * to read the line.
	 */
	const char *row;
	/* Points inside row at the current field terminated by '\0' that shall be
	 * read
	 */
	const char *field_start;
	/* Points inside row, at the rest of the line to read, usually it points to
	 * '\0'
	 */
	const char *field_end;
	/* Number of column in each row */
	uint64_t col_num;
	/* Index of the current column, bounded by col_num */
	uint64_t col_idx;
	/* Index of the current row */
	uint64_t row_idx;
	/* Length of row ignoring the '\0' */
	uint64_t row_len;
	/* NOTE: a usefull stat may be the number of bytes read */
} csv_t;

static_assert(sizeof (csv_t) == 64, "bad csv_t size");

/* Takes ownership of fp (so it is responsabile for closing it) */
extern void csv_init(FILE* fp, uint64_t col_num, csv_t *csv);
/* Clear all fileds to avoid missuse, return value of fclose is ignored, because
 * anyway after failure the stream is unusable. */
extern void csv_close(csv_t *csv);
/* Returns false on EOF or error, in case err is set with errno, should be
 * idempotent on failure
 */
extern bool csv_nextrow(csv_t *csv, int *err);
/* Idempotent on failure */
extern csv_err_t csv_nextfield(csv_t *csv);
extern const char *csv_strerr(csv_err_t err);

#endif /* CSV_INCLUDE */
