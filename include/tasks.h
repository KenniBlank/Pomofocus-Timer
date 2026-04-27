#ifndef TASKS_H
#define TASKS_H

struct String {
	int length;
	int capacity;
	char *chars;
};

struct Task {
	struct String desc;
	struct String countExpected;
	int count;
	int expected;

	bool completed;
};

typedef struct {
	struct Task* tasks;
	int __capacity__;
	int tasks_count;

	int currentSelectedTask;
} Tasks;

int Init_Tasks(Tasks*);
struct Task CreateNewTask(const char *, int count, int expected);
int Add_Task(Tasks*, struct Task, bool rearrange);
int Add_Focus_Count_To_Task(struct Task *task);
void Select_Task(Tasks*, int index, bool rearrange);
void Remove_Task(Tasks*, int index);
void Clean_Tasks(Tasks*);

#endif