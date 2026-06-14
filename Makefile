# ──────────────────────────────────────────────────────────────────────────────
#                                   LIBFTPP
# ──────────────────────────────────────────────────────────────────────────────

NAME             := libftpp.a

DOCKER_COMPOSE   := docker compose
SERVICE          := libftpp

# Colors used inside the Linux container
RESET            := \033[0m
BOLD             := \033[1m
GREEN            := \033[32m
YELLOW           := \033[33m
BLUE             := \033[34m
CYAN             := \033[36m
RED              := \033[31m

# ──────────────────────────────────────────────────────────────────────────────
# Compilation inside Docker
# ──────────────────────────────────────────────────────────────────────────────

ifeq ($(IN_DOCKER),1)

CXX              := c++
AR               := ar
ARFLAGS          := rcs

CXXFLAGS         := -Wall -Wextra -Werror -std=c++11
CPPFLAGS         := -Iinclude -MMD -MP

SRC_DIR          := src
OBJ_DIR          := build

SRCS             := $(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJS             := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS             := $(OBJS:.o=.d)

all: banner $(NAME)

banner:
	@printf "$(BOLD)$(CYAN)"
	@printf "╔══════════════════════════════════════╗\n"
	@printf "║            LIBFTPP BUILD             ║\n"
	@printf "╚══════════════════════════════════════╝\n"
	@printf "$(RESET)"

$(NAME): $(OBJS)
	@printf "$(BLUE)[AR]$(RESET)   %s\n" "$(NAME)"
	@$(AR) $(ARFLAGS) $(NAME) $(OBJS)
	@printf "$(GREEN)$(BOLD)[OK]$(RESET)   %s created successfully\n" "$(NAME)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@printf "$(YELLOW)[CXX]$(RESET)  %s\n" "$<"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	@printf "$(RED)[CLEAN]$(RESET) Removing %s\n" "$(OBJ_DIR)"
	@rm -rf $(OBJ_DIR)

fclean: clean
	@printf "$(RED)[FCLEAN]$(RESET) Removing %s\n" "$(NAME)"
	@rm -f $(NAME)

re: fclean all

-include $(DEPS)

# ──────────────────────────────────────────────────────────────────────────────
# Commands launched from the host machine
# ──────────────────────────────────────────────────────────────────────────────

else

all:
	@echo [DOCKER] Building libftpp...
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) make IN_DOCKER=1 all

clean:
	@echo [DOCKER] Removing object files...
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) make IN_DOCKER=1 clean

fclean:
	@echo [DOCKER] Performing full cleanup...
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) make IN_DOCKER=1 fclean

re:
	@echo [DOCKER] Rebuilding everything...
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) make IN_DOCKER=1 re

shell:
	@echo [DOCKER] Opening container shell...
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) bash

docker-build:
	@echo [DOCKER] Building Docker image...
	@$(DOCKER_COMPOSE) build

endif

.PHONY: all banner clean fclean re shell docker-build