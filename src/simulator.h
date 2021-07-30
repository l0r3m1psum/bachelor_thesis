#include <stdbool.h>
#include <stdint.h>

/* NOTE: Those are invariant during the simulation, can they be globals without
 * problems?
 */
typedef struct {
	double time_step;
	double cell_length;
	double cell_width;
	uint64_t cell_col_num;
	uint64_t cell_row_num;
} simulation_parameters_t;

typedef struct {
	uint16_t altimetry;
	uint8_t forest;
	uint8_t urbanization;
	uint8_t water1;
	bool water2;
	uint8_t naturemap;
} cell_parameters_t;
