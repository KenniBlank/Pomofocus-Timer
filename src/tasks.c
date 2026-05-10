#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tasks.h"

typedef struct Task Task;
static int __ID__ = 0;

// TODO: create Arena and preallocate memory for 16 tasks and description of it
int Init_Tasks(Tasks* task_arr) {
	int capacity = 16;
	task_arr->tasks = malloc(sizeof(Task) * capacity);
	if (task_arr->tasks == NULL) {
		return 1;
	}
	task_arr->__capacity__ = capacity;
	task_arr->tasks_count = 0;
	task_arr->currentSelectedTask = -1;
	task_arr->isEditing = IS_NOT_EDITING;

	// TODO:
	task_arr->isDragging = false;
	return 0;
}

Task CreateNewTask(const char *str_literal, int count, int expected) {
	Task newTask = {
		.count = count,
		.expected = expected,
		.__completed__ = false,
		.id = __ID__
	};

	// TODO: error handling
	snprintf(newTask.desc.chars, STR_BUFFER_CAPACITY, "%s", str_literal);
	newTask.desc.length = strlen(newTask.desc.chars);
	newTask.__descDefined__ = newTask.desc.length > 0;

	snprintf(newTask.count_expected.chars, STR_BUFFER_CAPACITY, "%d / %d", count, expected);
	newTask.count_expected.length = strlen(newTask.count_expected.chars);
	newTask.__countExpectedDefined__ = count != 0;

	__ID__++;
	return newTask;
};

int Add_Task(Tasks* task_arr, struct Task newTask, bool rearrange) {
	// Returns -2 on task being invalid
	// Returns -1 on being unable to add task to tasks
	// Returns 0 on success

	if (task_arr->__capacity__ < (task_arr->tasks_count + 1)) {
		int newCapacity = task_arr->__capacity__ * 2;

		Task *tasks = (Task*) realloc(task_arr->tasks, sizeof(Task) * newCapacity);
		if (tasks == NULL) {
			return -1;
		}

		task_arr->tasks = tasks;
		task_arr->__capacity__ = newCapacity;
	}

	task_arr->tasks[task_arr->tasks_count] = newTask;
	task_arr->tasks_count++;

	Select_Task(task_arr, task_arr->tasks_count - 1, rearrange);
	return 0;
}

void Add_Char_To_String(struct String* str, char ch) {
	if (str && str->length < (STR_BUFFER_CAPACITY - 2)) {
		str->chars[str->length] = ch;
		str->length++;
		str->chars[str->length] = '\0';
	}
}

void Remove_Char_From_String(struct String* str) {
	if (str && str->length > 0) {
		str->chars[--str->length] = '\0';
	}
}

void Select_Task(Tasks *tasks, int index, bool rearrange) {
	if (index == tasks->currentSelectedTask) return;
	if (index >= tasks->tasks_count || index < 0) return;

	if (!rearrange) {
		tasks->currentSelectedTask = index;
		return;
	}

	Task selectedTask = tasks->tasks[index];
	while (index > 0) {
		tasks->tasks[index] = tasks->tasks[index - 1];
		--index;
	}
	tasks->tasks[0] = selectedTask;
	tasks->currentSelectedTask = 0;
	tasks->isEditing = IS_NOT_EDITING;
}

int Add_Focus_Count_To_Task(Task *task) {
	// TODO: case handling in case the task's expected string is not long enough
	task->count++;
	snprintf(task->count_expected.chars, STR_BUFFER_CAPACITY, "%d / %d", task->count, task->expected);
	task->count_expected.length = strlen(task->count_expected.chars);

	// TODO Maybe: if (task->count == task->expected) task->completed = true;
	return 0;
}

void Remove_Task(Tasks* task_arr, int index) {
	if (index >= task_arr->tasks_count || index < 0) return;

	Task task_to_remove = task_arr->tasks[index];
	while (index < (task_arr->tasks_count - 1)) {
		task_arr->tasks[index] = task_arr->tasks[index + 1];
		++index;
	}
	task_arr->tasks[index] = task_to_remove;
	--task_arr->tasks_count;
	task_arr->currentSelectedTask = task_arr->tasks_count > 0? 0: -1;
}

void Clean_Tasks(Tasks *task_arr) {
	if (task_arr->tasks) {
		free(task_arr->tasks);
		task_arr->tasks = NULL;
	}
}