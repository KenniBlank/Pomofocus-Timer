#ifndef STATS_H
#define STATS_H

#include <stdint.h>

struct Stat {
	// Bitwise shift, save year_month_day
	uint32_t date;
	// In seconds
	uint32_t focusTime;
	uint32_t breakTime;
};

typedef struct {
	struct Stat *stats;
	int index;
	int capacity;

	int totalFocusCount; // Calculated on demand
} Stats ;

#endif