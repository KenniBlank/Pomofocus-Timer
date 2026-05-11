# GOAL

- [ ] Minimal Viable Product for release in Flathub finally

## Helper:
- Thag from discord, CLAY

# COOL
- https://stackoverflow.com/a/5256426

## APP

Support from idk like 300px x 300px to inf x inf?
Lets make it responsive i guess

## Dependencies:

- Raylib
- CLAY (Header only so not really a requirement)
- https://github.com/DaveGamble/cJSON

## Font
- https://www.youtube.com/watch?v=qcMuyHzhvpI

## Scaling
- https://acerx-amj.github.io/blogs/responsive_design.html

## Temporary Notes on CLAY

Clay Initilize
clayRequiredMemory
new areana with requiredmemory
initialize Clay finally


```c
// Minimal
Clay_Raylib_Initialize(screenWidth, screenHeight, APP_TITLE, WINDOW_FLAGS);

uint32_t clayRequiredMemory = Clay_MinMemorySize();
Clay_Arena clayMemory = (Clay_Arena) {
	.memory = malloc(clayRequiredMemory),
	.capacity = clayRequiredMemory
};

Clay_Initialize(clayMemory, (Clay_Dimensions) {.width = ..., .height = ...})

while (!WindowShouldClose()) {
	Clay_BeginLayout();
	// Define the UI
	// ...
	// ...
	Clay_RenderCommandArray renderCommands = Clay_EndLayout();

	// Draw the UI
	BeginDrawing();
	Clay_Raylib_Render(renderCommands);
	EndDrawing();
}

Clay_Raylib_Close(); // Internally frees allocated memory
```

1. Every frame, set Dimensions so that resizing reflects.

```c
CLAY(
	// Definition of parent element

	// 1. CLAY_ID("...")
	// 2. CLAY_LAYOUT({.layout = ..., ...})

) {
	// Child Element
}
```


2. Adding text

CLAY needs to know how big the text is gonna be when rendered so that it can account for it accordingly. (Done before the main loop by getting the values per font size)
```c
CLAY_TEXT(txt, CLAY_TEXT_CONFIG({...})}
```

3. Layout
-> Layout direction, child gap, padding, ...

4. Inputs
-> Inputs like mouse scroll, click, have to be passed to CLAY every frame

# Makefile

```pMakefile
targets: prerequisites
	commands to generate target
```

there can be multiple targets and prerequisite

the first of this type of syntax is run first

```bash
all: f1.o f2.o

f1.o f2.o:
	echo $@
# Equivalent to:
# f1.o:
#	 echo f1.o
# f2.o:
#	 echo f2.o
```

## WildCards

**\*** searches your filesystem for matching filenames.

Do: $(wildcard *.o)

Don't: *.0

```bash
thing_right := $(wildcard *.o)
```

can be prerequisite or ...

**%** wildcard
- When used in "matching" mode, it matches one or more characters in a string. This match is called the stem.
- When used in "replacing" mode, it takes the stem that was matched and replaces that in a string.
- % is most often used in rule definitions and in some specific functions.


Implicit rule: Compiling a C program: n.o is made automatically from n.c with a command of the form $(CC) -c $(CPPFLAGS) $(CFLAGS) $^ -o $@
```bash
n: n.o
```

```bash
hey: one two
	# Outputs "hey", since this is the target name
	echo $@

	# Outputs all prerequisites newer than the target
	echo $?

	# Outputs all prerequisites
	echo $^

	# Outputs the first prerequisite
	echo $<

	touch hey

one:
	touch one

two:
	touch two

clean:
	rm -f hey one two
```

## Sounds

- Click Sound Effect by <a href="https://pixabay.com/users/universfield-28281460/?utm_source=link-attribution&utm_medium=referral&utm_campaign=music&utm_content=351398">Universfield</a> from <a href="https://pixabay.com//?utm_source=link-attribution&utm_medium=referral&utm_campaign=music&utm_content=351398">Pixabay</a>

***

// Source - https://stackoverflow.com/a/1442131
// Posted by Adam Rosenfield, modified by community. See post 'Timeline' for change history
// Retrieved 2026-05-10, License - CC BY-SA 4.0
For the get today's date part

***

- Break Sound Effect by <a href="https://pixabay.com/users/freesound_community-46691455/?utm_source=link-attribution&utm_medium=referral&utm_campaign=music&utm_content=6416">freesound_community</a> from <a href="https://pixabay.com/sound-effects//?utm_source=link-attribution&utm_medium=referral&utm_campaign=music&utm_content=6416">Pixabay</a>


***
- Focus Sound Effect by <a href="https://pixabay.com/users/freesound_community-46691455/?utm_source=link-attribution&utm_medium=referral&utm_campaign=music&utm_content=102201">freesound_community</a> from <a href="https://pixabay.com/sound-effects//?utm_source=link-attribution&utm_medium=referral&utm_campaign=music&utm_content=102201">Pixabay</a>