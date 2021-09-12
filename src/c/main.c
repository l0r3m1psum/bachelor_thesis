#define _POSIX_C_SOURCE 200909L
#ifdef __APPLE__
	#define _DARWIN_C_SOURCE /* O_DIRECTORY */
#endif

#include "csv.h"
#include "simulator.h"

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strerror, strsignal */
#include <syslog.h>
#include <unistd.h> /* close */
#include <fcntl.h> /* open, openat */

#define GENERAL_PARAMS(X) \
X(uint64_t, Wstar, CSV_INT64,  integ, PRIu64, 0) \
X(uint64_t, Lstar, CSV_INT64,  integ, PRIu64, 1) \
X(uint64_t, h,     CSV_INT64,  integ, PRIu64, 2) \
X(uint64_t, s,     CSV_INT64,  integ, PRIu64, 3) \
X(uint32_t, seed,  CSV_INT64,  integ, PRIu64, 4) \
X(float,    tau,   CSV_DOUBLE, doubl, .2lf,   5) \
X(float,    theta, CSV_DOUBLE, doubl, .2lf,   6) \
X(float,    k0,    CSV_DOUBLE, doubl, .2lf,   7) \
X(float,    k1,    CSV_DOUBLE, doubl, .2lf,   8) \
X(float,    k2,    CSV_DOUBLE, doubl, .2lf,   9) \
X(float,    L,     CSV_DOUBLE, doubl, .2lf,   10)

#define CHECK_GENERAL_PARAMS0(x) ((x) < 3)
#define CHECK_GENERAL_PARAMS1(x) ((x) < 3)
#define CHECK_GENERAL_PARAMS2(x) ((x) < 0)
#define CHECK_GENERAL_PARAMS3(x) ((x) < 0)
#define CHECK_GENERAL_PARAMS4(x) ((x) < 0)
#define CHECK_GENERAL_PARAMS5(x) ((x) < 0)
#define CHECK_GENERAL_PARAMS6(x) ((x) < 0 || (x) > 1)
#define CHECK_GENERAL_PARAMS7(x) (false)
#define CHECK_GENERAL_PARAMS8(x) (false)
#define CHECK_GENERAL_PARAMS9(x) (false)
#define CHECK_GENERAL_PARAMS10(x) ((x) < 0)

#define CELLS_PARAMS(X) \
X(uint16_t, altimetry,    CSV_INT64,  integ, PRIu16, 0) \
X(uint8_t,  forest,       CSV_INT64,  integ, PRIu8,  1) \
X(uint8_t,  urbanization, CSV_INT64,  integ, PRIu8,  2) \
X(uint8_t,  water1,       CSV_INT64,  integ, PRIu8,  3) \
X(bool,     water2,       CSV_INT64,  integ, d,      4) \
X(uint8_t,  naturemap,    CSV_INT64,  integ, PRIu8,  5) \
X(float,    wind_dir,     CSV_DOUBLE, doubl, .2lf,   6) \
X(float,    wind_speed,   CSV_DOUBLE, doubl, .2lf,   7)
/* NOTE: gamma is not here because it is calculated */

#define CHECK_CELLS_PARAMS0(x) ((x) < 0 || (x) > 4380)
#define CHECK_CELLS_PARAMS1(x) ((x) < 0 ||(x) > 255)
#define CHECK_CELLS_PARAMS2(x) ((x) < 0 || ((x) > 100 && (x) != 255))
#define CHECK_CELLS_PARAMS3(x) ((x) < 0 || ((x) > 4 && (x) != 253 && (x) != 255))
#define CHECK_CELLS_PARAMS4(x) ((x) != 0 && (x) != 1)
#define CHECK_CELLS_PARAMS5(x) ((x) < 0 || (x) > 90)
#define CHECK_CELLS_PARAMS6(x) (false)
#define CHECK_CELLS_PARAMS7(x) ((x) < 0)

#define INITIAL_STATE(X) \
X(float, B, CSV_DOUBLE, doubl, .2lf, 0) \
X(bool, N, CSV_INT64, integ, PRIu64, 1)

#define CHECK_INITIAL_STATE0(x) ((x) < 0)
#define CHECK_INITIAL_STATE1(x) ((x) != 0 && (x) != 1)

#define CHECK_ALL(WHICH, fname, lineno, type, name, csv_type, csv_num, fmt, ord) \
if (CHECK_##WHICH##ord(nums[ord].csv_num)) { \
	syslog(LOG_ERR, "invalid "#name" value in file '%s' at line %"PRIu64, fname, lineno); \
	return false; \
}

#define GENERAL_PARAMS_NO 11
#define CELLS_PARAMS_NO 8
#define INITIAL_STATE_NO 2

#define GET_CSV_TYPE(type, name, csv_type, csv_num, fmt, ord) csv_type,
	static const csv_type general_params_types[GENERAL_PARAMS_NO] = {
		GENERAL_PARAMS(GET_CSV_TYPE)
	};
	static const csv_type cells_params_types[CELLS_PARAMS_NO] = {
		CELLS_PARAMS(GET_CSV_TYPE)
	};
	static const csv_type initial_state_types[INITIAL_STATE_NO] = {
		CSV_DOUBLE, CSV_INT64,
	};
#undef GET_CSV_TYPE

/* Insert the nums record in the sim struct, index is used to chose where to put
 * the record in sim, lineno and fname are for error reporting. Before insertion
 * check are made to validate the index and the nums. The length of nums in
 * assumed to be known by the function. Also lineno should start at 1.
 * @return true if insertion went ok else false
 */
#define INSERTER_FUNC(name) static bool \
name(simulation_t *sim, const csv_num *nums, uint64_t index, uint64_t lineno, const char *fname)

#define INSERTER_ASSERT assert(sim); assert(nums); assert(fname); assert(lineno);

INSERTER_FUNC(insert_general_params) {
	INSERTER_ASSERT
	if (index != 0) {
		syslog(LOG_ERR, "too many rows in file '%s'", fname);
		return false;
	}
#define CHECK(type, name, csv_type, csv_num, fmt, ord) CHECK_ALL(GENERAL_PARAMS, fname, lineno, type, name, csv_type, csv_num, fmt, ord)
	GENERAL_PARAMS(CHECK)
#undef CHECK
	if (nums[3].integ /*s*/ > nums[2].integ /*h*/) {
		syslog(LOG_ERR, "s cannot be greater then h");
		return false;
	}
#define ASSIGN_ALL(type, name, csv_type, csv_num, fmt, ord) sim->name = (type) nums[ord].csv_num;
	GENERAL_PARAMS(ASSIGN_ALL)
#undef ASSIGN_ALL
	return true;
}

INSERTER_FUNC(insert_cells_params) {
	INSERTER_ASSERT
	if (index >= sim->Wstar * sim->Lstar) {
		syslog(LOG_ERR, "too many rows in file '%s'", fname);
		return false;
	}
#define CHECK(type, name, csv_type, csv_num, fmt, ord) CHECK_ALL(CELLS_PARAMS, fname, lineno, type, name, csv_type, csv_num, fmt, ord)
	CELLS_PARAMS(CHECK)
#undef CHECK
#define ASSIGN_ALL(type, name, csv_type, csv_num, fmt, ord) const type name = (type) nums[ord].csv_num;
	CELLS_PARAMS(ASSIGN_ALL)
#undef ASSIGN_ALL
	(void) naturemap;
	(void) water2;
	if (forest == 255) {
		syslog(LOG_WARNING, "forest on line %"PRIu64" has undesired value: %"
			PRIu16, lineno, forest);
	}
	if (urbanization == 255) {
		syslog(LOG_WARNING, "urbanization on line %"PRIu64" has undesired "
			"value: %"PRIu16, lineno, urbanization);
	}
	const uint8_t H = forest + 1; /* intentionally overflowing this unsigned integer */
	const float A = (urbanization <= 100)
		? ((float) urbanization / 100) : 0;
	const float W =
		water1 == 0   ? 0    :
		water1 == 1   ? 1    :
		water1 == 2   ? 0.75 :
		water1 == 3   ? 0.75 :
		water1 == 4   ? 0.5  :
		water1 == 253 ? 1    :
		/*water1 == 255*/ 1;
	const float S = H*(1-A)*(1-W);
	assert(S >= 0 && S <= 3);
	/* constructing the matrix in row-major form */
	sim->params[index] = (params_t){
		.S = S, .P = altimetry, .F = wind_speed, .D = wind_dir,
		.gamma = sim->L*1*S, /* NOTE: where 1 is alpha i.e. our patch to the model */
	};
	return true;
}

INSERTER_FUNC(insert_initial_state) {
	INSERTER_ASSERT
	if (index >= sim->Wstar * sim->Lstar) {
		syslog(LOG_ERR, "too many rows in file '%s'", fname);
		return false;
	}
#define CHECK(type, name, csv_type, csv_num, fmt, ord) CHECK_ALL(INITIAL_STATE, fname, lineno, type, name, csv_type, csv_num, fmt, ord)
	INITIAL_STATE(CHECK)
#undef CHECK
#define ASSIGN_ALL(type, name, csv_type, csv_num, fmt, ord) const type name = (type) nums[ord].csv_num;
	INITIAL_STATE(ASSIGN_ALL)
#undef ASSIGN_ALL
	const float gamma = sim->params[index].gamma;
	if (nums[0].doubl /*N*/ > gamma) {
		syslog(LOG_WARNING, "B is greater then %f in file '%s' on line %"PRIu64
			" using %f instead",
			gamma, fname, lineno, gamma);
		sim->old_state[index] = (state_t){.N = N, .B = gamma};
	} else {
		sim->old_state[index] = (state_t){.N = N, .B = B};
	}
	return true;
}

/* NOTE: maybe I should pass a FILE pointer directly for testing purposes */
/* @param fname null terminated string
 * @param buf pointer to ponter to a dinamically allocated buffer (not owned)
 * @param linecap pointer to the size of the buffer pointed by buf;
 * @param sim simulation data
 * @param nums array to store temporarely converted numbers
 * @param len length of nums and types
 * @params types instructions to read numbers
 * @params insert_data callback to insert data in sim return false is something goes wrong
 * @return the number of inserted records
 */
static inline uint64_t
read_data(const char *fname, char **buf, size_t *linecap, simulation_t *sim, csv_num *nums, uint64_t len, const csv_type *types,
	bool (*insert_data)(simulation_t *, const csv_num *, uint64_t index, uint64_t lineno, const char *fname)) {
	assert(fname && sim && nums && len && types && insert_data);
	FILE *fp = fopen(fname, "r");
	if (!fp) {
		syslog(LOG_ERR, "unable to open '%s': %s", fname, strerror(errno));
		return 0; /* 0 is always an error because it doesn't make sense to read 0 records */
	}
	ssize_t linelen = 0;
	errno = 0;
	uint64_t index = 0;
	syslog(LOG_INFO, "reading file: '%s'", fname);
	for (uint64_t lineno = 1;
		(linelen = getline(buf, linecap, fp)) != -1;
		lineno++) {
		/* debug code for skipping coments */
		if (*buf[0] == '#') {
			continue;
		}
		if (!csv_read(*buf, len, nums, types)) {
			free(*buf);
			syslog(LOG_ERR, "unable ro read cells parameters from file '%s' at "
				"line %"PRIu64, fname, lineno);
			return index;
		}
		if (!insert_data(sim, nums, index, lineno, fname)) {
			syslog(LOG_ERR, "unable to insert_data data from file '%s' on line "
				"%"PRIu64, fname, lineno);
			return index;
		}
		index++;
	}
	if (errno) {
		free(*buf);
		syslog(LOG_ERR, "error while getting line from '%s': %s", fname,
			strerror(errno));
		return index;
	}
	if (fclose(fp) != 0) {
		syslog(LOG_WARNING, "unable to close file '%s': %s", fname,
			strerror(errno));
	}
	return index;
}

static int out_dir_fd;

/* NOTE: in this function UBsan can detect a problem, but it's just a conflict
 * with Asan.
 */
static bool
dump(simulation_t *s) {
	static uint64_t counter = 0;
	/* 20 is the maximum number of digits in a uint64_t so a 128 char buffer is
	 * more than enough
	 */
	const uint64_t size = 1 << 7;
	char fnamebuf[size];
	(void) snprintf(fnamebuf, size, "result%03"PRIu64, counter);
	counter++;
	const int fd = openat(out_dir_fd, fnamebuf, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) {
		syslog(LOG_WARNING, "unable to open file '%s': %s", fnamebuf,
			strerror(errno));
		return false;
	}
	FILE *fp = fdopen(fd, "w");
	if (!fp) {
		syslog(LOG_WARNING, "unable to convert file descriptor in stream: %s",
			strerror(errno));
		if (close(fd) == -1) {
			syslog(LOG_WARNING, "unable to close file descriptor: %s",
				strerror(errno));
		}
		return false;
	}
	if (setvbuf(fp, NULL, _IOFBF, 0) == EOF) {
		syslog(LOG_WARNING, "cannot put set stream for file '%s' to fully buffered", fnamebuf);
	}
	const uint64_t area = s->Lstar*s->Wstar;
	const uint64_t div = 10;
	const uint64_t div_area = area/div;
	for (uint64_t i = 0; i < div_area; i += div) {
		assert(s->new_state[i].B >= 0);
		(void) fprintf(fp,
			"%f,%d\n%f,%d\n%f,%d\n%f,%d\n%f,%d\n%f,%d\n%f,%d\n%f,%d\n%f,%d\n%f,%d\n",
			s->new_state[i+0].B, s->new_state[i+0].N,
			s->new_state[i+1].B, s->new_state[i+1].N,
			s->new_state[i+2].B, s->new_state[i+2].N,
			s->new_state[i+3].B, s->new_state[i+3].N,
			s->new_state[i+4].B, s->new_state[i+4].N,
			s->new_state[i+5].B, s->new_state[i+5].N,
			s->new_state[i+6].B, s->new_state[i+6].N,
			s->new_state[i+7].B, s->new_state[i+7].N,
			s->new_state[i+8].B, s->new_state[i+8].N,
			s->new_state[i+9].B, s->new_state[i+9].N);
	}
	for (uint64_t i = div_area*div; i < area; i++) {
		assert(area % div != 0);
		(void) fprintf(fp, "%f,%d\n", s->new_state[i].B, s->new_state[i].N);
	}
	fclose(fp);
	return true;
}

static inline void
free_all_sim(simulation_t *s) {
	free(s->old_state);
	free(s->new_state);
	free(s->params);
}

int
main(const int argc, const char *argv[]) {
	openlog(argv[0], LOG_PERROR, 0);

	if (argc != 5) {
		syslog(LOG_ERR, "wrong number of arguments, expected 4, received %d",
			argc-1);
		return EXIT_FAILURE;
	}

	simulation_t sim = {0};

	{
		const char * const restrict general_params = argv[1];
		const char * const restrict cells_params = argv[2];
		const char * const restrict initial_state = argv[3];
		const char * const restrict out_dir = argv[4];
		char *row = NULL;
		size_t linecap = 0;

		{
			csv_num nums[GENERAL_PARAMS_NO] = {0};
			if (read_data(general_params, &row, &linecap, &sim, nums, GENERAL_PARAMS_NO,
				general_params_types, insert_general_params) != 1) {
				syslog(LOG_ERR, "unable to read the general parameters from "
					"file '%s'", general_params);
				free(row);
				return EXIT_FAILURE;
			}
		}

		const uint64_t area = sim.Wstar * sim.Lstar;
		sim.old_state = malloc(sizeof (state_t) * area);
		sim.new_state = calloc(sizeof (state_t) * area, 1); /* to avoid weired values on boundaries */
		sim.params = malloc(sizeof (params_t) * area);

		if (!(sim.old_state && sim.new_state && sim.params)) {
			const uint64_t total = (sizeof (state_t) * 2 + sizeof (params_t))
				* area;
			syslog(LOG_ERR, "unable to allocate %"PRIu64" bytes of memory: %s",
				total, strerror(errno));
			free_all_sim(&sim);
			free(row);
			return EXIT_FAILURE;
		}

		{
			csv_num nums[CELLS_PARAMS_NO] = {0};
			uint64_t res = 0;
			if ((res = read_data(cells_params, &row, &linecap, &sim, nums, CELLS_PARAMS_NO,
				cells_params_types, insert_cells_params)) != area) {
				syslog(LOG_ERR, "not enough records in file '%s' only %"PRIu64,
					cells_params, res);
				free_all_sim(&sim);
				free(row);
				return EXIT_FAILURE;
			}
		}

		{
			csv_num nums[INITIAL_STATE_NO] = {0};
			uint64_t res = 0;
			if ((res = read_data(initial_state, &row, &linecap, &sim, nums, INITIAL_STATE_NO,
				initial_state_types, insert_initial_state)) != area) {
				syslog(LOG_ERR, "not enough records in file '%s' only %"PRIu64,
					cells_params, res);
				free_all_sim(&sim);
				free(row);
				return EXIT_FAILURE;
			}
		}

		free(row);

		/* TODO: test that I can write in out_dir */
		out_dir_fd = open(out_dir, O_RDONLY|O_DIRECTORY);
		if (out_dir_fd == -1) {
			syslog(LOG_ERR, "cannot open directory '%s': %s", out_dir,
				strerror(errno));
			free_all_sim(&sim);
			return EXIT_FAILURE;
		}
	}

	{
		syslog(LOG_INFO, "setting up interuption hanling");
		sigset_t set = {0};
		if (sigfillset(&set) == -1) {
			syslog(LOG_WARNING, "unable to fill sigset: %s", strerror(errno));
		}
		struct sigaction action = {
			.sa_handler = simulation_SIGINT_handler,
			.sa_mask = set,
			.sa_flags = SA_RESTART
		};
		if (sigaction(SIGINT, &action, NULL) == -1) {
			syslog(LOG_WARNING, "unable to set %s handler: %s",
				strsignal(SIGINT), strerror(errno));
		}
	}

	simulation_run(&sim, dump);

	free_all_sim(&sim);

	closelog();

	return EXIT_SUCCESS;
}
