#include "simulator.h"

#include <math.h>
#include <syslog.h>
#include <inttypes.h>

/* NOTE: the order of lookup matters for caching reason */
static const int8_t Gamma[8][2] = {
	{-1, 1},  {0, 1},  {1, 1},
	{-1, 0},           {1, 0},
	{-1, -1}, {0, -1}, {1, -1},
}; /* All offsets around a cell */
static_assert(sizeof Gamma == 16, "bad size");

static bool should_stop_early = false;

void
simulation_SIGINT_handler(int sig) {
	(void) sig;
	should_stop_early = true;
}

inline static float
maxf(float a, float b) {
	const float res = a > b ? a : b;
	return res;
}

/* Generates a normally distributed random number in [-1,1] using first a linear
 * cogruential generator and then the Box–Muller transform. The constants m and
 * a are taken from the errata of "Tables of Linear Congruential Generators of
 * Different Sizes and Good Lattice Structure", Mathematics of Computation, 68,
 * 225 (1999), 249–260.
 *
 * @param xn previous seed
 */
inline static float
rngf(uint32_t *xn) {
	assert(xn);
	const uint32_t m = 1u << 31, a = 37769685, c = 12345, max = m - 1;
	*xn = (a * (*xn) + c) % m;
	const float u1 = (float) (*xn)/(float) max;
	*xn = (a * (*xn) + c) % m;
	const float u2 = (float) (*xn)/(float) max;
	const float res = sqrtf(-2.0f*logf(u1))*cosf(2*pi*u2)/2.5f;
	return res;
}

/* Converts a coordinate (i, j) in a one dimensional index
 */
uint64_t
sim_index(uint64_t i, uint64_t j, simulation_t *s) {
	assert(s);
	const uint64_t res = i + j*s->Wstar;
	return res;
}

/* NOTE: Ottimizzazione: tutti i parametri che non dipendono da old_state o da
 * funzioni casuali possono essere precalcolate. Quindi d, fw e fP (a meno del
 * parametro di perturbazione) possono essere precalcolate.
 *
 * NOTE: Ottimizzatione: Considerare "log" asincrono per salvare lo stato del
 * simulatore. Questo potrebbe essere realizzato tramite un ring buffer nel
 * quale il simulatore produce una sequenza di stati e il logger, con i suoi
 * thread, li consuma (i.e. li salva sul disco). Qui l'hypertreading dovrebbe
 * aiutare siccome logger e simulatore dovrebbero usare parti diverse della CPU.
 */
/* @param s all the simulation data
 * @param dump used to dump the state of the simulation, returns null if
 * something goes wrong.
 */
void
simulation_run(simulation_t *s, bool (*dump)(simulation_t *)) {
	assert(s && dump);
	assert(s->old_state && s->new_state && s->params);
	assert(s->tau > 0 && s->L > 0);
	assert(s->Wstar >= 3 && s->Lstar >= 3);
	assert(s->theta >= 0 && s->theta <= 1);

	syslog(LOG_INFO, "starting simulation");
	uint32_t rng_state = s->seed;
	for (uint64_t loop0 = 0; loop0 < s->h; loop0++) {
		/* Skipping the border */
		for (uint64_t i = 1; i < s->Lstar - 1; i++) {
			for (uint64_t j = 1; j < s->Wstar - 1; j++) {
				const uint64_t ij = sim_index(i, j, s);
				const params_t *cur_param = s->params + ij;
				const float beta = 60*(1 + cur_param->F/10); /* burning rate */
				const float old_B = s->old_state[ij].B;
				assert(old_B >= 0);
				const bool old_N = s->old_state[ij].N;

				const uint64_t n_dir = sizeof Gamma / sizeof Gamma[0];
				bool V = false;
				for (uint64_t loop1 = 0; loop1 < n_dir && !V; loop1++) {
					const int8_t e1 = Gamma[loop1][0];
					const int8_t e2 = Gamma[loop1][1];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
					const uint64_t ie1je2 = sim_index(i+e1, j+e2, s);
#pragma clang diagnostic pop
					const params_t *adj_param = s->params + ie1je2;
					const state_t *adj_old_state = s->old_state +ie1je2;
					if (!adj_old_state->N) {
						continue;
					}
					/* Calculating probability */
					const float C = sinf(pi*adj_old_state->B/adj_param->gamma);
					const float d = (1 - 0.5f*fabsf((float) e1*e2));
					const float fw = expf(
						s->k1*adj_param->F
						*(e1*cosf(adj_param->D) + e2*sinf(adj_param->D))
						/sqrtf((float) (e1*e1 + e2*e2))
					) + rngf(&rng_state);
					const float fP = expf(
						s->k2*atanf((cur_param->P - adj_param->P)/s->L)
					) + rngf(&rng_state);
					const float p = s->k0 * cur_param->S * d * fw * fP * C;

					/* this is Q, N has been purposely removed because it's
					 * checked a priori
					 */
					V |= (p > s->theta);
				}

				/* NOTE: in this eqation u has been purposely removed */
				s->new_state[ij].N = old_B > 0 ? V : false;
				s->new_state[ij].B = old_N > 0 ? maxf(0, old_B - beta*s->tau) : old_B;
				assert(s->new_state[ij-1].B >= 0);
				assert(s->old_state[ij-1].B >= 0);
				assert(s->new_state[ij].B <= s->old_state[ij].B);
			}
		}
		if ((loop0+1) % s->s == 0) {
			syslog(LOG_INFO, "starting to dump the state of the simulation: %"
				PRIu64, loop0/s->s);
			(void) dump(s);
			syslog(LOG_INFO, "finished to dump the state of the simulation: %"
				PRIu64, loop0/s->s);
		}
		if (should_stop_early) {
			syslog(LOG_INFO, "exiting the simulation prematurely due to SIGINT");
			return;
		}
		state_t *tmp = s->old_state;
		s->old_state = s->new_state;
		s->new_state = tmp;
	}
	syslog(LOG_INFO, "finished simulation");
}

#ifdef TEST
#include <stdio.h>
#include <stdlib.h>

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
	return true;
}

int main(int argc, char const *argv[]) {
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
	};

	if (!(s.old_state && s.new_state && s.params)) {
		return EXIT_FAILURE;
	}

	for (uint64_t i = 0; i < area; i++) {
		s.old_state[i] = (state_t){.B = 0.5f, .N = false, .gamma = 10};
		s.new_state[i] = (state_t){0};
		s.params[i] = (params_t){.P = 1, .S = 0.7f, .F = 1, .D = pi};
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

	return EXIT_SUCCESS;
}
#endif
