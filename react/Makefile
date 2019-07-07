DIST_PATH  = ./build
DIST_FILES = 2.chunk.js main.css main.chunk.js runtime~main.js favicon.ico

all: build $(addsuffix .gz,$(addprefix $(DIST_PATH)/,$(DIST_FILES)))
	@echo "*BEFORE"
	@du -shc $(addprefix $(DIST_PATH)/,$(DIST_FILES))
	@echo "*AFTER"
	@du -shc $(addsuffix .gz,$(addprefix $(DIST_PATH)/,$(DIST_FILES)))

build: node_modules
	npm run build

node_modules:
	npm install

%.gz: %
	gzip -c --best $< > $@

.SUFFIXES: .gz
.PHONY: all build

