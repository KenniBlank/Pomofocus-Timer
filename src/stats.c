#include "stats.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define SAVE_FILE "stats.json"
#include "cJSON.h"

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

Stat create_today_stats(void) {
	Stat stat;

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	int day = tm.tm_mday;
	int month = tm.tm_mon + 1;
	int year = (tm.tm_year + 1900);
	get_key(stat.key, year, month, day);

	stat.breakTime = 0;
	stat.focusTime = 0;

	return stat;
}

int Save_Stat(Stat save_stat) {
	FILE *file = fopen(SAVE_FILE, "r");
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *buffer = (char *) malloc(fileSize + 1);
	fread(buffer, sizeof(char), fileSize, file);
	buffer[fileSize] = '\0';
	fclose(file);

	file = fopen(SAVE_FILE, "w");

	cJSON *json = cJSON_Parse(buffer);

	cJSON *key = cJSON_CreateObject();
	cJSON_AddItemToObject(json, save_stat.key, key);

	cJSON *focusTime = cJSON_CreateNumber(save_stat.focusTime);
	cJSON_AddItemToObject(key, "focusTime", focusTime);
	cJSON *breakTime = cJSON_CreateNumber(save_stat.breakTime);
	cJSON_AddItemToObject(key, "breakTime", breakTime);

	char *str = cJSON_Print(json);
	fputs(str, file);

	free(str);
	free(buffer);
	cJSON_Delete(json);
	fclose(file);
	return 0;
}