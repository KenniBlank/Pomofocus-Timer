#ifndef TASKS_H
#define TASKS_H

#define STR_BUFFER_CAPACITY 256

struct String {
	int length;
	char chars[STR_BUFFER_CAPACITY];
};

enum EDITING_STATUS {
	IS_NOT_EDITING,
	IS_EDITING_TASK_DESC,
	IS_EDITING_TASK_COUNT_EXPECTED
};

struct Task {
	struct String desc;
	struct String count_expected;

	int id;

	int count;
	int expected;

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

void Remove_Char_From_String(struct String* str);
void Add_Char_To_String(struct String* str, char ch);

// Maybe macro:
int Add_Focus_Count_To_Task(struct Task *task);
void Select_Task(Tasks*, int index, bool rearrange);
void Remove_Task(Tasks*, int index);
void Clean_Tasks(Tasks*);

#endif