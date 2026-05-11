#include "stats.h"
#include <stdlib.h>
#include <time.h>

// time_t t = time(NULL);
// struct tm tm = *localtime(&t);
// data->stats.total_stats[0].__day__ = tm.tm_mday;
// data->stats.total_stats[0].__month__ = tm.tm_mon + 1;
// data->stats.total_stats[0].__year__ = (tm.tm_year + 1900);

static void get_key(char *key, int year, int month, int day) {
	// 0-3 Year
	// -
	// 5-6 Month
	// -
	// 8-9 Day
	key[3] = year % 10 + '0';
	key[2] = (year / 10) % 10 + '0';
	key[1] = (year / 100) % 10 + '0';
	key[0] = (year / 1000) % 10 + '0';

	key[4] = '-';
	key[6] = month % 10 + '0';
	key[5] = (month / 10) % 10 + '0';

	key[7] = '-';
	key[9] = day % 10 + '0';
	key[8] = (day / 10) % 10 + '0';

	key[10] = '\0';
}

struct Stat *get_today_stats(Stats* stats) {
	return &stats->total_stats[0];
}

int Init_Stats(Stats *stats) {
	stats->capacity = 32;
	stats->total_stats = malloc(sizeof(struct Stat) * stats->capacity);
	if (stats->total_stats) {
		return 0;
	}
	get_key(stats->total_stats[0].key, 2026, 11, 06);
	return 1;
}

void Clean_Stats(Stats *stats) {
	if (stats->total_stats) {
		free(stats->total_stats);
		stats->total_stats = NULL;
	}
}