# C/C++ Coding standards and practices 
By Oskar Mendel (brokenprogrammer@gmail.com)

This is a work in progress and will be updated over time.

## About these standards
Theese conventions and this standard has been put together to make all personal and collaboration code will
remain easy to read, clear and concise. This allows people working on projects using this standard to share common coding
conventions which in turn makes it easier to read other developers code with focus on solving problems.

All code written should be written with readability and maintainability in mind. No code should be written with the mindset
that you'd be happy to never see that portion of code again. All pieces of code will be seen again and will need to be maintained.
Preparing code for readability will help prevent future problems.

## Project
Project should be by default be built with all warnings turned on as well as warnings being treated as errors. 
Specific warnings may be turned of when working with C based projects.
Source files should be named in Pascal case where words are separated by underscores.
* My_File.txt

### Source control
Source control has to be used for any project that meets one of the two following criterias:
* Might take more than 48 hours to complete.
* Has multiple developers.

## Header files
### Header protection
Every header file should be protected from multiple includes using the following technique:
Wrong:
```C
#ifndef GAME_ASSET_H
#include "Some_File.h"
#endif
```
Correct:
```C
#ifndef GAME_ASSET_H
#define GAME_ASSET_H
#pragma once

// Contents here

#endif GAME_ASSET_H
```

### Defining functions (Inline & static)
A header file should only be used for declaring new types. However if a set of helper functions are to be created
a header file may use inlined functions. 

Inline functions are functions that typically can be implemented in 3-4 lines of code.

Example:
```C
inline r32
Square(r32 Value)
{
	r32 Result = Value * Value;

	return (Result);
}
```

If the header file are providing interface functions to another subsystem or project functions may be marked as static and declared
within the header file but the implementation should still be provided within the source files.

## Source files
### Locally defined types
Types only used by a few set of functions may be defined locally. 

## Code formatting
### Indentation
Use tabs for indenting code but set tabs to be four spaces. (Some editors handles this differently but always strive towards this.)

### Spacing
A space should precede and follow binary operators such as +, -, *, / etc.
Wrong:
```C
MyVariable+=10;
```
Correct:
```C
MyVariable += 10;
```

### Scope braces
Braces for a new scope should be placed at separate lines and the contents of the scope should be indented.
Wrong:
```C
while (1) {
    // Contents
}
```
Correct:
```C
while(1)
{
    //Contents
}
```

### Functions
#### Naming
Function names must start with an uppercase letter followed with an uppercase letter for the first letter in each word afterwards.
Strive to make what the function is doing clear by reading its name. Never us abbreviations when full names describes it better.
Example: `DrawTransformedBitmap()`

#### Return values
All return values should be placed within parameters. This is to make the code more concise if you were ever to return values directly from the functions.
For shorter functions try to name the returning value "Result".

Example:
```C
inline b32
IsValid(sound_id ID)
{
	b32 Result = (ID.Value != 0);

	return (Result);
}
```

### Variables
#### Naming
All variable naming will follow the same naming conventions as with functions.
Example: `r32 Result = 0.0f;`

### Structs
#### Naming
Structs should be named with all letters in lowercase and words separated with an underscore.
As for struct members they should be named with the same type of naming conventions as variables.
