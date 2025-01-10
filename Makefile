NAME	= ircserv

CC		= c++
FLAGS	= -Wall -Wextra -Werror -std=c++98 #-g

SRC		= $(wildcard ./src/*.cpp) $(wildcard ./src/cmd/*.cpp)

OBJ_DIR	= ./obj
OBJ		= $(patsubst ./src/%.cpp, $(OBJ_DIR)/%.o, $(SRC))

HEADER	= $(wildcard ./inc/*.hpp)

.SILENT:

all:		$(NAME)

$(OBJ_DIR):
			@mkdir -p $(OBJ_DIR)

$(NAME):	$(OBJ_DIR) $(OBJ) $(HEADER)
			@echo "\033[36mMaking $(NAME)...\033[0m"
			$(CC) $(FLAGS) $(OBJ) -o $(NAME)
			@echo "\033[36m$(NAME) compiled\033[0m"
			@echo "\033[32mExecution: ./$(NAME) <port> <password>\033[0m"

$(OBJ_DIR)/%.o: ./src/%.cpp
			mkdir -p $(dir $@)
			$(CC) $(FLAGS) -c $< -o $@

clean:		
			rm -f $(OBJ)
			rm -rf $(OBJ_DIR)
			@echo "\033[31mCleaned\033[0m"

fclean:		clean
			rm -f $(NAME)
			@echo "\033[31mCleaned everything\033[0m"

re:			fclean all

.PHONY: all clean fclean re
