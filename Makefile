OBJS = main.c

CC = gcc

INCLUDE_PATHS = -IC:\MinGW_external_libs\SDL2-2.28.5\include\SDL2 -IC:\MinGW_external_libs\SDL2_Image-2.8.1\include\SDL2 -IC:\MinGW_external_libs\SDL2_ttf-2.20.2\include\SDL2 -IC:\MinGW_external_libs\cJSON

LIBRARY_PATHS = -LC:\MinGW_external_libs\SDL2-2.28.5\lib -LC:\MinGW_external_libs\SDL2_Image-2.8.1\lib -LC:\MinGW_external_libs\SDL2_ttf-2.20.2\lib

COMPILER_FLAGS = -g -o0

# compiler flags for release: -w -Wl,-subsystem,windows

LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf

OBJ_NAME = yarz

all:$(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
