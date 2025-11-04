MAKEFLAGS	= --no-print-directory --silent -j

NAME := ft_shield

CXX := gcc

CXXFLAGS := -Wall -Wextra -Werror
DEBUG := -g3 -fsanitize=address

RM := rm -rf

BUILD_DIR := .objs/

SRC_DIR := srcs/
INC_DIR := inc/

SRCS := $(shell find $(SRC_DIR) -name '*.c')

OBJS := $(SRCS:$(SRC_DIR)%.c=$(BUILD_DIR)%.o)
DEPS := $(OBJS:.o=.d)

INC := $(shell find $(INC_DIR) -type d)

INC_FLAGS := $(addprefix -I , $(INC))

CPPFLAGS := $(INC_FLAGS) -MMD -MP

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) -lbsd $(OBJS) $(DEBUG) -o $@ && printf "Linking: $(NAME)\n"

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	@mkdir -p $(dir $@)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEBUG) -c $< -o $@ && printf "Compiling: $(notdir $<)\n"

clean:
	@$(RM) $(BUILD_DIR)

fclean: clean
	@$(RM) $(NAME)
	- sudo systemctl stop $(NAME) 2>/dev/null; sudo systemctl disable $(NAME) 2>/dev/null; sudo rm /etc/systemd/system/$(NAME).service 2>/dev/null; sudo systemctl daemon-reload 2>/dev/null
	- sudo rm -f /bin/$(NAME) 2>/dev/null

re:: fclean
re:: $(NAME)

debug:: CPPFLAGS += -D DEBUG
debug:: fclean
debug:: $(NAME)

.PHONY: all clean fclean re debug

-include $(DEPS)
