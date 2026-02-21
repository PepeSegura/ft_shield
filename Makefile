MAKEFLAGS	= --no-print-directory -j 4 #--silent

NAME := ft_shield

CC := gcc

CCFLAGS := -Wall -Wextra -Werror

RM := rm -rf

SRCS := ft_shield.c

DEPS := unity.d

all: $(NAME)

$(NAME): ft_shield_quined.c
	$(CC) $(CCFLAGS) ft_shield_quined.c -o $@

ft_shield_quined.c: unity.c quine_gen
	gcc $(CCFLAGS) -MMD -MP -MF unity.d -E unity.c -I inc -o ft_shield.c
	./quine_gen

quine_gen: quine_gen.cpp
	g++ quine_gen.cpp -o quine_gen

clean:
	@$(RM) $(DEPS) ft_shield.c quine_gen ft_shield_quined.c

fclean: clean
	@$(RM) $(NAME)
	- sudo systemctl stop $(NAME) 2>/dev/null; sudo systemctl disable $(NAME) 2>/dev/null; sudo rm /etc/systemd/system/$(NAME).service 2>/dev/null; sudo systemctl daemon-reload 2>/dev/null
	- sudo rm -f /bin/$(NAME) 2>/dev/null
	- sudo rm -f /tmp/ft_shield.c 2>/dev/null

re:: fclean
re:: $(NAME)

debug:: CCFLAGS += -D DEBUG
debug:: fclean
debug:: $(NAME)

.PHONY: all clean fclean re debug

-include $(DEPS)
