MAKEFLAGS	= --no-print-directory --silent -j

NAME := ft_shield

CXX := gcc

CXXFLAGS := -Wall -Wextra -Werror
DEBUG := -g3 #-fsanitize=address

RM := rm -rf

SRCS := ft_shield.c

OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)

CPPFLAGS := -MMD -MP

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) $(DEBUG) -o $@ && printf "Linking: $(NAME)\n"

clean:
	@$(RM) $(OBJS) $(DEPS)

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
