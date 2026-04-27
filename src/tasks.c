#include <stdio.h>
#include <stdbool.h>
#include <string.h>
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

	return 0;
}

Task CreateNewTask(const char *str_literal, int count, int expected) {
	Task newTask = {
		.count = count,
		.expected = expected,
		.completed = false
	};

	newTask.desc.capacity = 124;
	newTask.desc.chars = malloc(sizeof(char) * newTask.desc.capacity);
	if (newTask.desc.chars == NULL) {
		return newTask;
	}
	strcpy(newTask.desc.chars, str_literal);
	newTask.desc.length = strlen(newTask.desc.chars);

	newTask.countExpected.capacity = 16; // MAGIC NUMBER fix later
	newTask.countExpected.chars = malloc(sizeof(char) * newTask.countExpected.capacity);
	if (newTask.countExpected.chars == NULL) {
		free(newTask.desc.chars);
		return newTask;
	}
	newTask.countExpected.length = snprintf(newTask.countExpected.chars, newTask.countExpected.capacity, "%d/%d", count, expected);

	return newTask;
};

// Task ModifyTask(Tasks *task_arr, int index, char *new_str, int count, int expected) {
// 	// struct String clay_str;

// 	// Task newTask = { .count = count, .expected = expected, .desc = clay_str };
// };

int Add_Task(Tasks* task_arr, struct Task newTask, bool rearrange) {
	// Returns -2 on task being invalid
	// Returns -1 on being unable to add task to tasks
	// Returns 0 on success

	if (newTask.desc.chars == NULL || newTask.countExpected.chars == NULL) return -2;
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

void Select_Task(Tasks *tasks, int index, bool rearrange) {
	if (index == tasks->currentSelectedTask) tasks->currentSelectedTask = -1;

	if (index > tasks->tasks_count || index < 0) return;
	if (!rearrange) {
		tasks->currentSelectedTask = index;
		return;
	}

	Task selectedTask = tasks->tasks[index];
	// index = tasks->tasks_count - 1;
	while (index > 0) {
		tasks->tasks[index] = tasks->tasks[index - 1];
		--index;
	}
	tasks->tasks[0] = selectedTask;
	tasks->currentSelectedTask = 0;
}

int Add_Focus_Count_To_Task(Task *task) {
	// TODO: case handling in case the task's expected string is not long enough
	task->count++;
	task->countExpected.length = snprintf(task->countExpected.chars, task->countExpected.capacity, "%d/%d", task->count, task->expected);
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
			if (task_arr->tasks[i].countExpected.chars) {
				free(task_arr->tasks[i].countExpected.chars);
			}
			if (task_arr->tasks[i].desc.chars) {
				free(task_arr->tasks[i].desc.chars);
			}
		}
		free(task_arr->tasks);
	}
}