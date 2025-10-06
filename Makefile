NAME = MAKEFLAGS    = --no-print-directory

NAME = ft_shield

CFLAGS       = #-Wextra -Wall -Werror
CFLAGS      += -I inc

CC = gcc

DEBUG        = -g3 -fsanitize=address

CPPFLAGS     = -MMD

HEADERS      = -I ./inc

FILES       =											\
				srcs/main.c								\

OBJS = $(patsubst srcs/%.c, objs/srcs/%.o, $(FILES))
DEPS       = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(DEBUG) $(OBJS) $(HEADERS) -o $(NAME) && printf "Linking: $(NAME)\n"

objs/srcs/%.o: ./srcs/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(DEBUG) $(CPPFLAGS) $(CFLAGS) -o $@ -c $< $(HEADERS) && printf "Compiling: $(notdir $<)\n"

clean:
	@rm -rf objs

fclean: clean
	@rm -rf $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re

# how to copy from host to VM
# rsync -avz -e "ssh -p 4040" --delete ~/ft_shield pepe@localhost:~/ft_shield