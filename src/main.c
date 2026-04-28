// TODOs:
// - [x] Modifing Tasks
	// - [ ] Add repeating keys option + CTRL + A to select all +

// - [ ] New Task
// - [ ] Add visual cue to selected task, tick for completed task, modify focus count on pomodoro counts
// - [ ] Statistics
// - [ ] On complete sound for break and focus, different
// - [ ] Save and Load System
// - [ ] Optimize to use as little memory as possible
// - [ ] Compile for release: .exe, .flatpak, .deb, nah not snap and so on, maybe use raylib's build template
// - [ ] Push to github


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
// #include "cJSON.h": Save and Load System
// Here is the truth: I could make a .dat file for saving but i want the user to be able to easily be able to transfer over to other apps if this doesn't meet their requirements

#include "raylib.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"

#include "tasks.h"
#include "stats.h"

#define FONT_SIZE(SIZE) ((SIZE * BASE_FONT_SIZE * g_UI_SCALE) / 100)
#define RAYLIB_COLOR_TO_CLAY_COLOR(color) (Clay_Color) { .r = color.r, .g = color.g, .b = color.b, .a = color.a }
#define STRING_TO_CLAY_STRING(str)  (Clay_String) {\
	.chars = str.chars,\
	.isStaticallyAllocated = true,\
	.length = str.length,\
}

#define getWidthRatio(baseWidth) (GetScreenWidth() / ((baseWidth) * 1.0f))
#define getHeightRatio(baseHeight) (GetScreenHeight() / ((baseHeight) * 1.0f))
#define getCubicRatio(baseWidth, baseHeight) fmin(getWidthRatio(baseWidth), getHeightRatio(baseHeight))
#define getFontSize(baseWidth, baseHeight, fontSize) getCubicRatio(baseWidth, baseHeight) * (fontSize)
#define getFontHelper(data, fontSize) FONT_SIZE(getFontSize(data->deviceDimensions.x, data->deviceDimensions.y, fontSize))

#define APP_TITLE "PomoFocus Timer" // Pomo means tomato btw
#define PRINT(variable) printf(#variable " = %g\n", (double) variable); fflush(stdout);
#define PRINT_S(variable) printf(#variable " = %s\n", variable); fflush(stdout);

// GLOBAL variables
bool g_reinit_clay;
bool g_debug_mode;
bool g_dark_mode;
float g_UI_SCALE;

#define BASE_FONT_SIZE 16
enum {
	HEADER1 = 200,
	HEADER2 = 150,
	HEADER3 = 117,
	HEADER4 = 100,
	HEADER5 = 83,
	HEADER6 = 67,
	PARAGRAPH = 100,

	LETTER_SPACING = BASE_FONT_SIZE / 2,
	BORDER_RADIUS = 6 * 4,
	BUTTON_PADDING = 6 * 4,
	BORDER_WIDTH = 5
};

enum {
	FONT_ID_DEFAULT,
	FONT_ID_12_PX,
	FONT_ID_16_PX,
	FONT_ID_24_PX,
	FONT_ID_32_PX,
	FONT_ID_64_PX,
	FONT_SIZE
};

enum {
	COLOR_BACKGROUND_INACTIVE = 0,
	COLOR_BACKGROUND_ACTIVE,

	COLOR_TXT_DEFAULT,

	COLOR_BUTTON_FSL_BACKGROUND, // FSB = Focus, Short break, Long break
	COLOR_BUTTON_FSL_BACKGROUND_SELECTED, // FSB = Focus, Short break, Long break

	COLOR_BUTTON_SS_BACKGROUND_INACTIVE, // SS = Start Stop
	COLOR_BUTTON_SS_BACKGROUND_ACTIVE,

	COLOR_SIZE
};

enum {
	FOCUS_TIMER,
	SHORT_BREAK_TIMER,
	LONG_BREAK_TIMER,
	TIMERS_COUNT
};

enum {
	DEVICE_WATCH_W = 240,
	DEVICE_WATCH_H = 240,

	DEVICE_SMART_PHONE_W = 340,
	DEVICE_SMART_PHONE_H = (int)(DEVICE_SMART_PHONE_W * 1.5f),
};

enum {
	SOUND_CLICK,
	SOUND_COUNT
};

typedef struct {
	Sound* sounds;
	float* count; // How many times to play sound!
} SoundData;

typedef enum {
	STATE_FOCUS,
	STATE_FOCUS_PAUSED,
	STATE_SHORT_BREAK,
	STATE_SHORT_BREAK_PAUSED,
	STATE_LONG_BREAK,
	STATE_LONG_BREAK_PAUSED,
} AppState;

typedef enum {
	DEVICE_WATCH,
	DEVICE_SMART_PHONE,
	DEVICE_TABLET,
	DEVICE_LAPTOP,
} Device;

typedef struct {
	int time_constants[TIMERS_COUNT];
	float timer;
	struct String timerStr;
} TimerConstraints;

typedef struct {
	int focusCount;
	int FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK; // Great name

	float REPEAT_DELAY; // How much time to wait to activate repeat
	float REPEAT_INTERVAL; // How much time per repeat

	float SCROLL_MULTIPLIER;
	bool rearrangeTaskOnSelect; // Moves task to top
} Options;

typedef struct {
	AppState appState;
	Device device;
	Vector2 deviceDimensions;
	TimerConstraints timerConstraints;
	Clay_Color* colors;

	Options options;
	Stats stats;

	Tasks tasks;
	SoundData sounds;
} PomodoroData;

static void HANDLE_EVENTS(uint32_t*, Clay_Arena*, PomodoroData* );
static Clay_RenderCommandArray ClayCreateLayout(PomodoroData*);
static void HandleClayErrors(Clay_ErrorData errorData) {
	printf("%s", errorData.errorText.chars);

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
			printf("Clay Error: %s\n", errorData.errorText.chars);
			break;
		}
	}
}

int initialize_data(PomodoroData* data) {
	// Global Data
	g_UI_SCALE = 2.0f;
	g_debug_mode = false;
	g_reinit_clay = false;
	g_dark_mode = false;

	// Pomodata initialization
	*data = (PomodoroData) {
		.appState = STATE_FOCUS_PAUSED,
		.deviceDimensions = {
			.x = DEVICE_SMART_PHONE_W * 2,
			.y = DEVICE_SMART_PHONE_H * 2,
		},
		.colors = NULL,
		.sounds.sounds = NULL,
		.sounds.count = NULL,
		.timerConstraints.timerStr.chars = NULL,
		.tasks = NULL,
		.options = {
			.FOCUS_COUNT_THRESHOLD_FOR_LONG_BREAK = 3,
			.focusCount = 0,
			.rearrangeTaskOnSelect = true,
			.REPEAT_DELAY = 0.30f, // 300ms
			.REPEAT_INTERVAL = 0.03f, // 30ms
			.SCROLL_MULTIPLIER = 10.0f
		},
		.stats = {
			.totalFocusCount = 0
		}
	};

	Init_Tasks(&data->tasks);
	data->timerConstraints.timerStr = (struct String) {
		.chars = malloc(sizeof(char) * 16),
		.capacity = 16,
		.length = 0
	};

	// TODO: remove later cause this is just dummy
	// NP here
	Add_Task(&data->tasks, CreateNewTask("Placeholder Task Description 5", 5, 10), data->options.rearrangeTaskOnSelect);
	Add_Task(&data->tasks, CreateNewTask("Placeholder Task Description 4", 4, 10), data->options.rearrangeTaskOnSelect);
	Add_Task(&data->tasks, CreateNewTask("Placeholder Task Description 3", 3, 10), data->options.rearrangeTaskOnSelect);
	Add_Task(&data->tasks, CreateNewTask("Placeholder Task Description 2", 2, 10), data->options.rearrangeTaskOnSelect);
	Add_Task(&data->tasks, CreateNewTask("Placeholder Task Description 1", 1, 10), data->options.rearrangeTaskOnSelect);
	// Add_Task(&data->tasks, CreateNewTask("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.", 6, 10), data->options.rearrangeTaskOnSelect);

	if (data->timerConstraints.timerStr.chars == NULL) return 1;
	data->timerConstraints.time_constants[FOCUS_TIMER] = 25 * 60;
	data->timerConstraints.time_constants[LONG_BREAK_TIMER] = 15 * 60;
	data->timerConstraints.time_constants[SHORT_BREAK_TIMER] = 5 * 60;
	data->timerConstraints.timer = data->timerConstraints.time_constants[FOCUS_TIMER];

	data->colors = malloc(sizeof(Clay_Color) * COLOR_SIZE);
	if (data->colors == NULL) return 1;
	data->colors[COLOR_BACKGROUND_INACTIVE] = (Clay_Color) {.r = 255, .g = 255, .b = 255, .a = 255};
	data->colors[COLOR_BACKGROUND_ACTIVE] = (Clay_Color) {.r = 178, .g = 242, .b = 187, .a = 255};
	data->colors[COLOR_TXT_DEFAULT] = (Clay_Color) {.r = 0, .g = 0, .b = 0, .a = 255};
	data->colors[COLOR_BUTTON_FSL_BACKGROUND] = (Clay_Color) {.r = 233, .g = 236, .b = 239, .a = 255};
	data->colors[COLOR_BUTTON_FSL_BACKGROUND_SELECTED] = (Clay_Color) {.r = 255, .g = 255, .b = 255, .a = 255};
	data->colors[COLOR_BUTTON_SS_BACKGROUND_INACTIVE] = (Clay_Color) {.r = 255, .g = 249, .b = 219, .a = 255};
	data->colors[COLOR_BUTTON_SS_BACKGROUND_ACTIVE] = (Clay_Color) {.r = 250, .g = 82, .b = 82, .a = 255};

	data->sounds.sounds = malloc(sizeof(Sound) * SOUND_COUNT);
	if (data->sounds.sounds == NULL) return 1;
	data->sounds.count = malloc(sizeof(float) * SOUND_COUNT);
	if (data->sounds.count == NULL) return 1;
	data->sounds.sounds[SOUND_CLICK] = LoadSound("resources/sounds/mouseClick.wav"); // TODO: load from wave instead for better control
	if (IsSoundValid(data->sounds.sounds[SOUND_CLICK])) {
		printf("Yes\n");
	} else {
		printf("No\n");
		return 1;
	}
	fflush(stdout);
	data->sounds.count[SOUND_CLICK] = 1;

	return 0;
}

void clean_data(PomodoroData* data) {
	Clean_Tasks(&data->tasks);

	if (data->colors) {
		free(data->colors);
		data->colors = NULL;
	}
	if (data->sounds.sounds) {
		for (int i = 0; i < SOUND_COUNT; i++) {
			UnloadSound(data->sounds.sounds[i]);
		}
		free(data->sounds.sounds);
		data->sounds.sounds = NULL;
	}
	if (data->sounds.count) {
		free(data->sounds.count);
		data->sounds.count = NULL;
	}
	if (data->timerConstraints.timerStr.chars) {
		free(data->timerConstraints.timerStr.chars);
		data->timerConstraints.timerStr.chars = NULL;
	}
}

int main(void) {
	InitAudioDevice();

	PomodoroData pomo_data;
	if (initialize_data(&pomo_data)) {
		clean_data(&pomo_data);
		return 0;
	}

	Clay_Raylib_Initialize(pomo_data.deviceDimensions.x, pomo_data.deviceDimensions.y, APP_TITLE, FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);

	uint32_t totalMemorySize = Clay_MinMemorySize();
	Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
	Clay_Initialize(clayMemory, (Clay_Dimensions) { (float)GetScreenWidth(), (float)GetScreenHeight() }, (Clay_ErrorHandler) { HandleClayErrors, 0 });

	// TODO: SDF font rather than this
	Font fonts[FONT_SIZE];
	fonts[FONT_ID_DEFAULT] = GetFontDefault();

	fonts[FONT_ID_64_PX] = LoadFontEx("resources/fonts/Roboto-Regular.ttf", 64 * 3, NULL, 0);
	SetTextureFilter(fonts[FONT_ID_64_PX].texture, TEXTURE_FILTER_BILINEAR);

	fonts[FONT_ID_32_PX] = LoadFontEx("resources/fonts/Roboto-Regular.ttf", 32 * 3, NULL, 0);
	SetTextureFilter(fonts[FONT_ID_32_PX].texture, TEXTURE_FILTER_BILINEAR);

	fonts[FONT_ID_24_PX] = LoadFontEx("resources/fonts/Roboto-Regular.ttf", 24 * 3, NULL, 0);
	SetTextureFilter(fonts[FONT_ID_24_PX].texture, TEXTURE_FILTER_BILINEAR);

	fonts[FONT_ID_16_PX] = LoadFontEx("resources/fonts/Roboto-Regular.ttf", 16 * 3, NULL, 0);
	SetTextureFilter(fonts[FONT_ID_16_PX].texture, TEXTURE_FILTER_BILINEAR);

	fonts[FONT_ID_12_PX] = LoadFontEx("resources/fonts/Roboto-Regular.ttf", 12 * 3, NULL, 0);
	SetTextureFilter(fonts[FONT_ID_12_PX].texture, TEXTURE_FILTER_BILINEAR);

	for (int i = 0; i < FONT_SIZE; i++) {
		if (!IsFontValid(fonts[i])) {
			UnloadFont(fonts[i]);
			fonts[i] = GetFontDefault();
		}
	}

	Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

	SetTargetFPS(24);
	SetExitKey(KEY_NULL);

	while (!WindowShouldClose()) {
		HANDLE_EVENTS(&totalMemorySize, &clayMemory, &pomo_data);
		if (pomo_data.appState % 2 == 0) {
			if (pomo_data.timerConstraints.timer <= 0.0f) {
				if (pomo_data.appState == STATE_FOCUS) {
					pomo_data.options.focusCount++;
					Add_Focus_Count_To_Task(&pomo_data.tasks.tasks[pomo_data.tasks.currentSelectedTask]);
				}
				if (pomo_data.options.focusCount > 3) {
					pomo_data.options.focusCount = 0;
					pomo_data.appState = STATE_LONG_BREAK_PAUSED;
					pomo_data.stats.totalFocusCount++;

				} else {
					pomo_data.appState = pomo_data.appState == STATE_FOCUS? STATE_SHORT_BREAK_PAUSED: STATE_FOCUS_PAUSED;
				}
				PRINT((double) pomo_data.options.focusCount);
				pomo_data.timerConstraints.timer = pomo_data.timerConstraints.time_constants[pomo_data.appState >> 1];
			} else {
				pomo_data.timerConstraints.timer -= GetFrameTime();
			}
		}
		float *timer = &pomo_data.timerConstraints.timer;
		struct String *str = &pomo_data.timerConstraints.timerStr;
		snprintf(str->chars, str->capacity, "%02d:%02d", (int)*timer / 60, (int)*timer % 60);
		str->length = strlen(str->chars);

		BeginDrawing();
			ClearBackground(BLACK);
			Clay_Raylib_Render(ClayCreateLayout(&pomo_data), fonts);
		EndDrawing();
	}

	for (int i = 0; i < FONT_SIZE; i++) {
		UnloadFont(fonts[i]);
	}
	clean_data(&pomo_data);
	CloseAudioDevice();
	Clay_Raylib_Close();
	return 0;
}

void RenderButton(Clay_String txt, int BG_COLOR, int TXT_COLOR, int BORDER_COLOR, Clay_Color* colors_array, int FONT_ID, int font_size, int letter_spacing) {
	float border_width = FONT_SIZE(BORDER_WIDTH);
	border_width = border_width > 1? border_width: 1;

	float cornerRadius = FONT_SIZE(BORDER_RADIUS);
	cornerRadius = (cornerRadius > 1.0f) ? cornerRadius: 0.0f;

	CLAY(CLAY_SID(txt)) {
		Clay_Color bg_color = colors_array[BG_COLOR];
		if (Clay_Hovered()) {
			bg_color.a -= 100;
		}

		CLAY_AUTO_ID({
			.backgroundColor = bg_color,
			.border = {
				.color = colors_array[BORDER_COLOR],
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
				.fontSize = font_size,
				.textColor = colors_array[TXT_COLOR],
				.wrapMode = CLAY_TEXT_WRAP_NONE,
				.textAlignment = CLAY_TEXT_ALIGN_CENTER
			}));
		}
	}
}

void RenderTask(PomodoroData *data, struct Task *task, int taskIndex, int TXT_COLOR, int FONT_ID, int font_size, int letter_spacing, bool calledFromTasks) {
	Clay_LayoutConfig layout = {
		.layoutDirection = CLAY_LEFT_TO_RIGHT,
		.padding = {
			.top = getFontHelper(data, BASE_FONT_SIZE),
			.bottom = getFontHelper(data, BASE_FONT_SIZE),
			.left = 0,
			.right = 1
		},
		.childGap = getFontHelper(data, HEADER2) * 2
	};

	if (!calledFromTasks) layout.childGap = 0;

	float borderWidth = getFontHelper(data, BORDER_WIDTH * 2);
	borderWidth = (borderWidth > 1.0f) ? borderWidth: 1.0f;

	CLAY_AUTO_ID({
		.layout = layout,
	}) {
		if (Clay_Hovered()) {
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				Select_Task(&data->tasks, taskIndex, data->options.rearrangeTaskOnSelect);
				Clay_UpdateScrollContainers(true, (Clay_Vector2) { .x = 0.0f, .y = 0.0f }, GetFrameTime());
				data->tasks.isEditing = false;
			}
		}

		if (taskIndex == data->tasks.currentSelectedTask && calledFromTasks) {
			Clay_BorderElementConfig border = {};
			Clay_CornerRadius cornerRadius = {};
			Clay_Padding padding = {};
			if (data->tasks.isEditing) {
				border.color = data->colors[TXT_COLOR];
				border.width = (Clay_BorderWidth) CLAY_BORDER_OUTSIDE(borderWidth);
				cornerRadius = (Clay_CornerRadius) CLAY_CORNER_RADIUS(getFontHelper(data, BORDER_WIDTH * 4));
				padding = (Clay_Padding) CLAY_PADDING_ALL(font_size / 2); // TODO: Clay doesn't handle left right padding well, find alternative
			}

			CLAY(CLAY_ID("Selected Text"), {
				.layout = {
					.sizing = {
						.width = CLAY_SIZING_FIT(0),
						.height = CLAY_SIZING_FIT(0)
					},
					.padding = padding,
					.childAlignment = {
						.x = CLAY_ALIGN_X_CENTER,
						.y = CLAY_ALIGN_Y_CENTER
					},
				},
				.border = border,
				.cornerRadius = cornerRadius
			}) {
				if (Clay_Hovered()) {
					if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
						data->tasks.isEditing = true;
					}
				}
				CLAY_TEXT(STRING_TO_CLAY_STRING(task->desc), {
					.fontSize = font_size,
					.fontId = FONT_ID,
					.textColor = data->colors[TXT_COLOR],
					.letterSpacing = letter_spacing,
					.wrapMode = CLAY_TEXT_WRAP_WORDS,
					.textAlignment = CLAY_TEXT_ALIGN_LEFT
				});
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
				CLAY_TEXT(STRING_TO_CLAY_STRING(task->desc), {
					.fontSize = font_size,
					.fontId = FONT_ID,
					.textColor = data->colors[TXT_COLOR],
					.letterSpacing = letter_spacing,
					.wrapMode = CLAY_TEXT_WRAP_WORDS,
					.textAlignment = CLAY_TEXT_ALIGN_LEFT
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
			CLAY_TEXT(STRING_TO_CLAY_STRING(task->countExpected), {
				.fontSize = font_size,
				.fontId = FONT_ID,
				.textColor = data->colors[TXT_COLOR],
				.letterSpacing = letter_spacing,
				.wrapMode = CLAY_TEXT_WRAP_WORDS,
				.textAlignment = CLAY_TEXT_ALIGN_RIGHT
			});
		}
	}
}

static void handle_edits_to_task(PomodoroData* data) {
	static int task_id = -1;

	static int str_buffer_length = 0;
	static char str_buffer[STR_BUFFER_CAPACITY];

	static struct Task* SelectedTask = NULL;

	static float keyTimer = 0.0f;
	static int lastKeyPressed = 0;

	if (data->tasks.currentSelectedTask == -1) return;

	 if (task_id != data->tasks.tasks[data->tasks.currentSelectedTask].id) {
		task_id = data->tasks.tasks[data->tasks.currentSelectedTask].id;
		if (task_id >= 0) {
			SelectedTask = &data->tasks.tasks[data->tasks.currentSelectedTask];

			snprintf(str_buffer, STR_BUFFER_CAPACITY, "%s", SelectedTask->desc.chars);
			str_buffer_length = strlen(str_buffer);
		} else {
			str_buffer_length = 0;
			str_buffer[str_buffer_length] = '\0';

			SelectedTask = NULL;
		}
	}
	if (SelectedTask == NULL) return;

	// Repeat Chars
	if (IsKeyDown(lastKeyPressed)) {
		keyTimer += GetFrameTime();
		if (keyTimer >= data->options.REPEAT_DELAY){
			if (keyTimer >= (data->options.REPEAT_DELAY + data->options.REPEAT_INTERVAL)) {
				switch (lastKeyPressed) {
					case KEY_BACKSPACE: {
						PRINT_S("Yes");
						Remove_Char_From_Task(SelectedTask);
						break;
					}

					default: {
						if (lastKeyPressed >= 32 && lastKeyPressed <= 125) {
							if (SelectedTask->desc.length >= (STR_BUFFER_CAPACITY - 3)) {
								return;
							}
							Add_Char_To_Task(SelectedTask, GetCharPressed());
						}
						break;
					}
				}
				keyTimer = data->options.REPEAT_DELAY;
			}
		}
	} else {
		keyTimer = 0.0f;
	}

	lastKeyPressed = GetKeyPressed();
	while(true) {
		switch (lastKeyPressed) {
			case KEY_ESCAPE: {
				snprintf(SelectedTask->desc.chars, STR_BUFFER_CAPACITY, "%s", str_buffer);
				SelectedTask->desc.length = strlen(SelectedTask->desc.chars);

				data->tasks.isEditing = false;
				task_id = -1;
				break;
			}

			case KEY_BACKSPACE: {
				if (SelectedTask->desc.length <= 0) {
					break;
				}

				Remove_Char_From_Task(SelectedTask);
				break;
			}

			case KEY_ENTER: {
				SelectedTask->desc.chars[SelectedTask->desc.length] = '\0';
				data->tasks.isEditing = false;

				task_id = -1;
				str_buffer_length = 0;
				str_buffer[str_buffer_length] = '\0';
				SelectedTask = NULL;
				break;
			}

			default: {
				if (lastKeyPressed >= 32 && lastKeyPressed <= 125) {
					if (SelectedTask->desc.length >= (STR_BUFFER_CAPACITY - 1)) {
						return;
					}
					Add_Char_To_Task(SelectedTask, GetCharPressed());
				}
				break;
			}
		}

		int temp = GetKeyPressed();
		if (temp == 0) {
			break;
		} else {
			lastKeyPressed = temp;
			PRINT(lastKeyPressed);
		}
	}
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
	Clay_SetDebugModeEnabled(g_debug_mode);

	// UI
	int screenWidth = GetScreenWidth();
	if (screenWidth >= DEVICE_SMART_PHONE_W) {
		data->device = DEVICE_SMART_PHONE;
	} else {
		data->device = DEVICE_WATCH;
	}

	handle_edits_to_task(data);

	if (!data->tasks.isEditing) {
		if (IsKeyPressed(KEY_D)) {
			g_debug_mode = !g_debug_mode;
		}
		if (IsKeyPressed(KEY_T)) {
			g_dark_mode = !g_dark_mode;
		}
		if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
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

			if (g_UI_SCALE < 0.8f) {
				g_UI_SCALE = 0.8f;
			}

			printf("Gui Scale: %.2f\n", g_UI_SCALE);
			fflush(stdout);
		}
	}

	if (IsMouseButtonUp(MOUSE_LEFT_BUTTON)) {
		data->tasks.isDragging = false;
	}
}

void Device_Smart_Watch(PomodoroData* data) {
	// Good enough
	Clay_String str_Start_STOP = {.chars = data->appState % 2 == 0 ? "  STOP  ": "  Start  ", .isStaticallyAllocated = false};
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
		.backgroundColor = data->colors[data->appState % 2 == 0? COLOR_BACKGROUND_ACTIVE: COLOR_BACKGROUND_INACTIVE],
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
			if (data->appState % 2 == 1) {
				CLAY_AUTO_ID({
					.layout  = {
						.layoutDirection = CLAY_LEFT_TO_RIGHT,
						.sizing = {
							.width = CLAY_SIZING_GROW(),
							.height = CLAY_SIZING_PERCENT(0.35f)
						},
						.padding = CLAY_PADDING_ALL(FONT_SIZE(BUTTON_PADDING)),
					},
				}) {
					RenderButton(Focus_ShortBreak_LongBreak, COLOR_BUTTON_FSL_BACKGROUND, COLOR_TXT_DEFAULT, COLOR_TXT_DEFAULT, data->colors, FONT_ID_12_PX, FONT_SIZE(HEADER6), getFontHelper(data, LETTER_SPACING));
					// TODO: add settings
				}
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

					int font_size = data->appState % 2 == 0? HEADER1: HEADER2;
					int font_id = data->appState % 2 == 0? FONT_ID_16_PX: FONT_ID_12_PX;
					int letterSpacing = LETTER_SPACING;

					CLAY_TEXT(str, {
						.fontSize = FONT_SIZE(font_size),
						.fontId = font_id,
						.letterSpacing = FONT_SIZE(letterSpacing),
						.textColor = data->colors[COLOR_TXT_DEFAULT],
						.textAlignment = CLAY_TEXT_ALIGN_CENTER
					});
				}

				CLAY_AUTO_ID({ .layout = { .sizing = { .height = CLAY_SIZING_PERCENT(0.1f) }}}) {}
				RenderButton(str_Start_STOP, data->appState % 2 == 0? COLOR_BUTTON_SS_BACKGROUND_ACTIVE: COLOR_BUTTON_SS_BACKGROUND_INACTIVE, COLOR_TXT_DEFAULT, COLOR_TXT_DEFAULT, data->colors, data->appState % 2 == 0? FONT_ID_12_PX: FONT_ID_16_PX, data->appState % 2 == 0? FONT_SIZE(HEADER4): FONT_SIZE(HEADER6), getFontHelper(data, LETTER_SPACING));
			}
		}
	}

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(str_Start_STOP))) {
		data->appState += data->appState % 2 == 0? 1: -1;
	}

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(Focus_ShortBreak_LongBreak))) {
		data->appState = data->appState == STATE_LONG_BREAK_PAUSED? STATE_FOCUS_PAUSED: data->appState + 2;
		data->timerConstraints.timer = data->timerConstraints.time_constants[data->appState >> 1];
	}
}

void Device_Smart_Phone(PomodoroData* data) {
	Clay_String str_Start_STOP = {.chars = data->appState % 2 == 0 ? "  STOP  ": "  Start  ", .isStaticallyAllocated = false};
	str_Start_STOP.length = strlen(str_Start_STOP.chars);

	bool longerTxtOnButtonCondition = GetScreenWidth() > (DEVICE_SMART_PHONE_W + 100) * g_UI_SCALE;
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
				.bottom = getFontHelper(data, HEADER1)
			},
			.childGap = getFontHelper(data, PARAGRAPH)
		},
		.backgroundColor = RAYLIB_COLOR_TO_CLAY_COLOR(WHITE),
	}) {
		float WIDTH = 0.0f;
		if (data->appState % 2 == 0) {
			WIDTH = Clay_GetElementData(CLAY_ID("Timer")).boundingBox.width;
		} else {
			WIDTH = Clay_GetElementData(CLAY_ID("Buttons")).boundingBox.width + getFontHelper(data, HEADER2) * 2;
		}
		float borderWidth = getFontHelper(data, BORDER_WIDTH * 2);
		borderWidth = (borderWidth > 1.0f) ? borderWidth: 1.0f;

		if (data->appState % 2 != 0) {
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
					.color = data->colors[COLOR_TXT_DEFAULT]
				},
			}){
				Clay_String str = {
					.chars = APP_TITLE,
					.length = strlen(APP_TITLE),
					.isStaticallyAllocated = false
				};

				CLAY_TEXT(str, {
					.fontSize = getFontHelper(data, HEADER1),
					.fontId = FONT_ID_32_PX,
					.letterSpacing = FONT_SIZE(LETTER_SPACING),
					.textColor = data->colors[COLOR_TXT_DEFAULT],
					.textAlignment = CLAY_TEXT_ALIGN_CENTER
				});

				// CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_GROW(0)}}}){}
				// Settings Button
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
					.top = getFontHelper(data, HEADER1),
					.bottom = getFontHelper(data, HEADER1)
				},
				.childGap = getFontHelper(data, HEADER2)
			},
			.border = {
				.width = CLAY_BORDER_OUTSIDE(borderWidth),
				.color = data->colors[COLOR_TXT_DEFAULT]
			},
			.cornerRadius = CLAY_CORNER_RADIUS(getFontHelper(data, BORDER_WIDTH * 4)),
			.backgroundColor = data->appState % 2 == 0? data->colors[COLOR_BACKGROUND_ACTIVE]: data->colors[COLOR_BACKGROUND_INACTIVE]
		}){
			if (data->appState % 2 == 0) {
				if (tasks.currentSelectedTask >= 0 && tasks.currentSelectedTask < tasks.tasks_count && data->appState == STATE_FOCUS) {
					CLAY_AUTO_ID({
						.clip = {
							.vertical = true,
							.childOffset = Clay_GetScrollOffset()
						},
						.layout = {
							.sizing = {
								.width = CLAY_SIZING_FIXED(WIDTH),
								.height = CLAY_SIZING_FIT(0, getFontHelper(data, HEADER3 * 3)),
							},
							.layoutDirection = CLAY_TOP_TO_BOTTOM,
							.childGap = getFontHelper(data, PARAGRAPH)
						},
						.border = {
							.width = {
								.bottom =  borderWidth
							},
							.color = data->colors[COLOR_TXT_DEFAULT]
						},
					}) {
						RenderTask(data, &tasks.tasks[tasks.currentSelectedTask], tasks.currentSelectedTask, COLOR_TXT_DEFAULT, FONT_ID_16_PX, getFontHelper(data, HEADER3), getFontHelper(data, LETTER_SPACING), false);
						CLAY_AUTO_ID({});
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
						.childGap = FONT_SIZE((BUTTON_PADDING * 3))
					},
					.border = {
						.width = CLAY_BORDER_OUTSIDE(borderWidth),
						.color = data->colors[COLOR_TXT_DEFAULT]
					},
					.cornerRadius = CLAY_CORNER_RADIUS(getFontHelper(data, BORDER_RADIUS * 2))
				}){
					// TODO: Better color for selected and clickables
					RenderButton(str_Focus, data->appState == STATE_FOCUS_PAUSED? COLOR_BUTTON_FSL_BACKGROUND_SELECTED: COLOR_BUTTON_FSL_BACKGROUND, COLOR_TXT_DEFAULT, COLOR_TXT_DEFAULT, data->colors, FONT_ID_16_PX, getFontHelper(data, PARAGRAPH), getFontHelper(data, LETTER_SPACING));
					RenderButton(str_Short_Break, data->appState == STATE_SHORT_BREAK_PAUSED? COLOR_BUTTON_FSL_BACKGROUND_SELECTED: COLOR_BUTTON_FSL_BACKGROUND, COLOR_TXT_DEFAULT, COLOR_TXT_DEFAULT, data->colors, FONT_ID_16_PX, getFontHelper(data, PARAGRAPH), getFontHelper(data, LETTER_SPACING));
					RenderButton(str_Long_Break, data->appState == STATE_LONG_BREAK_PAUSED? COLOR_BUTTON_FSL_BACKGROUND_SELECTED: COLOR_BUTTON_FSL_BACKGROUND, COLOR_TXT_DEFAULT, COLOR_TXT_DEFAULT, data->colors, FONT_ID_16_PX, getFontHelper(data, PARAGRAPH), getFontHelper(data, LETTER_SPACING));
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
					.childGap = getFontHelper(data, PARAGRAPH)
				}
			}) {
				Clay_String str = {
					.chars = data->timerConstraints.timerStr.chars,
					.length = data->timerConstraints.timerStr.length,
					.isStaticallyAllocated = true
				};

				int font_size = data->appState % 2 == 0? HEADER1 * 4: HEADER1 * 2;
				int font_id = font_size == HEADER1 * 4? FONT_ID_64_PX: FONT_ID_32_PX;
				int letterSpacing = LETTER_SPACING;

				CLAY_TEXT(str, {
					.fontSize = getFontHelper(data, font_size),
					.fontId = font_id,
					.letterSpacing = getFontHelper(data, letterSpacing),
					.textColor = data->colors[COLOR_TXT_DEFAULT],
					.textAlignment = CLAY_TEXT_ALIGN_CENTER
				});

				int fontSize = fmax(data->appState % 2 == 0? FONT_SIZE(HEADER4): FONT_SIZE(HEADER6), data->appState % 2 == 0? getFontHelper(data, HEADER3): getFontHelper(data, HEADER2));
				RenderButton(
					str_Start_STOP,
					data->appState % 2 == 0? COLOR_BUTTON_SS_BACKGROUND_ACTIVE: COLOR_BUTTON_SS_BACKGROUND_INACTIVE,
					COLOR_TXT_DEFAULT,
					COLOR_TXT_DEFAULT,
					data->colors,
					data->appState % 2 == 0? FONT_ID_16_PX: FONT_ID_24_PX,
					fontSize,
					getFontHelper(data, LETTER_SPACING)
				);
			}
		}

		if (data->appState % 2 != 0) {
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
					.color = data->colors[COLOR_TXT_DEFAULT]
				}
			}){
				for (int i = 0; i < tasks.tasks_count; ++i) {
					if (tasks.isDragging) continue;
					RenderTask(data, &tasks.tasks[i], i, COLOR_TXT_DEFAULT, FONT_ID_16_PX, getFontHelper(data, PARAGRAPH), getFontHelper(data, LETTER_SPACING), true);
				}

				// TODO: render dragging element and if it's border and another task's border cross more than 50% put it up of that task
			}
		}
	}

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(str_Start_STOP))) {
		data->appState += data->appState % 2 == 0? 1: -1;
		PlaySound(data->sounds.sounds[SOUND_CLICK]);
	} else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(str_Focus))) {
		if (data->appState != STATE_FOCUS && data->appState != STATE_FOCUS_PAUSED) {
			data->appState = STATE_FOCUS_PAUSED;
			data->timerConstraints.timer = data->timerConstraints.time_constants[data->appState >> 1];
			PlaySound(data->sounds.sounds[SOUND_CLICK]);
		}
	} else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(str_Short_Break))) {
		if (data->appState != STATE_SHORT_BREAK && data->appState != STATE_SHORT_BREAK_PAUSED) {
			data->appState = STATE_SHORT_BREAK_PAUSED;
			data->timerConstraints.timer = data->timerConstraints.time_constants[data->appState >> 1];
			PlaySound(data->sounds.sounds[SOUND_CLICK]);
		}
	} else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Clay_PointerOver(Clay_GetElementId(str_Long_Break))) {
		if (data->appState != STATE_LONG_BREAK && data->appState != STATE_LONG_BREAK_PAUSED) {
			data->options.focusCount = 0;
			data->appState = STATE_LONG_BREAK_PAUSED;
			data->timerConstraints.timer = data->timerConstraints.time_constants[data->appState >> 1];
			PlaySound(data->sounds.sounds[SOUND_CLICK]);
		}
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