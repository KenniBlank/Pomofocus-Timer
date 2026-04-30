#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tasks.h"

typedef struct Task Task;
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
	static int id = 0;

	Task newTask = {
		.count = count,
		.expected = expected,
		.completed = false,
		.id = id
	};
	id++;

	// TODO: error handling
	newTask.desc.capacity = STR_BUFFER_CAPACITY;
	newTask.desc.chars = malloc(sizeof(char) * newTask.desc.capacity);
	if (newTask.desc.chars == NULL) return newTask;
	snprintf(newTask.desc.chars, STR_BUFFER_CAPACITY, "%s", str_literal);
	newTask.desc.length = strlen(newTask.desc.chars);

	newTask.count_expected.capacity = STR_BUFFER_CAPACITY;
	newTask.count_expected.chars = malloc(sizeof(char) * newTask.desc.capacity);
	if (newTask.desc.chars == NULL) {
		if (newTask.desc.chars) {
			free(newTask.desc.chars);
		}
		return newTask;
	}
	snprintf(newTask.count_expected.chars, STR_BUFFER_CAPACITY, "%d / %d", count, expected);
	newTask.count_expected.length = strlen(newTask.count_expected.chars);

	return newTask;
};

int Add_Task(Tasks* task_arr, struct Task newTask, bool rearrange) {
	// Returns -2 on task being invalid
	// Returns -1 on being unable to add task to tasks
	// Returns 0 on success

	if (newTask.desc.chars == NULL) return -2;
	if (task_arr->__capacity__ < (task_arr->tasks_count + 1)) {
		int newCapacity = task_arr->__capacity__ * 2;

		Task *tasks = (Task*) realloc(task_arr->tasks, sizeof(Task) * task_arr->__capacity__);
		if (tasks == NULL) {
			return -1;
		}

		task_arr->tasks = tasks;
		task_arr->__capacity__ = newCapacity;
	}

	task_arr->tasks[task_arr->tasks_count] = newTask;

	Select_Task(task_arr, task_arr->tasks_count, rearrange);

	task_arr->tasks_count++;
	return 0;
}

void Add_Char_To_Task(Task* task, char ch) {
	if (task && task->desc.length < (task->desc.capacity - 2)) {
		task->desc.chars[task->desc.length] = ch;
		task->desc.length++;
		task->desc.chars[task->desc.length] = '\0';
	}
}

void Remove_Char_From_Task(Task* task) {
	if (task && task->desc.length > 0) {
		task->desc.chars[--task->desc.length] = '\0';
	}
}

void Select_Task(Tasks *tasks, int index, bool rearrange) {
	if (index == tasks->currentSelectedTask) return;
	if (index > tasks->tasks_count || index < 0) return;

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
	return 0;
}

void Remove_Task(Tasks* task_arr, int index) {
	if (index >= task_arr->tasks_count || index < 0) return;

	while (index < (task_arr->tasks_count - 1)) {
		task_arr->tasks[index] = task_arr->tasks[index + 1];
		++index;
	}
	--task_arr->tasks_count;
}

void Clean_Tasks(Tasks *task_arr) {
	if (task_arr->tasks) {
		for (int i = 0; i < task_arr->tasks_count; i++) {
			if (task_arr->tasks[i].desc.chars) {
				free(task_arr->tasks[i].desc.chars);
			}
		}
		free(task_arr->tasks);
	}
}