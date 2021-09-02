#include "simulator.h"

#include <math.h>

static const int8_t Gamma[8][2] = {
	{0, 1}, {1, 0}, {1, 1}, {0, -1}, {-1, 0}, {-1, -1}, {1, -1}, {-1, 1}
}; /* All offsets around a cell */
static_assert(sizeof Gamma == 16, "bad size");

static const float pi = 3.14159265359f;

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
 */
inline static float
rngf(uint32_t *xn) {
	const uint32_t m = 1u << 31, a = 37769685, c = 12345, max = m - 1;
	*xn = (a * (*xn) + c) % m;
	const float u1 = (float) (*xn)/max;
	*xn = (a * (*xn) + c) % m;
	const float u2 = (float) (*xn)/max;
	const float res = sqrtf(-2.0f*logf(u1))*cosf(2*pi*u2)/2.5f;
	return res;
}

/* TODO: completare tutti i check e metterlo nell'assert di simulation_run. */
static bool
validate_simulation(simulation_t *s) {
	const bool all_pointer_present = s->old_state && s->new_state && s->params && s->gamma;
	const bool at_least_3by3 = s->Wstar >= 3 && s->Lstar >= 3;
	const bool between_0and1 = s->theta >= 0 && s->theta <= 1;
	const bool res = all_pointer_present && at_least_3by3 && between_0and1;
	return res;
}

/* NOTE: non so se il simulatore ha un bug o io non so scegliere bene i
 * parametri.
 */
void
simulation_run(simulation_t *s, bool (*dump)(simulation_t *)) {
	assert(s && dump);
	assert(s->old_state && s->new_state && s->params && s->gamma);
	assert(s->tau > 0 && s->theta > 0 && s->L > 0);

	uint32_t rng_state = s->seed;
	for (uint64_t loop0 = 0; loop0 < s->h; loop0++) {
		/* Skipping the border */
		for (uint64_t i = 1; i < s->Lstar - 1; i++) {
			for (uint64_t j = 1; j < s->Wstar - 1; j++) {
				const uint64_t ij = i + j*s->Wstar;
				const float beta = 60*(1 + s->params[ij].F/10); /* burning rate */
				const float old_B = s->old_state[ij].B;
				const bool old_N = s->old_state[ij].N;

				const uint64_t n_dir = sizeof Gamma / sizeof Gamma[0];
				bool V = false;
				for (uint64_t loop1 = 0; loop1 < n_dir && !V; loop1++) {
					const int8_t e1 = Gamma[loop1][0];
					const int8_t e2 = Gamma[loop1][1];
					const uint64_t ie1je2 = (i+e1) + (j+e2)*s->Wstar;
					// Calculating probability
					const float C = sinf(pi*s->old_state[ie1je2].B/s->gamma[ie1je2]);
					const float d = (1 - 0.5f*fabsf((float) e1*e2));
					const float fw = expf(
						s->k1*s->params[ie1je2].F
						*(e1*cosf(s->params[ie1je2].D) + e2*sinf(s->params[ie1je2].D))
						/sqrtf(e1*e1 + e2*e2)
					) + rngf(&rng_state);
					const float fP = expf(
						s->k2*atanf((s->params[ij].P-s->params[ie1je2].P)/s->L)
					) + rngf(&rng_state);
					const float p = s->k0 * s->params[ij].S * C * d * fw * fP;

					V = V || (p * s->old_state[ij].N > s->theta);
				}

				/* NOTE: in this eqation u has been purposely removed */
				s->new_state[ij].N = old_B > 0 ? V : false;
				s->new_state[ij].B = old_N > 0 ? maxf(0, old_B - beta*s->tau) : old_B;
			}
		}
		if (s->h % s->s == 0) {
			(void) dump(s);
		}
		state_t *tmp = s->old_state;
		s->old_state = s->new_state;
		s->new_state = tmp;
	}
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
		.theta = 0.4,
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
		s.old_state[i] = (state_t){.B = 0.5, .N = false};
		s.new_state[i] = (state_t){0};
		s.params[i] = (params_t){.P = 1, .S = 0.7, .F = 1, .D = pi};
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

	return EXIT_SUCCESS;
}
#endif
