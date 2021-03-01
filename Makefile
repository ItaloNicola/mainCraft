TARGET_EXEC ?= mainCraft.run
BUILD_DIR ?= ./build

PROJ_DIRS ?= opengl vulkan common
SOURCES := $(wildcard $(addsuffix /*.c,$(PROJ_DIRS))) $(wildcard $(addsuffix /*.cpp,$(PROJ_DIRS))) main.c

OBJS := $(SOURCES:%=$(BUILD_DIR)/%.o)

HEADER_DIRS += $(PROJ_DIRS) /usr/include/freetype2/ libs
INCLUDES += $(addprefix -I,$(HEADER_DIRS))

LIB_NAMES ?= ftgl GL glfw vulkan m stb stdc++
LD_LIBS += $(addprefix -l,$(LIB_NAMES))

CFLAGS += -Wall -Wcast-align -Wunreachable-code

ifeq (${XDG_SESSION_TYPE}, wayland)
	MACROS += -D GLFW_USE_WAYLAND=ON
endif

all: $(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) shaders
	$(CC) $(OBJS) $(LD_LIBS) $(LDFLAGS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ $(MACROS)

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) $(INCLUDES) -c $< -o $@

debug: CFLAGS += -ggdb3 -Og -fstack-protector-all -fsanitize=address -fsanitize=undefined
debug: CFLAGS += -fsanitize=leak -fsanitize-recover=all -fno-omit-frame-pointer
debug: LDFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize-recover=all
debug: LD_LIBS += -lasan
debug: $(BUILD_DIR)/$(TARGET_EXEC)

debug_vk: MACROS += -D ENABLE_VALIDATION_LAYERS
debug_vk: debug

.PHONY: shaders
shaders: shaders/main_shader.vert shaders/main_shader.frag
	glslangValidator -V shaders/main_shader.vert -o shaders/vert.spv
	glslangValidator -V shaders/main_shader.frag -o shaders/frag.spv

.PHONY: run
run:
	$(BUILD_DIR)/$(TARGET_EXEC)

.PHONY: tags
tags:
	ctags -R --c++-kinds=+p --extra=+q --fields=+iaS .

.PHONY: clean
clean:
	@$(RM) -rf $(BUILD_DIR)

MKDIR_P ?= @mkdir -p
