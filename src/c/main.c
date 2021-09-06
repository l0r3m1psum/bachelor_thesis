/* Considerare "log" asincrono per salvare lo stato del simulatore. Questo
 * potrebbe essere realizzato tramite un ring buffer nel quale il simulatore
 * produce una sequenza di stati e il logger, con i suoi thread, li consuma
 * (i.e. li salva sul disco). Qui l'hypertreading dovrebbe aiutare siccome
 * logger e simulatore dovrebbero usare parti diverse della CPU.
 *
 * Fase di inizializzazione
 *   - Leggere il csv dei parametri delle celle, validarlo, emettendo
 *     eventuali warning o errori e trasformarli in quelli necessari alla
 *     simulazione;
 *   - leggere il csv dello stato iniziale e validarlo;
 *   - leggere il csv dei parametri globali della simulazione e e validarlo;
 *   - creare eventuali thread pool;
 *   - allocare tutta la memoria necessaria al simulatore.
 *
 * Fase di esecuzione
 *   - Durante questa fase, dopo aver lanciato il simulatore, mi devo solo
 *     preoccupare di gestire il segnale per la terminazione "gentile" (i.e.
 *     il programma deve finire di scrivere l'ultimo stato per intero)
 *
 * Fase di spegnimento
 *  - Nulla di particolare è necessario in questa fase, se non liberare la
 *    memoria, ma siccome il sistema operativo lo farà comunque alla fine
 *    per noi non è realmente necassario.
 */

#include "csv.h"
#include "simulator.h"

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strerror */
#include <syslog.h>
#include <unistd.h> /* sleep */
#include <sys/stat.h>

/* Here are defined a series of tables used by X-macros with the following form:
 * type, name, csv_type, csv_num, fmt, ord. TODO: finish documentation
 */

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

/* TODO: add file and line to the log error, add to the arguments of the macro */
#define CHECK_ALL(WHICH, type, name, csv_type, csv_num, fmt, ord) \
if (CHECK_##WHICH##ord(nums[ord].csv_num)) { \
	syslog(LOG_ERR, #name " is not valid"); \
	exit(EXIT_FAILURE); \
}

/* NOTE: maybe ASSIGN_ALL can be abstracted */

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

#define INSERTER_FUNC(name) \
name(simulation_t *sim, csv_num *nums, uint64_t index, uint64_t lineno, const char *fname)

static void
INSERTER_FUNC(insert_general_params) {
	if (index != 0) {
		syslog(LOG_ERR, "too many rows in file '%s'", fname);
		exit(EXIT_FAILURE);
	}
#define CHECK(type, name, csv_type, csv_num, fmt, ord) CHECK_ALL(GENERAL_PARAMS, type, name, csv_type, csv_num, fmt, ord)
	GENERAL_PARAMS(CHECK)
#undef CHECK
	if (nums[3].integ /*s*/ > nums[2].integ /*h*/) {
		syslog(LOG_ERR, "s cannot be greater then h");
		exit(EXIT_FAILURE);
	}
#define ASSIGN_ALL(type, name, csv_type, csv_num, fmt, ord) sim->name = (type) nums[ord].csv_num;
	GENERAL_PARAMS(ASSIGN_ALL)
#undef ASSIGN_ALL
}

static void
INSERTER_FUNC(insert_cells_params) {
	if (index >= sim->Wstar * sim->Lstar) {
		syslog(LOG_ERR, "too many rows in file '%s'", fname);
		exit(EXIT_FAILURE);
	}
#define CHECK(type, name, csv_type, csv_num, fmt, ord) CHECK_ALL(CELLS_PARAMS, type, name, csv_type, csv_num, fmt, ord)
	CELLS_PARAMS(CHECK)
#undef CHECK
#define ASSIGN_ALL(type, name, csv_type, csv_num, fmt, ord) const type name = (type) nums[ord].csv_num;
	CELLS_PARAMS(ASSIGN_ALL)
#undef ASSIGN_ALL
	if (forest == 255) {
		syslog(LOG_WARNING, "forest on line %"PRIu64" has undesired value: %"PRIu16, lineno, forest);
	}
	if (urbanization == 255) {
		syslog(LOG_WARNING, "urbanization on line %"PRIu64" has undesired value: %"PRIu16, lineno, urbanization);
	}
	const uint8_t H = forest + 1; /* intentionally overflowing this unsigned integer */
	const float A = (0 <= urbanization && urbanization <= 100) ? ((float) urbanization / 100) : 0;
	const float W =
		water1 == 0   ? 0    :
		water1 == 1   ? 1    :
		water1 == 2   ? 0.75 :
		water1 == 3   ? 0.75 :
		water1 == 4   ? 0.5  :
		water1 == 253 ? 1    :
		/*water1 == 255*/ 1;
	const float S = H*(1-A)*(1-W);
	/* constructing the matrix in row-major form */
	sim->params[index] = (params_t){
		.S = S, .P = altimetry, .F = wind_speed, .D = wind_dir,
	};
	sim->gamma[index] = sim->L*1*S; /* NOTE: where 1 is alpha i.e. our patch to the model */
}

static void
INSERTER_FUNC(insert_initial_state) {
	if (index >= sim->Wstar * sim->Lstar) {
		syslog(LOG_ERR, "too many rows in file '%s'", fname);
		exit(EXIT_FAILURE);
	}
#define CHECK(type, name, csv_type, csv_num, fmt, ord) CHECK_ALL(INITIAL_STATE, type, name, csv_type, csv_num, fmt, ord)
	INITIAL_STATE(CHECK)
#undef CHECK
#define ASSIGN_ALL(type, name, csv_type, csv_num, fmt, ord) const type name = (type) nums[ord].csv_num;
	INITIAL_STATE(ASSIGN_ALL)
#undef ASSIGN_ALL
	sim->old_state[index] = (state_t){.N = N, .B = B};
}

/* NOTE: maybe a should pass a file pointer directly for testing purposes */
static inline void
read_data(const char *fname, simulation_t *sim, csv_num *nums, uint64_t len, const csv_type *types,
	void (*insert_data)(simulation_t *, csv_num *, uint64_t index, uint64_t lineno, const char *fname)) {
	FILE *fp = fopen(fname, "r");
	if (!fp) {
		syslog(LOG_ERR, "unable to open '%s': %s", fname, strerror(errno));
		exit(EXIT_FAILURE);
	}
	char *row = NULL;
	size_t linecap = 0;
	ssize_t linelen = 0;
	errno = 0;
	syslog(LOG_INFO, "reading file: '%s'", fname);
	for (uint64_t lineno = 1, index = 0;
		(linelen = getline(&row, &linecap, fp)) != -1;
		lineno++) {
		/* debug code for skipping coments */
		if (row[0] == '#') {
			continue;
		}
		if (!csv_read(row, len, nums, types)) {
			syslog(LOG_ERR, "unable ro read cells parameters from file '%s' at line %"PRIu64, fname, lineno);
			exit(EXIT_FAILURE);
		}
		insert_data(sim, nums, index, lineno, fname);
		index++;
	}
	if (errno) {
		free(row);
		syslog(LOG_ERR, "error while getting line from '%s': %s", fname, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (fclose(fp) != 0) {
		syslog(LOG_WARNING, "unable to close file '%s': %s", fname, strerror(errno));
	}
	free(row);
}

static bool
dump(simulation_t *s) {
	/* TODO: log additiona information like the file where it is beeing dumped. */
	syslog(LOG_INFO, "starting to dump the state of the simulation");
	for (uint64_t i = 0; i < s->Lstar; i++) {
		for (uint64_t j = 0; j < s->Wstar-1; j++) {
			const uint64_t ij = i + j*s->Wstar;
			printf("%d,", s->new_state[ij].N);
		}
		printf("%d\n", s->new_state[i + (s->Wstar-1)*s->Wstar].N);
	}
	putchar('\n');
	sleep(2);
	syslog(LOG_INFO, "finished to dump the state of the simulation");
	return true;
}

int
main(const int argc, const char *argv[]) {
	openlog(argv[0], LOG_PERROR, 0);

	if (argc != 5) {
		syslog(LOG_ERR, "wrong number of arguments, expected 4, received %d", argc-1);
		return EXIT_FAILURE;
	}

	simulation_t sim2 = {0};

	{
		const char * const restrict general_params = argv[1];
		const char * const restrict cells_params = argv[2];
		const char * const restrict initial_state = argv[3];
		const char * const restrict out_dir = argv[4];

		{
			csv_num nums[GENERAL_PARAMS_NO] = {0};
			read_data(general_params, &sim2, nums, GENERAL_PARAMS_NO, general_params_types, insert_general_params);
		}

		const uint64_t area = sim2.Wstar * sim2.Lstar;
		sim2.old_state = malloc(sizeof (state_t) * area);
		sim2.new_state = malloc(sizeof (state_t) * area);
		sim2.params = malloc(sizeof (params_t) * area);
		sim2.gamma = malloc(sizeof (float) * area);

		if (!(sim2.old_state && sim2.new_state && sim2.params && sim2.gamma)) {
			const uint64_t total = (sizeof (state_t) * 2 + sizeof (params_t) + sizeof (float)) * area;
			syslog(LOG_ERR, "unable to allocate %"PRIu64" bytes of memory: %s", total, strerror(errno));
			return EXIT_FAILURE;
		}

		{
			csv_num nums[CELLS_PARAMS_NO] = {0};
			read_data(cells_params, &sim2, nums, CELLS_PARAMS_NO, cells_params_types, insert_cells_params);
		}

		{
			csv_num nums[INITIAL_STATE_NO] = {0};
			read_data(initial_state, &sim2, nums, INITIAL_STATE_NO, initial_state_types, insert_initial_state);
		}

		struct stat buf = {0};
		if(stat(out_dir, &buf) == -1) {
			syslog(LOG_ERR, "unable to use stat on '%s': %s", out_dir, strerror(errno));
			return EXIT_FAILURE;
		}
		if (!S_ISDIR(buf.st_mode)) {
			syslog(LOG_ERR, "'%s' is not a directory", out_dir);
			return EXIT_FAILURE;
		}
		/* TODO: test that I can write in out_dir */
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
			syslog(LOG_WARNING, "unable to set %s handler: %s", sys_siglist[SIGINT], strerror(errno));
		}
	}

	simulation_run(&sim2, dump);

	free(sim2.old_state);
	free(sim2.new_state);
	free(sim2.params);
	free(sim2.gamma);

	closelog();

	return EXIT_SUCCESS;
}
