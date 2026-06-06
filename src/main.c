#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "cJSON.h"
#include "Notification.h"
#include "raylib.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"

#include "tasks.h"
#include "stats.h"

#define BASE_FONT_SIZE (16)
#define FONT_SIZE(SIZE) ((SIZE * BASE_FONT_SIZE * g_UI_SCALE) / 100.0f)

#define RAYLIB_COLOR_TO_CLAY_COLOR(color) (Clay_Color) { .r = color.r, .g = color.g, .b = color.b, .a = color.a }
#define STRING_TO_CLAY_STRING(str) (Clay_String) { .chars = str.chars, .isStaticallyAllocated = true, .length = str.length }

#define onHover(color) (Clay_Color) {.r = color.r, .g = color.g, .b = color.b, .a = (color.a / 2.0f)}

#define getWidthRatio(baseWidth) (GetScreenWidth() / ((baseWidth) * 1.0f))
#define getHeightRatio(baseHeight) (GetScreenHeight() / ((baseHeight) * 1.0f))
#define getCubicRatio(baseWidth, baseHeight) fmin(getWidthRatio(baseWidth), getHeightRatio(baseHeight))
#define getFontSize(baseWidth, baseHeight, fontSize) getCubicRatio(baseWidth, baseHeight) * (fontSize)
#define getFontHelper(data, fontSize) FONT_SIZE(getFontSize(data->baseDeviceDimensions.x, data->baseDeviceDimensions.y, fontSize))

#define INVERT_CLAY_COLOR(color) \
	color = (Clay_Color) {\
		.r = 255 - color.r,\
		.g = 255 - color.g,\
		.b = 255 - color.b,\
		.a = color.a\
	};

#ifndef APP_TITLE
	#define APP_TITLE "PomoFocus Timer"
#endif

#define FILE_NAME "data.json"
#define TASKS_LIST "data.json"
#define APP_ICON_LOC "./resources/appIcon-32.png"

#ifndef DEBUG
	#define PRINT(variable) ((void*)0)
	#define PRINT_S(variable) ((void*)0)
#else
	#define PRINT(variable) printf(#variable " = %g\n", (double) variable); fflush(stdout)
	#define PRINT_S(variable) printf(#variable " = %s\n", variable); fflush(stdout)
#endif

#define max(a, b) (a) > (b) ? (a): (b)
#define min(a, b) (a) > (b) ? (b): (a)

#define HEADER1 (200)
#define HEADER2 (150)
#define HEADER3 (117)
#define HEADER4 (100)
#define HEADER5 (83)
#define HEADER6 (67)
#define PARAGRAPH (100)
#define LETTER_SPACING (BASE_FONT_SIZE >> 3)
#define BORDER_RADIUS (6 * 4)
#define BUTTON_PADDING (6 * 4)
#define BORDER_WIDTH (5)

enum {
	TASK_COMPLETED,
	TASK_INCOMPLETE,
	TASK_EDITING
};

// GLOBAL variables
bool g_reinit_clay;
float g_UI_SCALE;
bool g_task_index;
bool g_playSound;

enum {
	FONT_ID_DEFAULT,
	FONT_ID_16_PX,
	FONT_ID_32_PX,
	FONT_ID_64_PX,
	FONT_SIZE
};

enum {
	// Overall Background Color
	COLOR_BACKGROUND_INACTIVE,
	COLOR_BACKGROUND_ACTIVE,

	// Main section's color
	COLOR_BACKGROUND_MAIN_INACTIVE,
	COLOR_BACKGROUND_MAIN_ACTIVE,

	// Task List:
	COLOR_BACKGROUND_TASKS,
	COLOR_SELECTED_TASK_INDICATOR,
	COLOR_BACKGROUND_SELECTED_TASK,
	COLOR_BACKGROUND_TASK,

	// FSB = Focus, Short break, Long break
	COLOR_BACKGROUND_FSL_BUTTON,
	COLOR_BACKGROUND_FSL_BUTTON_SELECTED,

	// SS = Start Stop
	COLOR_BACKGROUND_SS_BUTTON,
	COLOR_BACKGROUND_SS_BUTTON_SELECTED,

	COLOR_TXT,
	COLOR_BORDER,
	COLOR_SIZE
};

enum {
	FOCUS_TIMER,
	SHORT_BREAK_TIMER,
	LONG_BREAK_TIMER,
	TIMERS_COUNT
};

// For HEIGHT always 1.5x width
#define DEVICE_WATCH_W 240
#define DEVICE_SMART_PHONE_W 340

enum {
	SOUND_CLICK,

	SOUND_FOCUS,
	SOUND_BREAK,

	SOUND_COUNT
};

typedef struct {
	Sound* sounds;
	int count[SOUND_COUNT]; // How many times to play sound!
} SoundData;

typedef enum {
	STATE_FOCUS,
	STATE_FOCUS_PAUSED,
	STATE_SHORT_BREAK,
	STATE_SHORT_BREAK_PAUSED,
	STATE_LONG_BREAK,
	STATE_LONG_BREAK_PAUSED,
} AppState;
#define APP_PAUSED(state) ((state) % 2 != 0)

typedef enum {
	DEVICE_WATCH,
	DEVICE_SMART_PHONE,
	DEVICE_TABLET,
	DEVICE_LAPTOP,
} Device;

typedef struct {
	float timer;
	float savedTime;

	struct String timerStr;
} TimerConstraints;

typedef struct {
	int time_constants[TIMERS_COUNT];
	float masterVolume;
	int focusCount;
	int FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK;
	float REPEAT_DELAY; // How much time to wait to activate repeat
	float REPEAT_INTERVAL; // How much time per repeat
	float SCROLL_MULTIPLIER;
	bool selectedTaskOnTop; // Moves task to top
	bool onTaskSwitchResetTimer;
} Options;

typedef struct {
	Tasks tasks;
	AppState appState;
	Options options;
	TimerConstraints timerConstraints;
	Vector2 __DEVICE_DIMENSION__; // Only for opening back again in same width

	Stat stat_today;

	Clay_Color colors[COLOR_SIZE];
	Device device;
	Vector2 baseDeviceDimensions;
	SoundData sounds;
} PomodoroData;

void Expected_End_Time(PomodoroData *pomo_data) {
	// pomo_data->timerConstraints.timer;
	// pomo_data->options.FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK
}

void ResetTimer(PomodoroData* pomo_data);
void Start_Stop_Pressed(PomodoroData* pomo_data);

static void HANDLE_EVENTS(uint32_t*, Clay_Arena*, PomodoroData* );
static Clay_RenderCommandArray ClayCreateLayout(PomodoroData*);

#define JSON(obj, key) ((obj) != NULL && cJSON_HasObjectItem((obj), (key)) ? cJSON_GetObjectItem((obj), (key)) : NULL)
#define JSON_INT(obj, key, field) do { \
	cJSON *it = JSON(obj, key); \
	if (it == NULL) { \
		fprintf(stderr, "Warning: Missing required field '%s'\n", (key)); \
	} else if (!cJSON_IsNumber(it)) { \
		fprintf(stderr, "Warning: Field '%s' is not a number (got type %d)\n", (key), it->type); \
	} else { \
		(field) = it->valueint; \
	} \
} while (0)

#define JSON_DBL(obj, key, field) do { \
	cJSON *it = JSON(obj, key); \
	if (it == NULL) { \
		fprintf(stderr, "Warning: Missing required field '%s'\n", (key)); \
	} else if (!cJSON_IsNumber(it)) { \
		fprintf(stderr, "Warning: Field '%s' is not a number (got type %d)\n", (key), it->type); \
	} else { \
		(field) = it->valuedouble; \
	} \
} while (0)

#define JSON_STR(obj, key, field) do { \
	cJSON *it = JSON(obj, key); \
	if (it == NULL) { \
		fprintf(stderr, "Warning: Missing required field '%s'\n", (key)); \
	} else if (!cJSON_IsString(it)) { \
		fprintf(stderr, "Warning: Field '%s' is not a string (got type %d)\n", (key), it->type); \
	} else { \
		(field) = it->valuestring; \
	} \
} while (0)

static int LoadData(PomodoroData* data) {
	FILE *file = fopen(FILE_NAME, "r");
	if (file == NULL) {
		return 1; // Todo: handle Error
	}

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *buffer = (char *) malloc(fileSize + 1);
	if (buffer == NULL) {
		fclose(file);
		return 1;
	}
	fread(buffer, sizeof(char), fileSize, file);
	buffer[fileSize] = '\0';

	cJSON *json = cJSON_Parse(buffer);
	if (json == NULL) {
		free(buffer);
		fclose(file);
		return 1;
	}

	// Options
	cJSON *options = JSON(json, "options");
	if (options != NULL) {
		JSON_DBL(options, "masterVolume", data->options.masterVolume);
		// JSON_INT(options, "focusCount", data->options.focusCount);
		JSON_INT(options, "FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK", data->options.FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK);
		JSON_DBL(options, "REPEAT_DELAY", data->options.REPEAT_DELAY);
		JSON_DBL(options, "REPEAT_INTERVAL", data->options.REPEAT_INTERVAL);
		JSON_DBL(options, "SCROLL_MULTIPLIER", data->options.SCROLL_MULTIPLIER);
		JSON_INT(options, "selectedTaskOnTop", data->options.selectedTaskOnTop);
		JSON_INT(options, "onTaskSwitchResetTimer", data->options.onTaskSwitchResetTimer);

		cJSON *time_constants = JSON(options, "time_constants");
		if (time_constants != NULL) {
			cJSON *time_constant = NULL;
			int index = 0;
			cJSON_ArrayForEach(time_constant, time_constants) {
				if (cJSON_IsNumber(time_constant)) {
					data->options.time_constants[index++] = time_constant->valueint;
				}
			}
		}
	}

	// Tasks:
	cJSON *tasks = JSON(json, "tasks");
	if (tasks != NULL) {
		JSON_INT(tasks, "currentSelectedTask", data->tasks.currentSelectedTask);

		cJSON *tasks_arr = JSON(tasks, "tasks");
		if (tasks_arr != NULL) {
			cJSON *task = NULL;
			cJSON_ArrayForEach(task, tasks_arr) {
				int count = 0;
				int expected = 0;
				int completed_val = 0;
				int descDefined_val = 0;

				JSON_INT(task, "count", count);
				JSON_INT(task, "expected", expected);
				JSON_INT(task, "__completed__", completed_val);
				JSON_INT(task, "__descDefined__", descDefined_val);

				bool __completed__ = (bool)completed_val;
				bool __descDefined__ = (bool)descDefined_val;

				if (__descDefined__) {
					char *desc = NULL;
					JSON_STR(task, "desc", desc);
					if (desc != NULL) {
						Add_Task(&data->tasks, CreateNewTask(desc, count, expected), data->options.selectedTaskOnTop);
					}
				} else {
					if (count || expected) {
						Add_Task(&data->tasks, CreateNewTask("", count, expected), data->options.selectedTaskOnTop);
					}
				}
				data->tasks.tasks[data->tasks.currentSelectedTask].__completed__ = __completed__;
			}
		}
	}

	// AppState
	int app_state_val = 0;
	JSON_INT(json, "appState", app_state_val);
	data->appState = app_state_val;
	// APP_PAUSED(data->appState);
	data->appState += !APP_PAUSED(data->appState) ? 1 : 0;

	// TimerConstraints
	cJSON *timerConstraints = JSON(json, "timerConstraints");
	if (timerConstraints != NULL) {
		JSON_DBL(timerConstraints, "timer", data->timerConstraints.timer);
		JSON_DBL(timerConstraints, "savedTime", data->timerConstraints.savedTime);
	}

	// __DEVICE_DIMENSION__
	cJSON *__DEVICE_DIMENSION__ = JSON(json, "__DEVICE_DIMENSION__");
	if (__DEVICE_DIMENSION__ != NULL) {
		JSON_INT(__DEVICE_DIMENSION__, "x", data->__DEVICE_DIMENSION__.x);
		JSON_INT(__DEVICE_DIMENSION__, "y", data->__DEVICE_DIMENSION__.y);
	}

	// Stats
	cJSON *stat_today = JSON(json, "stat_today");
	if (stat_today != NULL) {
		char *key_str = NULL;
		JSON_STR(stat_today, "key", key_str);
		if (key_str != NULL && strcmp(data->stat_today.key, key_str) == 0) {
			strcpy(data->stat_today.key, key_str);
			JSON_INT(stat_today, "focusTime", data->stat_today.focusTime);
			JSON_INT(stat_today, "breakTime", data->stat_today.breakTime);
		} else {
			Save_Stat(data->stat_today);
		}
	}

	free(buffer);
	cJSON_Delete(json);
	fclose(file);
	return 0;
}

// TODO: err handling later
static int SaveData(PomodoroData* data) {
	// Save current device size!
	data->__DEVICE_DIMENSION__ = (Vector2) {.x = GetScreenWidth(), .y = GetScreenHeight()};

	FILE *save_file = fopen(FILE_NAME, "w");
	cJSON *json = cJSON_CreateObject();

	// Tasks:
	cJSON *tasks = cJSON_CreateObject();
	cJSON_AddItemToObject(json, "tasks", tasks);

	cJSON *tasks_arr = cJSON_CreateArray();
	cJSON_AddItemToObject(tasks, "tasks", tasks_arr);

	cJSON *currentSelectedTask = cJSON_CreateNumber(data->tasks.currentSelectedTask);
	cJSON_AddItemToObject(tasks, "currentSelectedTask", currentSelectedTask);

	for (int i = data->tasks.tasks_count - 1; i >= 0; --i) {
		struct Task *task = &data->tasks.tasks[i];

		cJSON *tsk = cJSON_CreateObject();
		cJSON_AddItemToArray(tasks_arr, tsk);

		cJSON *count = cJSON_CreateNumber(task->count);
		cJSON_AddItemToObject(tsk, "count", count);

		cJSON *expected = cJSON_CreateNumber(task->expected);
		cJSON_AddItemToObject(tsk, "expected", expected);

		cJSON *__completed__ = cJSON_CreateNumber(task->__completed__);
		cJSON_AddItemToObject(tsk, "__completed__", __completed__);

		cJSON *__descDefined__ = cJSON_CreateNumber(task->__descDefined__);
		cJSON_AddItemToObject(tsk, "__descDefined__", __descDefined__);

		if (task->__descDefined__) {
			cJSON *desc = cJSON_CreateString(!task->desc.length? " ": task->desc.chars);
			cJSON_AddItemToObject(tsk, "desc", desc);
		}
	}

	// AppState
	cJSON *appState = cJSON_CreateNumber(data->appState);
	cJSON_AddItemToObject(json, "appState", appState);

	// Options:
	Options* opts = &data->options;

	cJSON *options = cJSON_CreateObject();
	cJSON_AddItemToObject(json, "options", options);

	cJSON *masterVolume = cJSON_CreateNumber(opts->masterVolume);
	cJSON_AddItemToObject(options, "masterVolume", masterVolume);

	// cJSON *focusCount = cJSON_CreateNumber(opts->focusCount);
	// cJSON_AddItemToObject(options, "focusCount", focusCount);

	cJSON *FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK = cJSON_CreateNumber(opts->FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK);
	cJSON_AddItemToObject(options, "FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK", FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK);

	cJSON *REPEAT_DELAY = cJSON_CreateNumber(opts->REPEAT_DELAY);
	cJSON_AddItemToObject(options, "REPEAT_DELAY", REPEAT_DELAY);

	cJSON *REPEAT_INTERVAL = cJSON_CreateNumber(opts->REPEAT_INTERVAL);
	cJSON_AddItemToObject(options, "REPEAT_INTERVAL", REPEAT_INTERVAL);

	cJSON *SCROLL_MULTIPLIER = cJSON_CreateNumber(opts->SCROLL_MULTIPLIER);
	cJSON_AddItemToObject(options, "SCROLL_MULTIPLIER", SCROLL_MULTIPLIER);

	cJSON *selectedTaskOnTop = cJSON_CreateNumber(opts->selectedTaskOnTop);
	cJSON_AddItemToObject(options, "selectedTaskOnTop", selectedTaskOnTop);

	cJSON *onTaskSwitchResetTimer = cJSON_CreateNumber(opts->onTaskSwitchResetTimer);
	cJSON_AddItemToObject(options, "onTaskSwitchResetTimer", onTaskSwitchResetTimer);

	cJSON *time_constants = cJSON_CreateIntArray(opts->time_constants, 3);
	cJSON_AddItemToObject(options, "time_constants", time_constants);

	// Timer Constraints
	TimerConstraints *time_constraints = &data->timerConstraints;
	cJSON *timerConstraints = cJSON_CreateObject();
	cJSON_AddItemToObject(json, "timerConstraints", timerConstraints);

	cJSON *timer = cJSON_CreateNumber(time_constraints->timer);
	cJSON_AddItemToObject(timerConstraints, "timer", timer);

	cJSON *savedTime = cJSON_CreateNumber(time_constraints->savedTime);
	cJSON_AddItemToObject(timerConstraints, "savedTime", savedTime);

	// __DEVICE_DIMENSION__
	Vector2 *_DEVICE_DIMENSION_ = &data->__DEVICE_DIMENSION__;
	cJSON *__DEVICE_DIMENSION__ = cJSON_CreateObject();
	cJSON_AddItemToObject(json, "__DEVICE_DIMENSION__", __DEVICE_DIMENSION__);

	cJSON *x = cJSON_CreateNumber(_DEVICE_DIMENSION_->x);
	cJSON_AddItemToObject(__DEVICE_DIMENSION__, "x", x);
	cJSON *y = cJSON_CreateNumber(_DEVICE_DIMENSION_->y);
	cJSON_AddItemToObject(__DEVICE_DIMENSION__, "y", y);

	// stats
	cJSON *stat_today = cJSON_CreateObject();
	cJSON_AddItemToObject(json, "stat_today", stat_today);
	cJSON *key = cJSON_CreateString(data->stat_today.key);
	cJSON_AddItemToObject(stat_today, "key", key);
	cJSON *focusTime = cJSON_CreateNumber(data->stat_today.focusTime);
	cJSON_AddItemToObject(stat_today, "focusTime", focusTime);
	cJSON *breakTime = cJSON_CreateNumber(data->stat_today.breakTime);
	cJSON_AddItemToObject(stat_today, "breakTime", breakTime);

	// Save to file and cleanups:
	char* str = cJSON_Print(json);
	fprintf(save_file, "%s", str);
	free(str);
	cJSON_Delete(json);
	fclose(save_file);
	return 0;
}

static void HandleClayErrors(Clay_ErrorData errorData) {
	PRINT_S(errorData.errorText.chars);

	switch (errorData.errorType) {
		case CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED: {
			g_reinit_clay = true;
			Clay_SetMaxElementCount(Clay_GetMaxElementCount() * 2);
			break;
		}

		case CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED: {
			g_reinit_clay = true;
			Clay_SetMaxMeasureTextCacheWordCount(Clay_GetMaxMeasureTextCacheWordCount() * 2);
			break;
		}

		default: {
			PRINT_S(errorData.errorText.chars);
			break;
		}
	}
}

void THEME_LIGHT(PomodoroData *data) {
	data->colors[COLOR_BACKGROUND_INACTIVE] = (Clay_Color) {.r = 252, .g = 252, .b = 252, .a = 255};
	data->colors[COLOR_BACKGROUND_ACTIVE] = (Clay_Color) {.r = 242, .g = 242, .b = 242, .a = 255};

	data->colors[COLOR_BACKGROUND_MAIN_INACTIVE] = data->colors[COLOR_BACKGROUND_INACTIVE];
	data->colors[COLOR_BACKGROUND_MAIN_ACTIVE] = (Clay_Color) {.r = 178, .g = 242, .b = 187, .a = 255};

	data->colors[COLOR_BACKGROUND_TASKS] = data->colors[COLOR_BACKGROUND_INACTIVE];
	data->colors[COLOR_SELECTED_TASK_INDICATOR] = (Clay_Color) {.r = 200, .g = 100, .b = 100, .a = 255};
	data->colors[COLOR_BACKGROUND_SELECTED_TASK] = data->colors[COLOR_BACKGROUND_INACTIVE];
	data->colors[COLOR_BACKGROUND_TASK] = data->colors[COLOR_BACKGROUND_INACTIVE];

	data->colors[COLOR_BACKGROUND_FSL_BUTTON] = data->colors[COLOR_BACKGROUND_ACTIVE];
	data->colors[COLOR_BACKGROUND_FSL_BUTTON_SELECTED] = data->colors[COLOR_BACKGROUND_INACTIVE];

	data->colors[COLOR_BACKGROUND_SS_BUTTON] = (Clay_Color) {.r = 255, .g = 249, .b = 219, .a = 255};
	data->colors[COLOR_BACKGROUND_SS_BUTTON_SELECTED] = (Clay_Color) {.r = 250, .g = 82, .b = 82, .a = 255};

	data->colors[COLOR_TXT] = (Clay_Color){33, 37, 41, 255};
	data->colors[COLOR_BORDER] = (Clay_Color){100, 100, 100, 255};
}

int initialize_data(PomodoroData* data) {
	// Global Data
	g_UI_SCALE = 2.0f;
	g_reinit_clay = false;
	g_task_index = true;
	g_playSound = false;

	// Pomodata Defaults:
	*data = (PomodoroData) {
		.appState = STATE_FOCUS_PAUSED,
		.baseDeviceDimensions = { .x = DEVICE_SMART_PHONE_W * g_UI_SCALE, .y = (DEVICE_SMART_PHONE_W * 1.5f) * g_UI_SCALE },
		.__DEVICE_DIMENSION__ = { .x = DEVICE_SMART_PHONE_W * g_UI_SCALE, .y = (DEVICE_SMART_PHONE_W * 1.5f) * g_UI_SCALE },
		.options = {
			.masterVolume = 1.0f,
			.FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK = 4,
			.focusCount = 0,
			.REPEAT_DELAY = 0.20f,
			.REPEAT_INTERVAL = 0.06f,
			.SCROLL_MULTIPLIER = 10.0f,
			.selectedTaskOnTop = true,
			.onTaskSwitchResetTimer = true,
		},
		.stat_today = create_today_stats()
	};

	Init_Tasks(&data->tasks);

	data->timerConstraints.timerStr = (struct String) {
		.length = 0
	};
	data->options.time_constants[FOCUS_TIMER] = 25 * 60;
	data->options.time_constants[LONG_BREAK_TIMER] = 15 * 60;
	data->options.time_constants[SHORT_BREAK_TIMER] = 5 * 60;

	data->timerConstraints.timer = data->options.time_constants[data->appState >> 1];
	data->timerConstraints.savedTime = 0.0f;

	// Colors Definitions:
	THEME_LIGHT(data);

	data->sounds.sounds = malloc(sizeof(Sound) * SOUND_COUNT);
	if (data->sounds.sounds == NULL) return 1;
	data->sounds.sounds[SOUND_CLICK] = LoadSound("resources/sounds/mouseClick.wav");
	data->sounds.count[SOUND_CLICK] = 1;
	data->sounds.sounds[SOUND_FOCUS] = LoadSound("resources/sounds/focus.wav");
	data->sounds.count[SOUND_FOCUS] = 3;
	data->sounds.sounds[SOUND_BREAK] = LoadSound("resources/sounds/break.wav");
	data->sounds.count[SOUND_BREAK] = -1;

	LoadData(data);
	return 0;
}

void clean_data(PomodoroData* pomo_data) {
	float initial_constant = pomo_data->options.time_constants[pomo_data->appState >> 1];
	float delta = (initial_constant - pomo_data->timerConstraints.timer) - pomo_data->timerConstraints.savedTime;
	if (pomo_data->appState == STATE_FOCUS) pomo_data->stat_today.focusTime += delta;
	else pomo_data->stat_today.breakTime += delta;
	pomo_data->timerConstraints.savedTime += delta;

	SaveData(pomo_data);

	Clean_Tasks(&pomo_data->tasks);
	if (pomo_data->sounds.sounds) {
		for (int i = 0; i < SOUND_COUNT; i++) {
			UnloadSound(pomo_data->sounds.sounds[i]);
		}
		free(pomo_data->sounds.sounds);
		pomo_data->sounds.sounds = NULL;
	}
}

int main(void) {
	ChangeDirectory(GetApplicationDirectory());

	InitAudioDevice();
	PomodoroData pomo_data;
	if (initialize_data(&pomo_data)) {
		clean_data(&pomo_data);
		return 0;
	}

	Clay_Raylib_Initialize(pomo_data.__DEVICE_DIMENSION__.x, pomo_data.__DEVICE_DIMENSION__.y, APP_TITLE, FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);

	uint32_t totalMemorySize = Clay_MinMemorySize();
	Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
	Clay_Initialize(clayMemory, (Clay_Dimensions) { (float)GetScreenWidth(), (float)GetScreenHeight() }, (Clay_ErrorHandler) { HandleClayErrors, 0 });

	Font fonts[FONT_SIZE];
	fonts[FONT_ID_DEFAULT] = GetFontDefault();

	#define FONT_LOC "resources/fonts/RobotoMono-Regular.ttf"
	#define FONT_LOC2 "resources/fonts/Roboto-Regular.ttf"

	fonts[FONT_ID_64_PX] = LoadFontEx(FONT_LOC2, 64 * 3, NULL, 0);
	SetTextureFilter(fonts[FONT_ID_64_PX].texture, TEXTURE_FILTER_BILINEAR);

	fonts[FONT_ID_32_PX] = LoadFontEx(FONT_LOC, 32 * 3, NULL, 0);
	SetTextureFilter(fonts[FONT_ID_32_PX].texture, TEXTURE_FILTER_BILINEAR);

	fonts[FONT_ID_16_PX] = LoadFontEx(FONT_LOC, 16 * 3, NULL, 0);
	SetTextureFilter(fonts[FONT_ID_16_PX].texture, TEXTURE_FILTER_BILINEAR);

	for (int i = 0; i < FONT_SIZE; i++) {
		if (!IsFontValid(fonts[i])) {
			UnloadFont(fonts[i]);
			fonts[i] = GetFontDefault();
		}
	}

	Image appIcon = LoadImage(APP_ICON_LOC);
	if (IsImageValid(appIcon)) {
		SetWindowIcon(appIcon);
	}

	Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

	SetTargetFPS(24);
	SetExitKey(KEY_NULL);

	// Sound
	int playSoundCount = 0;
	while (!WindowShouldClose()) {
		HANDLE_EVENTS(&totalMemorySize, &clayMemory, &pomo_data);
		if (strcmp(pomo_data.stat_today.key, create_today_stats().key)) {
			Save_Stat(pomo_data.stat_today);
			pomo_data.stat_today = create_today_stats();
		}

		if (pomo_data.appState % 2 == 0) {
			if (pomo_data.timerConstraints.timer <= 0.0f) {
				if (pomo_data.appState == STATE_FOCUS) {
					pomo_data.options.focusCount++;
					if (pomo_data.tasks.tasks_count > 0) {
						Add_Focus_Count_To_Task(&pomo_data.tasks.tasks[pomo_data.tasks.currentSelectedTask]);
					}
					Notify(APP_TITLE, "Time for break", APP_ICON_LOC, 3);

					pomo_data.stat_today.focusTime += pomo_data.options.time_constants[FOCUS_TIMER] - pomo_data.timerConstraints.savedTime;
				} else {
					Notify(APP_TITLE, "Time to Focus", APP_ICON_LOC, 6);

					pomo_data.stat_today.breakTime += pomo_data.options.time_constants[pomo_data.appState >> 1] - pomo_data.timerConstraints.savedTime;
				}

				if (pomo_data.options.focusCount >= pomo_data.options.FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK) {
					pomo_data.options.focusCount = 0;
					pomo_data.appState = STATE_LONG_BREAK_PAUSED;
				} else {
					pomo_data.appState = pomo_data.appState == STATE_FOCUS? STATE_SHORT_BREAK_PAUSED: STATE_FOCUS_PAUSED;
				}
				pomo_data.timerConstraints.timer = pomo_data.options.time_constants[pomo_data.appState >> 1];
				pomo_data.timerConstraints.savedTime = 0.0f;

				g_playSound = true;
				playSoundCount = 0;
			} else {
				pomo_data.timerConstraints.timer -= GetFrameTime();
			}
		}

		// On space, start/stop switch and stop any sound playing iff pressed:
		if (IsKeyPressed(KEY_SPACE) && !pomo_data.tasks.isEditing) {
			Start_Stop_Pressed(&pomo_data);
			pomo_data.tasks.isEditing = false;
		}

		if (g_playSound) {
			int soundToPlay = pomo_data.appState == STATE_FOCUS_PAUSED? SOUND_BREAK: SOUND_FOCUS;
			if (!IsSoundPlaying(pomo_data.sounds.sounds[soundToPlay])) {
				PlaySound(pomo_data.sounds.sounds[soundToPlay]);
				if (pomo_data.sounds.count[soundToPlay] != -1) {
					playSoundCount++;
				}
				if (playSoundCount >= pomo_data.sounds.count[soundToPlay] && playSoundCount != 0) {
					g_playSound = false;
				}
			}
		}

		float *timer = &pomo_data.timerConstraints.timer;
		struct String *str = &pomo_data.timerConstraints.timerStr;
		snprintf(str->chars, STR_BUFFER_CAPACITY, "%02d:%02d", (int)*timer / 60, (int)*timer % 60);
		str->length = strlen(str->chars);

		BeginDrawing();
			ClearBackground(CLAY_COLOR_TO_RAYLIB_COLOR(pomo_data.colors[COLOR_BACKGROUND_INACTIVE]));
			Clay_RenderCommandArray renderCommands = ClayCreateLayout(&pomo_data);
			Clay_Raylib_Render(renderCommands, fonts);

			float throughLineHeight = getFontHelper((&pomo_data), BORDER_WIDTH * 2);
			throughLineHeight = (throughLineHeight > 1.0f) ? throughLineHeight: 1.0f;
			for (int32_t j = 0; j < renderCommands.length; j++) {
				Clay_RenderCommand *renderCommand = Clay_RenderCommandArray_Get(&renderCommands, j);
				if (renderCommand->commandType == CLAY_RENDER_COMMAND_TYPE_TEXT) {
					if (renderCommand->userData == (void*) TASK_COMPLETED) {
						Clay_BoundingBox tasks_bounding_box = Clay_GetElementData(Clay_GetElementId(CLAY_STRING("Tasks"))).boundingBox;
						if (renderCommand->boundingBox.y + renderCommand->boundingBox.height <= tasks_bounding_box.y + 1.0f) continue;
						if (renderCommand->boundingBox.y >= (tasks_bounding_box.y + tasks_bounding_box.height - 1.0f)) continue;

						Clay_BoundingBox boundingBox = {renderCommand->boundingBox.x, renderCommand->boundingBox.y, renderCommand->boundingBox.width, renderCommand->boundingBox.height};
						Clay_TextRenderData *textData = &renderCommand->renderData.text;
						DrawRectangleRec((Rectangle) {
							.x = boundingBox.x,
							.y = boundingBox.y  + (boundingBox.height - throughLineHeight) / 2.0f,
							.width = boundingBox.width,
							.height = throughLineHeight,
						}, CLAY_COLOR_TO_RAYLIB_COLOR(textData->textColor));
					} else if (pomo_data.tasks.isEditing == IS_EDITING_TASK_DESC && renderCommand->userData == (void*) TASK_EDITING) {
						if (j + 1 < renderCommands.length) {
							Clay_RenderCommand *nextCommand = Clay_RenderCommandArray_Get(&renderCommands, j + 1);
							if (nextCommand->commandType == CLAY_RENDER_COMMAND_TYPE_TEXT && nextCommand->userData == (void*)TASK_EDITING) {
								continue;
							}
						}
						Clay_BoundingBox tasks_bounding_box = Clay_GetElementData(Clay_GetElementId(CLAY_STRING("Tasks"))).boundingBox;
						if (renderCommand->boundingBox.y + renderCommand->boundingBox.height <= tasks_bounding_box.y + 1.0f) continue;
						if (renderCommand->boundingBox.y >= (tasks_bounding_box.y + tasks_bounding_box.height - 1.0f)) continue;

						Clay_BoundingBox boundingBox = {renderCommand->boundingBox.x, renderCommand->boundingBox.y, renderCommand->boundingBox.width, renderCommand->boundingBox.height};

						if (IsKeyDown(KEY_DELETE)) {
							Expected_End_Time(&pomo_data);
							DrawRectangleRec((Rectangle){.x = boundingBox.x, .y = boundingBox.y, .height = boundingBox.height, .width = boundingBox.width}, MAGENTA);
						}


						Clay_TextRenderData *textData = &renderCommand->renderData.text;
						if (((int)(GetTime() * 2.0) % 2) == 0) {
							float cursorWidth = getFontHelper((&pomo_data), BORDER_WIDTH);
							float cursorHeight = boundingBox.height * 0.8f;

							DrawRectangleRec((Rectangle) {
								.x = boundingBox.x + boundingBox.width,
								.y = boundingBox.y + (boundingBox.height - cursorHeight) / 2.0f,
								.width = cursorWidth,
								.height = cursorHeight
							}, CLAY_COLOR_TO_RAYLIB_COLOR(textData->textColor));
						}
					}
				}
			}
		EndDrawing();
	}

	for (int i = 0; i < FONT_SIZE; i++) {
		if (IsFontValid(fonts[i])) {
			UnloadFont(fonts[i]);
		}
	}

	clean_data(&pomo_data);
	if (IsImageValid(appIcon)) {
		UnloadImage(appIcon);
	}
	CloseAudioDevice();
	Clay_Raylib_Close();
	return 0;
}

void RenderButton(Clay_String txt, int BG_COLOR, Clay_Color* colors_array, int FONT_ID, int font_size, int letter_spacing) {
	float border_width = FONT_SIZE(BORDER_WIDTH);
	border_width = border_width > 1.0f ? border_width: 1.0f;

	float cornerRadius = FONT_SIZE(BORDER_RADIUS);
	cornerRadius = (cornerRadius > 1.0f) ? cornerRadius: 0.0f;

	bool flag = Clay_PointerOver(Clay_GetElementId(txt));
	CLAY(CLAY_SID(txt)) {
		Clay_Color bg_color = colors_array[BG_COLOR];
		if (BG_COLOR != COLOR_BACKGROUND_FSL_BUTTON_SELECTED && Clay_Hovered()) {
			bg_color = onHover(bg_color);
		}

		CLAY_AUTO_ID({
			.backgroundColor = bg_color,
			.border = {
				.color = colors_array[COLOR_BORDER],
				.width = CLAY_BORDER_OUTSIDE(border_width)
			},
			.layout = {
				.padding = CLAY_PADDING_ALL(FONT_SIZE(BUTTON_PADDING)),
				.sizing = {
					.width = CLAY_SIZING_FIT(0),
					.height = CLAY_SIZING_FIT(0)
				}
			},
			.cornerRadius = CLAY_CORNER_RADIUS(cornerRadius),
		}) {
			CLAY_TEXT(txt, CLAY_TEXT_CONFIG({
				.fontId = FONT_ID,
				.letterSpacing = letter_spacing,
				.fontSize = (BG_COLOR != COLOR_BACKGROUND_FSL_BUTTON_SELECTED && flag) ? font_size * 1.04f: font_size,
				.textColor = colors_array[COLOR_TXT],
				.wrapMode = CLAY_TEXT_WRAP_WORDS,
				.textAlignment = CLAY_TEXT_ALIGN_CENTER
			}));
		}
	}
}

void RenderTask(PomodoroData *data, int taskIndex, const int WIDTH, int FONT_ID, int font_size, int letter_spacing) {
	struct Task *task = &data->tasks.tasks[taskIndex];
	Clay_LayoutConfig layout = {
		.layoutDirection = CLAY_LEFT_TO_RIGHT,
		.sizing = {
			.width = CLAY_SIZING_FIXED(WIDTH),
			.height = CLAY_SIZING_FIT(0)
		},
		.padding = {
			.top = getFontHelper(data, BASE_FONT_SIZE),
			.bottom = getFontHelper(data, BASE_FONT_SIZE),
			.left = 0,
			.right = 0
		},
		.childGap = getFontHelper(data, BUTTON_PADDING)
	};

	float borderWidth = getFontHelper(data, BORDER_WIDTH * 2);
	borderWidth = (borderWidth > 1.0f) ? borderWidth: 1.0f;

	Clay_BorderElementConfig eborder = {
		.color = data->colors[COLOR_BORDER],
		.width = CLAY_BORDER_OUTSIDE(borderWidth)
	};
	Clay_CornerRadius ecornerRadius = CLAY_CORNER_RADIUS(getFontHelper(data, BORDER_WIDTH * 4));
	Clay_Padding epadding = CLAY_PADDING_ALL(font_size / 2); // TODO: Clay doesn't handle left right padding well, find alternative
	Clay_BorderElementConfig _border = {};
	Clay_CornerRadius _cornerRadius = {};
	Clay_Padding _padding = {};

	Clay_Color text_ColorNotDefined = (Clay_Color) {
		.r = data->colors[COLOR_TXT].r,
		.g = data->colors[COLOR_TXT].g,
		.b = data->colors[COLOR_TXT].b,
		.a = data->colors[COLOR_TXT].a / 2.0f,
	};

	if (!task->__descDefined__ && (data->tasks.isEditing != IS_EDITING_TASK_DESC || data->tasks.currentSelectedTask != taskIndex)) {
		snprintf(task->desc.chars, STR_BUFFER_CAPACITY, "%s", "Task Description");
		task->desc.length = strlen(task->desc.chars);
	}

	if (!task->__countExpectedDefined__ && (data->tasks.isEditing != IS_EDITING_TASK_COUNT_EXPECTED || data->tasks.currentSelectedTask != taskIndex)) {
		snprintf(task->count_expected.chars, STR_BUFFER_CAPACITY, "%d / 0", task->count);
		task->count_expected.length = strlen(task->count_expected.chars);
	}

	Clay_String desc = STRING_TO_CLAY_STRING(task->desc);
	Clay_String count_expected = STRING_TO_CLAY_STRING(task->count_expected);

	CLAY_AUTO_ID({
		.layout = layout,
		.backgroundColor = APP_PAUSED(data->appState)? taskIndex == data->tasks.currentSelectedTask? data->colors[COLOR_BACKGROUND_SELECTED_TASK]: data->colors[COLOR_BACKGROUND_TASK]: data->colors[COLOR_BACKGROUND_MAIN_ACTIVE]
	}) {
		if (Clay_Hovered()) {
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				if (data->options.onTaskSwitchResetTimer && !(data->tasks.currentSelectedTask == taskIndex)) {
					ResetTimer(data);
				}
				if (taskIndex != data->tasks.currentSelectedTask) {
					Select_Task(&data->tasks, taskIndex, data->options.selectedTaskOnTop);
					PlaySound(data->sounds.sounds[SOUND_CLICK]);
				}
				Clay_UpdateScrollContainers(true, (Clay_Vector2) { .x = 0.0f, .y = 0.0f }, GetFrameTime());
			}
		}

		if (taskIndex == data->tasks.currentSelectedTask && APP_PAUSED(data->appState)) {
			CLAY_AUTO_ID({
				.layout = {
					.sizing = {
						.width = CLAY_SIZING_FIXED(getFontHelper(data, BUTTON_PADDING)),
						.height = CLAY_SIZING_FIXED(getFontHelper(data, HEADER5)),
					},
					.childAlignment = {
						.x = CLAY_ALIGN_X_CENTER,
						.y = CLAY_ALIGN_Y_TOP
					},
					.padding = epadding
				},
				.backgroundColor =  data->colors[COLOR_SELECTED_TASK_INDICATOR],
			});

			if (task->__descDefined__) {
				CLAY_AUTO_ID({
					.layout = {
						.sizing = {
							.width = CLAY_SIZING_FIXED(getFontHelper(data, HEADER5)),
							.height = CLAY_SIZING_FIXED(getFontHelper(data, HEADER5)),
						},
						.padding = epadding
					},
					.backgroundColor = task->__completed__? // TODO: define better colors
						Clay_Hovered()? onHover(RAYLIB_COLOR_TO_CLAY_COLOR(WHITE)): RAYLIB_COLOR_TO_CLAY_COLOR(RED):
						Clay_Hovered()? onHover(RAYLIB_COLOR_TO_CLAY_COLOR(RED)): RAYLIB_COLOR_TO_CLAY_COLOR(WHITE),
					.border = data->tasks.currentSelectedTask == taskIndex? eborder: _border,
				}) {
					if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
						Tasks *tasks = &data->tasks;
						task->__completed__ = !task->__completed__;
						if (task->__completed__) {
							struct Task tsk = *task;
							for (int i = tasks->currentSelectedTask; i < tasks->tasks_count - 1; i++) {
								tasks->tasks[i] = tasks->tasks[i + 1];
							}
							tasks->tasks[tasks->tasks_count - 1] = tsk;
							data->tasks.isEditing = IS_NOT_EDITING;
						}
						if (tasks->tasks[tasks->currentSelectedTask].__completed__) {
							tasks->currentSelectedTask = -1;
						}
					}
				}
			}

			CLAY_AUTO_ID({
				.layout = {
					.sizing = {
						.width = CLAY_SIZING_FIT(0),
						.height = CLAY_SIZING_FIT(0)
					},
					.padding = data->tasks.isEditing == IS_EDITING_TASK_DESC? epadding: _padding,
				},
				.border = data->tasks.isEditing == IS_EDITING_TASK_DESC? eborder: _border,
				.cornerRadius = data->tasks.isEditing == IS_EDITING_TASK_DESC? ecornerRadius: _cornerRadius,
			}) {
				CLAY_TEXT(desc, {
					.fontSize = font_size,
					.fontId = FONT_ID,
					.textColor = task->__descDefined__? data->colors[COLOR_TXT]: text_ColorNotDefined,
					.letterSpacing = letter_spacing,
					.wrapMode = CLAY_TEXT_WRAP_WORDS,
					.userData = task->__completed__ ?
							(void*)TASK_COMPLETED :
							data->tasks.isEditing ? (void*)TASK_EDITING:
							(void*)TASK_INCOMPLETE
				});
				if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
					data->tasks.isEditing = IS_EDITING_TASK_DESC;
				}
			}
		} else {
			CLAY_AUTO_ID({
				.layout = {
					.sizing = {
						.width = CLAY_SIZING_FIT(0),
						.height = CLAY_SIZING_FIT(0)
					},
					.childAlignment = {
						.x = CLAY_ALIGN_X_LEFT,
						.y = CLAY_ALIGN_Y_TOP
					},
				}
			}) {
				CLAY_TEXT(desc, {
					.fontSize = font_size,
					.fontId = FONT_ID,
					.textColor = task->__descDefined__? data->colors[COLOR_TXT]: text_ColorNotDefined,
					.letterSpacing = letter_spacing,
					.wrapMode = CLAY_TEXT_WRAP_WORDS,
					.textAlignment = CLAY_TEXT_ALIGN_LEFT,
					.userData = task->__completed__ ? (void*)TASK_COMPLETED : (void*)TASK_INCOMPLETE
				});
			}
		}

		CLAY_AUTO_ID({
			.layout = {
				.sizing = {
					.width = CLAY_SIZING_GROW(0),
					.height = CLAY_SIZING_FIT(0)
				},
				.childAlignment = {
					.x = CLAY_ALIGN_X_RIGHT,
					.y = CLAY_ALIGN_Y_TOP
				}
			}
		}) {
			CLAY_AUTO_ID({
				.layout = {
					.sizing = {
						.width = CLAY_SIZING_FIT(0),
						.height = CLAY_SIZING_FIT(0)
					},
					.padding = data->tasks.isEditing == IS_EDITING_TASK_COUNT_EXPECTED && taskIndex == data->tasks.currentSelectedTask? epadding: _padding,
				},
				.border = data->tasks.isEditing == IS_EDITING_TASK_COUNT_EXPECTED && taskIndex == data->tasks.currentSelectedTask? eborder: _border,
				.cornerRadius = data->tasks.isEditing == IS_EDITING_TASK_COUNT_EXPECTED && taskIndex == data->tasks.currentSelectedTask? ecornerRadius: _cornerRadius,
			}) {
				CLAY_TEXT(count_expected, {
					.fontSize = font_size,
					.fontId = FONT_ID,
					.textColor = task->__countExpectedDefined__? data->colors[COLOR_TXT]: text_ColorNotDefined,
					.letterSpacing = letter_spacing,
					.wrapMode = CLAY_TEXT_WRAP_WORDS,
					.textAlignment = CLAY_TEXT_ALIGN_RIGHT,
					.userData = task->__completed__ ? (void*)TASK_COMPLETED : (void*)TASK_INCOMPLETE
				});

				if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
					data->tasks.isEditing = IS_EDITING_TASK_COUNT_EXPECTED;
				}
			}
		}
	}
}

static int handle_String_edits(struct String *selectedString, Options* options) {
	if (selectedString == NULL) return -1;
	static float keyTimer = 0.0f;
	static int lastKeyPressed = 0;
	static char lastCharPressed = '\0';

	// handle char input
	int character = GetCharPressed();
	while (character > 0) {
		if (character >= 32 && character <= 125) {
			Add_Char_To_String(selectedString, (char)character);
			lastCharPressed = (char)character;
			lastKeyPressed = 0;  // Reset key repeat when typing a character
			keyTimer = 0.0f;
		}
		character = GetCharPressed();
	}

	// handle action keys
	int key = GetKeyPressed();
	while (key > 0) {
		lastKeyPressed = key;
		keyTimer = 0.0f;
		lastCharPressed = '\0';  // Reset char when pressing action keys
		switch (key) {
			case KEY_ENTER:
			return 1;
			case KEY_ESCAPE:
			return 2;
			case KEY_TAB:
			return 3;
			case KEY_BACKSPACE:
			Remove_Char_From_String(selectedString);
			break;
		}
		key = GetKeyPressed();
	}

	// key repeat logic
	if (lastKeyPressed > 0 && IsKeyDown(lastKeyPressed)) {
		keyTimer += GetFrameTime();
		if (keyTimer >= (options->REPEAT_DELAY + options->REPEAT_INTERVAL)) {
			if (lastKeyPressed == KEY_BACKSPACE) {
				Remove_Char_From_String(selectedString);
			} else if (lastCharPressed != '\0') {
				Add_Char_To_String(selectedString, lastCharPressed);
			}
			keyTimer -= options->REPEAT_INTERVAL;  // Subtract instead of reset for smoother repeat
		}
	} else {
		keyTimer = 0.0f;
		lastKeyPressed = 0;
		lastCharPressed = '\0';
	}

	return 0;
}

static void HANDLE_EDITS_TO_TASK(PomodoroData* data) {
	if (data->tasks.currentSelectedTask == -1 || data->tasks.isEditing == IS_NOT_EDITING) return;

	static char str_buffer[STR_BUFFER_CAPACITY];
	static int str_buffer_length = 0;
	static struct String* SelectedString = NULL;
	static int task_id = -1;
	static enum EDITING_STATUS EDITING_STATE = IS_NOT_EDITING;

	if (task_id != data->tasks.tasks[data->tasks.currentSelectedTask].id || EDITING_STATE != data->tasks.isEditing) {
		// On click after editing, save whatever was typed
		if (EDITING_STATE != IS_NOT_EDITING) {
			SelectedString = EDITING_STATE == IS_EDITING_TASK_DESC? &data->tasks.tasks[data->tasks.currentSelectedTask].desc: &data->tasks.tasks[data->tasks.currentSelectedTask].count_expected;
			struct Task *task = &data->tasks.tasks[data->tasks.currentSelectedTask];

			sscanf(SelectedString->chars, "%d", &task->expected);
			snprintf(task->count_expected.chars, STR_BUFFER_CAPACITY, "%d / %d", task->count, task->expected);
			task->count_expected.length = strlen(task->count_expected.chars);
			SelectedString->chars[SelectedString->length] = '\0';
		}

		str_buffer_length = 0;
		str_buffer[str_buffer_length] = '\0';
		task_id = data->tasks.tasks[data->tasks.currentSelectedTask].id;
		EDITING_STATE = data->tasks.isEditing;

		data->tasks.tasks[data->tasks.currentSelectedTask].count_expected.length = 0;

		SelectedString = EDITING_STATE == IS_EDITING_TASK_DESC? &data->tasks.tasks[data->tasks.currentSelectedTask].desc: &data->tasks.tasks[data->tasks.currentSelectedTask].count_expected;

		if (data->tasks.tasks[data->tasks.currentSelectedTask].__descDefined__ && EDITING_STATE == IS_EDITING_TASK_DESC) {
			snprintf(str_buffer, STR_BUFFER_CAPACITY, "%s", SelectedString->chars);
			str_buffer_length = strlen(str_buffer);
		} else {
			SelectedString->length = 0;
		}
	}

	if (SelectedString == NULL) return;

	struct Task *task = &data->tasks.tasks[data->tasks.currentSelectedTask];
	switch (handle_String_edits(SelectedString, &data->options)) {
		case 0: break;

		case 1: {
			if (data->tasks.isEditing == IS_EDITING_TASK_COUNT_EXPECTED) {
				sscanf(SelectedString->chars, "%d", &task->expected);
				snprintf(task->count_expected.chars, STR_BUFFER_CAPACITY, "%d / %d", task->count, task->expected);
				task->count_expected.length = strlen(task->count_expected.chars);
			}
			SelectedString->chars[SelectedString->length] = '\0';
			task_id = -1;
			SelectedString = NULL;

			data->tasks.isEditing = IS_NOT_EDITING;
			break;
		}

		case -1: case 2: {
			snprintf(SelectedString->chars, STR_BUFFER_CAPACITY, "%s", str_buffer);
			SelectedString->length = strlen(SelectedString->chars);
			data->tasks.isEditing = IS_NOT_EDITING;
			task_id = -1;

			SelectedString = NULL;
			break;
		}

		case 3: {
			if (data->tasks.isEditing == IS_EDITING_TASK_COUNT_EXPECTED) {
				sscanf(SelectedString->chars, "%d", &task->expected);
				snprintf(task->count_expected.chars, STR_BUFFER_CAPACITY, "%d / %d", task->count, task->expected);
				task->count_expected.length = strlen(task->count_expected.chars);
			}
			SelectedString->chars[SelectedString->length] = '\0';
			task_id = -1;
			SelectedString = NULL;

			data->tasks.isEditing = data->tasks.isEditing == IS_EDITING_TASK_COUNT_EXPECTED? IS_EDITING_TASK_DESC: IS_EDITING_TASK_COUNT_EXPECTED;
			break;
		}
	}

	task->__countExpectedDefined__ = task->expected > 0;
	task->__descDefined__ = task->desc.length > 0;
}

static void HANDLE_EVENTS(uint32_t* totalMemorySize, Clay_Arena* clayMemory, PomodoroData* data) {
	// CLAY SPECIFIC EVENTS:
	Clay_SetLayoutDimensions((Clay_Dimensions) {.width = GetScreenWidth(), .height = GetScreenHeight()});

	Vector2 mousePosition = GetMousePosition();
	Vector2 scrollDelta = GetMouseWheelMoveV();

	Clay_UpdateScrollContainers(true, (Clay_Vector2) { scrollDelta.x * data->options.SCROLL_MULTIPLIER, scrollDelta.y * data->options.SCROLL_MULTIPLIER }, GetFrameTime());

	Clay_SetPointerState((Clay_Vector2) { mousePosition.x, mousePosition.y }, IsMouseButtonDown(0));

	if (g_reinit_clay) {
		*totalMemorySize = Clay_MinMemorySize();
		*clayMemory = Clay_CreateArenaWithCapacityAndMemory(*totalMemorySize, malloc(*totalMemorySize));
		Clay_Initialize(*clayMemory, (Clay_Dimensions) { (float)GetScreenWidth(), (float)GetScreenHeight() }, (Clay_ErrorHandler) { HandleClayErrors, 0 });
		g_reinit_clay = false;
	}

	// UI
	if (GetScreenWidth() >= DEVICE_SMART_PHONE_W && GetScreenHeight() >= DEVICE_SMART_PHONE_W * 1.5f) {
		data->device = DEVICE_SMART_PHONE;
	} else {
		data->device = DEVICE_WATCH;
	}

	// Key presses
	HANDLE_EDITS_TO_TASK(data);

	#ifdef DEBUG
	if (data->tasks.isEditing == IS_NOT_EDITING) {
		if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
			if (IsKeyPressed(KEY_R)) {
				for (int i = 0; i < COLOR_SIZE; i++) {
					INVERT_CLAY_COLOR(data->colors[i]);
				}
			}

			if (IsKeyDown(KEY_KP_ADD) || IsKeyDown(KEY_EQUAL)) {
				g_UI_SCALE += 0.1f;
			} else if (IsKeyDown(KEY_MINUS)) {
				g_UI_SCALE -= 0.1f;
			} else {
				Vector2 scrollDelta = GetMouseWheelMoveV();
				g_UI_SCALE += scrollDelta.y;
			}

			if (IsKeyDown(KEY_ZERO)) {
				g_UI_SCALE = 2.0f;
			}

			if (g_UI_SCALE < 0.5f) {
				g_UI_SCALE = 0.8f;
			}
		}
	}
	#endif

	if (IsMouseButtonUp(MOUSE_LEFT_BUTTON)) {
		data->tasks.isDragging = false;
	}

	SetMasterVolume(data->options.masterVolume);
}

void Device_Smart_Watch(PomodoroData* data) {
	// Good enough
	Clay_String str_Start_STOP = {.chars = !APP_PAUSED(data->appState) ? "  STOP  ": "  Start  ", .isStaticallyAllocated = false};
	str_Start_STOP.length = strlen(str_Start_STOP.chars);
	Clay_String Focus_ShortBreak_LongBreak = { .isStaticallyAllocated = false };

	switch (data->appState) {
		case STATE_FOCUS_PAUSED: {
			Focus_ShortBreak_LongBreak.chars = "  Focus  ";
			Focus_ShortBreak_LongBreak.length = strlen(Focus_ShortBreak_LongBreak.chars);
			break;
		}

		case STATE_SHORT_BREAK_PAUSED: {
			Focus_ShortBreak_LongBreak.chars = "  Short Break  ";
			Focus_ShortBreak_LongBreak.length = strlen(Focus_ShortBreak_LongBreak.chars);
			break;
		}

		case STATE_LONG_BREAK_PAUSED: {
			Focus_ShortBreak_LongBreak.chars = "  Long Break  ";
			Focus_ShortBreak_LongBreak.length = strlen(Focus_ShortBreak_LongBreak.chars);
			break;
		}

		default: {
			break;
		}
	}

	CLAY_AUTO_ID({
		.layout  = {
			.sizing = {
				.width = CLAY_SIZING_GROW(),
				.height = CLAY_SIZING_GROW()
			},
			.childAlignment = {
				.x = CLAY_ALIGN_X_CENTER,
				.y = CLAY_ALIGN_Y_CENTER
			},
		},
		.backgroundColor = data->colors[!APP_PAUSED(data->appState)? COLOR_BACKGROUND_MAIN_ACTIVE: COLOR_BACKGROUND_MAIN_INACTIVE],
	}) {
		CLAY(CLAY_ID("Smart Watch"), {
			.layout  = {
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
				.childAlignment = {
					.x = CLAY_ALIGN_X_CENTER,
					.y = CLAY_ALIGN_Y_CENTER
				},
			},
		}) {
			if (APP_PAUSED(data->appState)) {
				RenderButton(Focus_ShortBreak_LongBreak, COLOR_BACKGROUND_FSL_BUTTON, data->colors, FONT_ID_16_PX, FONT_SIZE(HEADER6), getFontHelper(data, LETTER_SPACING));
			}

			CLAY_AUTO_ID({
				.layout  = {
					.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.sizing = {
						.width = CLAY_SIZING_GROW(),
						.height = CLAY_SIZING_GROW()
					},
					.childAlignment = {
						.x = CLAY_ALIGN_X_CENTER,
						.y = CLAY_ALIGN_Y_CENTER
					},
				},
			}) {
				CLAY_AUTO_ID({
					.layout  = {
						.layoutDirection = CLAY_TOP_TO_BOTTOM,
						.sizing = {
							.width = CLAY_SIZING_FIT(),
							.height = CLAY_SIZING_FIT()
						},
						.childAlignment = {
							.x = CLAY_ALIGN_X_CENTER,
							.y = CLAY_ALIGN_Y_CENTER
						},
					},
				}){
					Clay_String str = {
						.chars = data->timerConstraints.timerStr.chars,
						.length = data->timerConstraints.timerStr.length,
						.isStaticallyAllocated = true
					};

					int font_size = !APP_PAUSED(data->appState)? HEADER1: HEADER2;
					int font_id = !APP_PAUSED(data->appState)? FONT_ID_32_PX: FONT_ID_16_PX;
					int letterSpacing = LETTER_SPACING;

					CLAY_TEXT(str, {
						.fontSize = FONT_SIZE(font_size),
						.fontId = font_id,
						.letterSpacing = FONT_SIZE(letterSpacing),
						.textColor = data->colors[COLOR_TXT],
						.textAlignment = CLAY_TEXT_ALIGN_CENTER
					});
				}

				CLAY_AUTO_ID({ .layout = { .sizing = { .height = CLAY_SIZING_PERCENT(0.1f) }}}) {}
				RenderButton(str_Start_STOP, !APP_PAUSED(data->appState)? COLOR_BACKGROUND_SS_BUTTON_SELECTED: COLOR_BACKGROUND_SS_BUTTON, data->colors, FONT_ID_16_PX, !APP_PAUSED(data->appState)? FONT_SIZE(HEADER4): FONT_SIZE(HEADER6), getFontHelper(data, LETTER_SPACING));
			}
		}
	}

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(str_Start_STOP))) {
		Start_Stop_Pressed(data);
		data->tasks.isEditing = false;
	}

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(Focus_ShortBreak_LongBreak))) {
		data->appState = data->appState == STATE_LONG_BREAK_PAUSED? STATE_FOCUS_PAUSED: data->appState + 2;
		data->timerConstraints.timer = data->options.time_constants[data->appState >> 1];
		data->tasks.isEditing = false;
	}
}

void Device_Smart_Phone(PomodoroData* data) {
	Clay_String str_Start_STOP = {.chars = !APP_PAUSED(data->appState) ? "  STOP  ": "  Start  ", .isStaticallyAllocated = false};
	str_Start_STOP.length = strlen(str_Start_STOP.chars);
	bool longerTxtOnButtonCondition = false; // TODO Priority!!!

	Clay_String app_title = {.chars = longerTxtOnButtonCondition ? APP_TITLE : "PomoFocus", .isStaticallyAllocated = false};
	app_title.length = strlen(app_title.chars);

	Clay_String str_Focus = {.chars = longerTxtOnButtonCondition ? "  Focus  ": " Focus ", .isStaticallyAllocated = false};
	str_Focus.length = strlen(str_Focus.chars);

	Clay_String str_Short_Break = {.chars = longerTxtOnButtonCondition ? "  Short Break  ": " Short ", .isStaticallyAllocated = false};
	str_Short_Break.length = strlen(str_Short_Break.chars);

	Clay_String str_Long_Break = {.chars = longerTxtOnButtonCondition ? "  Long Break  ": " Long ", .isStaticallyAllocated = false};
	str_Long_Break.length = strlen(str_Long_Break.chars);

	Tasks tasks = data->tasks;
	CLAY_AUTO_ID({
		.layout = {
			.sizing = {
				.width = CLAY_SIZING_GROW(0),
				.height = CLAY_SIZING_GROW(0),
			},
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
			.childAlignment = {
				.x = CLAY_ALIGN_X_CENTER,
				.y = CLAY_ALIGN_Y_CENTER
			},
			.padding = {
				.top = getFontHelper(data, HEADER1),
				.bottom = getFontHelper(data, HEADER1),
			},
			.childGap = getFontHelper(data, PARAGRAPH)
		},
		.backgroundColor = data->colors[!APP_PAUSED(data->appState)? COLOR_BACKGROUND_ACTIVE: COLOR_BACKGROUND_INACTIVE],
	}) {
		float WIDTH = 0.0f;
		if (!APP_PAUSED(data->appState)) {
			WIDTH = Clay_GetElementData(CLAY_ID("Timer")).boundingBox.width;
		} else {
			WIDTH = Clay_GetElementData(CLAY_ID("Buttons")).boundingBox.width + getFontHelper(data, HEADER2) * 2;
		}
		float borderWidth = getFontHelper(data, BORDER_WIDTH * 2);
		borderWidth = (borderWidth > 1.0f) ? borderWidth: 1.0f;

		if (APP_PAUSED(data->appState)) {
			CLAY(CLAY_ID("Title"), {
				.layout = {
					.sizing = {
						.width = CLAY_SIZING_FIXED(WIDTH),
						.height = CLAY_SIZING_FIT(0)
					},
					.layoutDirection = CLAY_LEFT_TO_RIGHT,
					.padding = CLAY_PADDING_ALL(getFontHelper(data, BUTTON_PADDING)),
					.childAlignment = {
						.x = CLAY_ALIGN_X_CENTER,
						.y = CLAY_ALIGN_Y_BOTTOM,
					}
				},
				.border = {
					.width = {
						.bottom = borderWidth
					},
					.color = data->colors[COLOR_BORDER]
				},
			}){
				CLAY_TEXT(app_title, {
					.fontSize = getFontHelper(data, HEADER1),
					.fontId = FONT_ID_32_PX,
					.letterSpacing = FONT_SIZE(LETTER_SPACING),
					.textColor = data->colors[COLOR_TXT],
					.textAlignment = CLAY_TEXT_ALIGN_CENTER
				});

				// TODO: Settings Button
			}
		}
		CLAY_AUTO_ID({
			.layout  = {
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
				.sizing = {
					.width = CLAY_SIZING_FIT(0),
					.height = CLAY_SIZING_FIT(0)
				},
				.childAlignment = {
					.x = CLAY_ALIGN_X_CENTER,
					.y = CLAY_ALIGN_Y_CENTER
				},
				.padding = {
					.left = getFontHelper(data, HEADER2),
					.right = getFontHelper(data, HEADER2),
					.top = getFontHelper(data, HEADER2),
					.bottom = getFontHelper(data, HEADER1)
				},
				.childGap = getFontHelper(data, HEADER2)
			},
			.border = {
				.width = CLAY_BORDER_OUTSIDE(borderWidth),
				.color = data->colors[COLOR_BORDER]
			},
			.cornerRadius = CLAY_CORNER_RADIUS(getFontHelper(data, BORDER_WIDTH * 4)),
			.backgroundColor = !APP_PAUSED(data->appState)? data->colors[COLOR_BACKGROUND_MAIN_ACTIVE]: data->colors[COLOR_BACKGROUND_MAIN_INACTIVE]
		}){
			if (!APP_PAUSED(data->appState)) {
				if (tasks.currentSelectedTask >= 0 && tasks.currentSelectedTask < tasks.tasks_count && data->appState == STATE_FOCUS && !data->tasks.tasks[data->tasks.currentSelectedTask].__completed__ && data->tasks.tasks[data->tasks.currentSelectedTask].__descDefined__) {
					CLAY_AUTO_ID({
						.clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
						.layout = {
							.sizing = {
								.width = CLAY_SIZING_FIXED(WIDTH),
								.height = CLAY_SIZING_FIT(0, getFontHelper(data, HEADER3 * 3)),
							},
							.layoutDirection = CLAY_TOP_TO_BOTTOM,
							.childGap = getFontHelper(data, PARAGRAPH)
						},
						.border = { .width = { .bottom =  borderWidth }, .color = data->colors[COLOR_BORDER] },
					}) {
						RenderTask(data, tasks.currentSelectedTask, WIDTH, FONT_ID_16_PX, getFontHelper(data, HEADER3), getFontHelper(data, LETTER_SPACING));
					}
				}
			} else {
				CLAY(CLAY_ID("Buttons"), {
					.layout  = {
						.layoutDirection = CLAY_LEFT_TO_RIGHT,
						.sizing = {
							.width = CLAY_SIZING_FIT(0),
							.height = CLAY_SIZING_FIT(0)
						},
						.padding = CLAY_PADDING_ALL(getFontHelper(data, BUTTON_PADDING * 2)),
						.childGap = getFontHelper(data, BUTTON_PADDING * 2)
					},
					.border = {
						.width = CLAY_BORDER_OUTSIDE(borderWidth),
						.color = data->colors[COLOR_BORDER]
					},
					.cornerRadius = CLAY_CORNER_RADIUS(getFontHelper(data, BORDER_RADIUS * 2))
				}){
					RenderButton(str_Focus, data->appState == STATE_FOCUS_PAUSED? COLOR_BACKGROUND_FSL_BUTTON_SELECTED: COLOR_BACKGROUND_FSL_BUTTON, data->colors, FONT_ID_16_PX, getFontHelper(data, PARAGRAPH), getFontHelper(data, LETTER_SPACING));
					RenderButton(str_Short_Break, data->appState == STATE_SHORT_BREAK_PAUSED? COLOR_BACKGROUND_FSL_BUTTON_SELECTED: COLOR_BACKGROUND_FSL_BUTTON, data->colors, FONT_ID_16_PX, getFontHelper(data, PARAGRAPH), getFontHelper(data, LETTER_SPACING));
					RenderButton(str_Long_Break, data->appState == STATE_LONG_BREAK_PAUSED? COLOR_BACKGROUND_FSL_BUTTON_SELECTED: COLOR_BACKGROUND_FSL_BUTTON, data->colors, FONT_ID_16_PX, getFontHelper(data, PARAGRAPH), getFontHelper(data, LETTER_SPACING));
				}
			}

			CLAY(CLAY_ID("Timer"), {
				.layout = {
					.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.sizing = {
						.width = CLAY_SIZING_FIT(0),
						.height = CLAY_SIZING_GROW(0)
					},
					.childAlignment = {
						.x = CLAY_ALIGN_X_CENTER,
						.y = CLAY_ALIGN_Y_CENTER
					},
					.childGap = getFontHelper(data, HEADER3)
				}
			}) {
				Clay_String str = {
					.chars = data->timerConstraints.timerStr.chars,
					.length = data->timerConstraints.timerStr.length,
					.isStaticallyAllocated = true
				};

				int font_size = !APP_PAUSED(data->appState)? HEADER1 * 4: HEADER1 * 2;
				int font_id = FONT_ID_64_PX;
				int letterSpacing = LETTER_SPACING;

				CLAY_TEXT(str, {
					.fontSize = getFontHelper(data, font_size),
					.fontId = font_id,
					.letterSpacing = getFontHelper(data, letterSpacing),
					.textColor = data->colors[COLOR_TXT],
					.textAlignment = CLAY_TEXT_ALIGN_CENTER
				});

				int fontSize = fmax(!APP_PAUSED(data->appState)? FONT_SIZE(HEADER4): FONT_SIZE(HEADER6), !APP_PAUSED(data->appState)? getFontHelper(data, HEADER3): getFontHelper(data, HEADER2));
				RenderButton(
					str_Start_STOP,
					!APP_PAUSED(data->appState)? COLOR_BACKGROUND_SS_BUTTON_SELECTED: COLOR_BACKGROUND_SS_BUTTON,
					data->colors,
					!APP_PAUSED(data->appState)? FONT_ID_16_PX: FONT_ID_32_PX,
					fontSize,
					getFontHelper(data, LETTER_SPACING)
				);
			}
		}

		if (APP_PAUSED(data->appState)) {
			CLAY_AUTO_ID({
				.layout = {
					.layoutDirection = CLAY_LEFT_TO_RIGHT,
					.childGap = getFontHelper(data, BUTTON_PADDING)
				}
			}) {
				Clay_String ADD_TASK = { .chars = " +Task ", .isStaticallyAllocated = false};
				ADD_TASK.length = strlen(ADD_TASK.chars);
				RenderButton(ADD_TASK, COLOR_BACKGROUND_MAIN_ACTIVE, data->colors, FONT_ID_16_PX, getFontHelper(data, PARAGRAPH), getFontHelper(data, LETTER_SPACING));
				if (Clay_PointerOver(Clay_GetElementId(ADD_TASK)) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
					Add_Task(&data->tasks, CreateNewTask("", 0, 0), data->options.selectedTaskOnTop);
					data->tasks.isEditing = IS_EDITING_TASK_DESC;
					PlaySound(data->sounds.sounds[SOUND_CLICK]);

					if (data->options.onTaskSwitchResetTimer) data->timerConstraints.timer = data->options.time_constants[data->appState >> 1];
				}

				if (-1 != data->tasks.currentSelectedTask) {
					Clay_String REMOVE_TASK = { .chars = " x ", .isStaticallyAllocated = false};
					REMOVE_TASK.length = strlen(REMOVE_TASK.chars);

					RenderButton(REMOVE_TASK, COLOR_BACKGROUND_SS_BUTTON_SELECTED, data->colors, FONT_ID_16_PX, getFontHelper(data, PARAGRAPH), getFontHelper(data, LETTER_SPACING));
					if (Clay_PointerOver(Clay_GetElementId(REMOVE_TASK)) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
						Remove_Task(&data->tasks, data->tasks.currentSelectedTask);
						PlaySound(data->sounds.sounds[SOUND_CLICK]);
					}
				}
			}

			CLAY(CLAY_ID("Tasks"), {
				.clip = {
					.vertical = true,
					.childOffset = Clay_GetScrollOffset()
				},
				.layout  = {
					.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.sizing = {
						.width = CLAY_SIZING_FIXED(WIDTH),
						.height = CLAY_SIZING_FIT(0)
					},
					.padding = {
						.top = getFontHelper(data, LETTER_SPACING),
						.bottom = getFontHelper(data, LETTER_SPACING)
					},
					.childAlignment = {
						.x = CLAY_ALIGN_X_CENTER,
						.y = CLAY_ALIGN_Y_CENTER,
					},
					.childGap = getFontHelper(data, HEADER6)
				},
				.border = {
					.width = {
						.betweenChildren = borderWidth
					},
					.color = data->colors[COLOR_BORDER]
				},
				.backgroundColor = data->colors[COLOR_BACKGROUND_TASKS]
			}){
				for (int i = 0; i < tasks.tasks_count; ++i) {
					// if (tasks.isDragging) continue;
					RenderTask(data, i, WIDTH, FONT_ID_16_PX, getFontHelper(data, PARAGRAPH), getFontHelper(data, LETTER_SPACING));
				}
				// TODO: render dragging element and if it's border and another task's border cross more than 50% put it up of that task
			}
		}
	}

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(str_Start_STOP))) {
		Start_Stop_Pressed(data);
		data->tasks.isEditing = false;
	} else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(str_Focus))) {
		// if (data->appState != STATE_FOCUS_PAUSED) {
			data->appState = STATE_FOCUS_PAUSED;
			data->timerConstraints.timer = data->options.time_constants[data->appState >> 1];
			PlaySound(data->sounds.sounds[SOUND_CLICK]);
			data->tasks.isEditing = false;
		// }
	} else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(str_Short_Break))) {
		// if (data->appState != STATE_SHORT_BREAK_PAUSED) {
			data->appState = STATE_SHORT_BREAK_PAUSED;
			data->timerConstraints.timer = data->options.time_constants[data->appState >> 1];
			PlaySound(data->sounds.sounds[SOUND_CLICK]);
			data->tasks.isEditing = false;
		// }
	} else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(str_Long_Break))) {
		// if (data->appState != STATE_LONG_BREAK_PAUSED) {
			data->options.focusCount = 0;
			data->appState = STATE_LONG_BREAK_PAUSED;
			data->timerConstraints.timer = data->options.time_constants[data->appState >> 1];
			PlaySound(data->sounds.sounds[SOUND_CLICK]);
			data->tasks.isEditing = false;
		// }
	}
}

static Clay_RenderCommandArray ClayCreateLayout(PomodoroData* data) {
	Clay_BeginLayout();
	switch (data->device) {
		case DEVICE_WATCH: {
			Device_Smart_Watch(data);
			break;
		}

		case DEVICE_SMART_PHONE: {
			Device_Smart_Phone(data);
			break;
		}

		default: {
			Device_Smart_Phone(data);
			break;
		}
	}
	return Clay_EndLayout(GetFrameTime());
}

void ResetTimer(PomodoroData* data) {
	data->timerConstraints.timer = data->options.time_constants[data->appState >> 1];
}

void Start_Stop_Pressed(PomodoroData* pomo_data) {
	// Save Time focus/break rn
	if (!APP_PAUSED(pomo_data->appState)) {
		float initial_constant = pomo_data->options.time_constants[pomo_data->appState >> 1];
		float delta = (initial_constant - pomo_data->timerConstraints.timer) - pomo_data->timerConstraints.savedTime;
		if (delta > 0) {
			const float threshold = 5.0f; // Threshold below which no save!
			if (delta <= threshold + 1e-6f) {
				pomo_data->timerConstraints.timer += delta;
				pomo_data->timerConstraints.savedTime = 0.0f;
			} else {
				if (pomo_data->appState == STATE_FOCUS) pomo_data->stat_today.focusTime += (int)delta;
				else pomo_data->stat_today.breakTime += (int)delta;
				pomo_data->timerConstraints.savedTime += delta;
			}
		}
	}

	pomo_data->appState += pomo_data->appState % 2 == 0? 1: -1;
	g_playSound = false;
	for (int i = 0; i < SOUND_COUNT; i++) {
		if (IsSoundPlaying(pomo_data->sounds.sounds[i])) {
			StopSound(pomo_data->sounds.sounds[i]);
		}
	}
	PlaySound(pomo_data->sounds.sounds[SOUND_CLICK]);
}
