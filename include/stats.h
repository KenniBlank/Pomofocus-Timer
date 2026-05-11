#ifndef STATS_H
#define STATS_H

struct Stat {
	char key[11]; // XXXX-XX-XX

	// In seconds
	int focusTime;
	int breakTime;
};

typedef struct {
	struct Stat *total_stats;
	int capacity;
} Stats ;

struct Stat *get_today_stats(Stats* stats);
int Init_Stats(Stats *stats);
void Clean_Stats(Stats *stats);

#endif