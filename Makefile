# ──────────────────────────────────────────────────────────────────────────────
#                                   LIBFTPP
# ──────────────────────────────────────────────────────────────────────────────

NAME             := libftpp.a

DOCKER_COMPOSE   := docker compose
SERVICE          := libftpp

# ──────────────────────────────────────────────────────────────────────────────
# Paths
# ──────────────────────────────────────────────────────────────────────────────

SRC_DIR          := src
INC_DIR          := include
OBJ_DIR          := build
DOCS_DIR         := docs

DOXYGEN_BUILD    := $(OBJ_DIR)/doxygen
DOXYFILE         := $(DOXYGEN_BUILD)/Doxyfile.generated
DOCS_HTML        := $(DOCS_DIR)/html/index.html

# ──────────────────────────────────────────────────────────────────────────────
# Colors
# ──────────────────────────────────────────────────────────────────────────────

RESET            := \033[0m
BOLD             := \033[1m
GREEN            := \033[32m
YELLOW           := \033[33m
BLUE             := \033[34m
CYAN             := \033[36m
RED              := \033[31m

# ──────────────────────────────────────────────────────────────────────────────
# Commands executed inside Docker
# ──────────────────────────────────────────────────────────────────────────────

ifeq ($(IN_DOCKER),1)

CXX              := c++
AR               := ar
ARFLAGS          := rcs
DOXYGEN          := doxygen

CXXFLAGS         := -Wall -Wextra -Werror -std=c++11
CPPFLAGS         := -I$(INC_DIR) -MMD -MP

# Do not print an error when src/ does not exist.
SRCS             := $(shell \
	if [ -d "$(SRC_DIR)" ]; then \
		find "$(SRC_DIR)" -type f -name '*.cpp'; \
	fi \
)

OBJS             := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS             := $(OBJS:.o=.d)

# ──────────────────────────────────────────────────────────────────────────────
# Main build
# ──────────────────────────────────────────────────────────────────────────────

all: banner $(NAME)

banner:
	@printf "$(BOLD)$(CYAN)"
	@printf "╔══════════════════════════════════════╗\n"
	@printf "║            LIBFTPP BUILD             ║\n"
	@printf "╚══════════════════════════════════════╝\n"
	@printf "$(RESET)"

$(NAME): $(OBJS)
	@printf "$(BLUE)[AR]$(RESET)   %s\n" "$(NAME)"
	@$(AR) $(ARFLAGS) "$(NAME)" $(OBJS)
	@printf "$(GREEN)$(BOLD)[OK]$(RESET)   %s created successfully\n" "$(NAME)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p "$(dir $@)"
	@printf "$(YELLOW)[CXX]$(RESET)  %s\n" "$<"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c "$<" -o "$@"

# ──────────────────────────────────────────────────────────────────────────────
# Automatic Doxygen configuration
# ──────────────────────────────────────────────────────────────────────────────

$(DOXYFILE):
	@mkdir -p "$(DOXYGEN_BUILD)"
	@printf "$(BLUE)[DOXYGEN]$(RESET) Creating automatic configuration...\n"
	@{ \
		printf '%s\n' 'PROJECT_NAME           = "libftpp"'; \
		printf '%s\n' 'PROJECT_NUMBER         = "1.0"'; \
		printf '%s\n' 'PROJECT_BRIEF          = "Reusable C++ template library"'; \
		printf '%s\n' ''; \
		printf '%s\n' 'OUTPUT_DIRECTORY       = docs'; \
		printf '%s\n' 'CREATE_SUBDIRS          = NO'; \
		printf '%s\n' ''; \
		printf '%s\n' 'INPUT                  = include src README.md'; \
		printf '%s\n' 'RECURSIVE              = YES'; \
		printf '%s\n' ''; \
		printf '%s\n' 'FILE_PATTERNS          = *.h *.hpp *.hh *.hxx *.c *.cpp *.cc *.cxx *.md'; \
		printf '%s\n' ''; \
		printf '%s\n' 'EXCLUDE                = build docs .git'; \
		printf '%s\n' 'EXCLUDE_PATTERNS       = */build/* */docs/* */.git/*'; \
		printf '%s\n' ''; \
		printf '%s\n' 'EXTRACT_ALL            = YES'; \
		printf '%s\n' 'EXTRACT_PRIVATE        = YES'; \
		printf '%s\n' 'EXTRACT_STATIC         = YES'; \
		printf '%s\n' 'EXTRACT_LOCAL_CLASSES  = YES'; \
		printf '%s\n' ''; \
		printf '%s\n' 'SOURCE_BROWSER         = YES'; \
		printf '%s\n' 'INLINE_SOURCES         = YES'; \
		printf '%s\n' 'REFERENCED_BY_RELATION = YES'; \
		printf '%s\n' 'REFERENCES_RELATION    = YES'; \
		printf '%s\n' ''; \
		printf '%s\n' 'GENERATE_HTML          = YES'; \
		printf '%s\n' 'HTML_OUTPUT            = html'; \
		printf '%s\n' 'GENERATE_TREEVIEW      = YES'; \
		printf '%s\n' 'FULL_SIDEBAR           = YES'; \
		printf '%s\n' ''; \
		printf '%s\n' 'GENERATE_LATEX         = NO'; \
		printf '%s\n' 'GENERATE_MAN           = NO'; \
		printf '%s\n' 'GENERATE_XML           = NO'; \
		printf '%s\n' ''; \
		printf '%s\n' 'HAVE_DOT               = YES'; \
		printf '%s\n' 'DOT_NUM_THREADS        = 0'; \
		printf '%s\n' 'CLASS_DIAGRAMS         = YES'; \
		printf '%s\n' 'COLLABORATION_GRAPH    = YES'; \
		printf '%s\n' 'INCLUDE_GRAPH          = YES'; \
		printf '%s\n' 'INCLUDED_BY_GRAPH      = YES'; \
		printf '%s\n' 'DIRECTORY_GRAPH        = YES'; \
		printf '%s\n' 'GRAPHICAL_HIERARCHY    = YES'; \
		printf '%s\n' ''; \
		printf '%s\n' 'QUIET                  = NO'; \
		printf '%s\n' 'WARNINGS               = YES'; \
		printf '%s\n' 'WARN_IF_UNDOCUMENTED   = YES'; \
		printf '%s\n' 'WARN_IF_DOC_ERROR      = YES'; \
		printf '%s\n' 'WARN_AS_ERROR          = NO'; \
		printf '%s\n' ''; \
		printf '%s\n' 'JAVADOC_AUTOBRIEF      = YES'; \
		printf '%s\n' 'QT_AUTOBRIEF           = YES'; \
		printf '%s\n' 'MULTILINE_CPP_IS_BRIEF = YES'; \
		printf '%s\n' 'MARKDOWN_SUPPORT       = YES'; \
		printf '%s\n' 'AUTOLINK_SUPPORT       = YES'; \
		printf '%s\n' ''; \
		printf '%s\n' 'OPTIMIZE_OUTPUT_FOR_C  = NO'; \
		printf '%s\n' 'CPP_CLI_SUPPORT        = NO'; \
		printf '%s\n' 'BUILTIN_STL_SUPPORT    = YES'; \
	} > "$(DOXYFILE)"

# ──────────────────────────────────────────────────────────────────────────────
# Documentation
# ──────────────────────────────────────────────────────────────────────────────

docs: docs-check docs-clean $(DOXYFILE)
	@printf "$(BLUE)[DOXYGEN]$(RESET) Generating documentation...\n"
	@$(DOXYGEN) "$(DOXYFILE)"
	@if [ ! -f "$(DOCS_HTML)" ]; then \
		printf "$(RED)[ERROR]$(RESET) Expected file not found: %s\n" "$(DOCS_HTML)"; \
		printf "$(YELLOW)[INFO]$(RESET) Existing generated indexes:\n"; \
		find . -type f -path '*/html/index.html' -print; \
		exit 1; \
	fi
	@printf "$(GREEN)$(BOLD)[OK]$(RESET) Documentation generated successfully\n"
	@printf "$(GREEN)[PATH]$(RESET) %s\n" "$(DOCS_HTML)"

docs-check:
	@if ! command -v "$(DOXYGEN)" >/dev/null 2>&1; then \
		printf "$(RED)[ERROR]$(RESET) Doxygen is not installed in the container\n"; \
		exit 1; \
	fi
	@if ! command -v dot >/dev/null 2>&1; then \
		printf "$(RED)[ERROR]$(RESET) Graphviz is not installed in the container\n"; \
		exit 1; \
	fi
	@if [ ! -d "$(INC_DIR)" ] && \
	    [ ! -d "$(SRC_DIR)" ] && \
	    [ ! -f README.md ]; then \
		printf "$(RED)[ERROR]$(RESET) No documentation input was found\n"; \
		printf "$(YELLOW)[INFO]$(RESET) Expected include/, src/ or README.md\n"; \
		exit 1; \
	fi
	@printf "$(CYAN)[INPUT]$(RESET) Documentation sources:\n"
	@if [ -d "$(INC_DIR)" ]; then printf "  - %s/\n" "$(INC_DIR)"; fi
	@if [ -d "$(SRC_DIR)" ]; then printf "  - %s/\n" "$(SRC_DIR)"; fi
	@if [ -f README.md ]; then printf "  - README.md\n"; fi

docs-clean:
	@printf "$(RED)[DOCS CLEAN]$(RESET) Removing generated documentation\n"
	@rm -rf "$(DOCS_DIR)"
	@rm -rf "$(DOXYGEN_BUILD)"

# ──────────────────────────────────────────────────────────────────────────────
# Cleaning
# ──────────────────────────────────────────────────────────────────────────────

clean:
	@printf "$(RED)[CLEAN]$(RESET) Removing object files\n"
	@rm -rf "$(OBJ_DIR)"

fclean: clean
	@printf "$(RED)[FCLEAN]$(RESET) Removing %s\n" "$(NAME)"
	@rm -f "$(NAME)"
	@rm -rf "$(DOCS_DIR)"

re: fclean all

-include $(DEPS)

# ──────────────────────────────────────────────────────────────────────────────
# Commands executed from the host
# ──────────────────────────────────────────────────────────────────────────────

else

all:
	@printf "[DOCKER] Building image if necessary...\n"
	@$(DOCKER_COMPOSE) build $(SERVICE)
	@printf "[DOCKER] Building libftpp...\n"
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) make IN_DOCKER=1 all

clean:
	@printf "[DOCKER] Removing object files...\n"
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) make IN_DOCKER=1 clean

fclean:
	@printf "[DOCKER] Performing full cleanup...\n"
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) make IN_DOCKER=1 fclean

re:
	@printf "[DOCKER] Rebuilding image and project...\n"
	@$(DOCKER_COMPOSE) build $(SERVICE)
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) make IN_DOCKER=1 re

docs:
	@printf "[DOCKER] Building documentation image...\n"
	@$(DOCKER_COMPOSE) build $(SERVICE)
	@printf "[DOCKER] Generating Doxygen documentation...\n"
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) make IN_DOCKER=1 docs

docs-clean:
	@printf "[DOCKER] Removing generated documentation...\n"
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) make IN_DOCKER=1 docs-clean

docs-open:
	@if [ ! -f "$(DOCS_HTML)" ]; then \
		printf "[INFO] Documentation is missing, generating it first...\n"; \
		$(MAKE) docs; \
	fi
	@if command -v xdg-open >/dev/null 2>&1; then \
		printf "[OPEN] %s\n" "$(DOCS_HTML)"; \
		xdg-open "$(DOCS_HTML)" >/dev/null 2>&1 & \
	elif command -v gio >/dev/null 2>&1; then \
		printf "[OPEN] %s\n" "$(DOCS_HTML)"; \
		gio open "$(DOCS_HTML)" >/dev/null 2>&1 & \
	else \
		printf "[INFO] Open this file in your browser:\n"; \
		printf "       %s\n" "$(DOCS_HTML)"; \
	fi

shell:
	@printf "[DOCKER] Building image if necessary...\n"
	@$(DOCKER_COMPOSE) build $(SERVICE)
	@printf "[DOCKER] Opening container shell...\n"
	@$(DOCKER_COMPOSE) run --rm $(SERVICE) bash

docker-build:
	@printf "[DOCKER] Building Docker image...\n"
	@$(DOCKER_COMPOSE) build $(SERVICE)

endif

.PHONY: \
	all \
	banner \
	clean \
	fclean \
	re \
	docs \
	docs-check \
	docs-clean \
	docs-open \
	shell \
	docker-build