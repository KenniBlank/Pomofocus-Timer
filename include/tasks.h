#ifndef TASKS_H
#define TASKS_H

#define STR_BUFFER_CAPACITY 256

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

	int id;

	bool completed;
};

typedef struct {
	struct Task* tasks;
	int __capacity__;
	int tasks_count;

	int currentSelectedTask;
	bool isEditing;

	bool isDragging;
} Tasks;

int Init_Tasks(Tasks*);
struct Task CreateNewTask(const char *, int count, int expected);
int Add_Task(Tasks*, struct Task, bool rearrange);
void Add_Char_To_Task(struct Task* task, char ch);

// Maybe macro:
int Add_Focus_Count_To_Task(struct Task *task);
void Remove_Char_From_Task(struct Task* task);

void Select_Task(Tasks*, int index, bool rearrange);
void Remove_Task(Tasks*, int index);
void Clean_Tasks(Tasks*);

#endif