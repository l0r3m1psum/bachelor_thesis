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
#include <stdalign.h>
#include <string.h> /* strerror */
#include <inttypes.h> /* PRI macros */
#include <signal.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h> /* sleep */

static int
old_main(int argc, char *argv[]) {
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
	return 0;
}

static bool
dump(simulation_t *s) {
	for (uint64_t i = 0; i < s->Lstar; i++) {
		for (uint64_t j = 0; j < s->Wstar-1; j++) {
			const uint64_t ij = i + j*s->Wstar;
			printf("%d,", s->new_state[ij].N);
		}
		printf("%d\n", s->new_state[i + (s->Wstar-1)*s->Wstar].N);
	}
	putchar('\n');
	sleep(2);
	return true;
}

int
main(int argc, char *argv[]) {
	openlog(argv[0], LOG_PERROR, 0);

	{
		syslog(LOG_INFO, "setting up interuption hanling");
		sigset_t set = {0};
		struct sigaction action = {
			.sa_handler = simulation_SIGINT_handler,
			.sa_mask = (sigset_t){0},
			.sa_flags = SA_RESTART
		};
		if (sigfillset(&set) == -1) {
			syslog(LOG_WARNING, "unable to fill sigset");
		}
		if (sigdelset(&set, SIGINT) == -1) {
			syslog(LOG_WARNING, "unable to delete SIGINT from sigset");
		}
		if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
			syslog(LOG_WARNING, "unable to set the sigprocmask");
		}
		if (sigaction(SIGINT, &action, NULL) == -1) {
			syslog(LOG_WARNING, "unable to set SIGINT handler");
		}
	}

	const uint64_t width = 10, height = 10, area = width * height;
	simulation_t s = {
		.Wstar = width,
		.Lstar = height,
		.h = 30,
		.s = 1,
		.seed = 123,
		.tau = 1,
		.theta = 0.4f,
		.k0 = 1,
		.k1 = 1,
		.k2 = 1,
		.L = 1,
		.old_state = malloc(sizeof (state_t) * area),
		.new_state = malloc(sizeof (state_t) * area),
		.params = malloc(sizeof (params_t) * area),
		.gamma = malloc(sizeof (float) * area)
	};

	if (!(s.old_state && s.new_state && s.params && s.gamma)) {
		return EXIT_FAILURE;
	}

	for (uint64_t i = 0; i < area; i++) {
		s.old_state[i] = (state_t){.B = 0.5f, .N = false};
		s.new_state[i] = (state_t){0};
		s.params[i] = (params_t){.P = 1, .S = 0.7f, .F = 1, .D = pi};
		s.gamma[i] = 10;
	}

	for (uint64_t i = 0; i < s.Lstar; i++) {
		for (uint64_t j = 0; j < s.Wstar-1; j++) {
			if (3 <= i && i <= 5 && 3 <= j && j <= 5) {
				uint64_t ij = i + j*s.Wstar;
				s.old_state[ij] = (state_t){ .B = 10, .N = true };
			}
		}
	}

	simulation_run(&s, dump);

	free(s.old_state);
	free(s.new_state);
	free(s.params);
	free(s.gamma);

	closelog();

	return EXIT_SUCCESS;
}
