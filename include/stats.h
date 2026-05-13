#ifndef STATS_H
#define STATS_H

typedef struct {
	char key[11]; // XXXX-XX-XX

	// In seconds
	int focusTime;
	int breakTime;
} Stat;

Stat create_today_stats(void);
int Save_Stat(Stat save_stat);

#endif