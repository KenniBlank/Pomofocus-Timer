#ifndef TASKS_H
#define TASKS_H

#define STR_BUFFER_CAPACITY 256

struct String {
	int length;
	int capacity;
	char *chars;
};

enum EDITING_STATUS {
	IS_NOT_EDITING,
	IS_EDITING_TASK_DESC,
	IS_EDITING_TASK_COUNT_EXPECTED
};

struct Task {
	struct String desc;
	struct String count_expected;

	int count;
	int expected;

	int id;

	bool __completed__;
	bool __descDefined__;
	bool __countExpectedDefined__;
};

typedef struct {
	struct Task* tasks;
	int currentSelectedTask;

	int __capacity__;
	int tasks_count;

	enum EDITING_STATUS isEditing;
	bool isDragging;
} Tasks;

int Init_Tasks(Tasks*);
struct Task CreateNewTask(const char *, int count, int expected);
int Add_Task(Tasks*, struct Task, bool rearrange);

// Maybe macro:
int Add_Focus_Count_To_Task(struct Task *task);
void Select_Task(Tasks*, int index, bool rearrange);
void Remove_Task(Tasks*, int index);
void Clean_Tasks(Tasks*);

#endif