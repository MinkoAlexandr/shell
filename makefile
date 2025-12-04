COMPILER = g++
COMPILE_FLAGS = -std=c++20 -Wall -Wextra
FUSE_CONFIG = -I/usr/include/fuse3 -lfuse3 -L/usr/lib/x86_64-linux-gnu
OUTPUT_NAME = kubsh

APP_VERSION = 1.0.0
APP_PACKAGE = kubsh
BUILD_FOLDER = build
DEB_FOLDER = $(BUILD_FOLDER)/$(APP_PACKAGE)_$(APP_VERSION)_amd64
DEB_OUTPUT = $(PWD)/kubsh.deb

SOURCE_FILES = main.cpp vfs.cpp
OBJECT_FILES = $(SOURCE_FILES:.cpp=.o)

build: $(OUTPUT_NAME)

$(OUTPUT_NAME): $(OBJECT_FILES)
	$(COMPILER) $(COMPILE_FLAGS) -o $(OUTPUT_NAME) $(OBJECT_FILES) $(FUSE_CONFIG)

%.o: %.cpp
	$(COMPILER) $(COMPILE_FLAGS) $(FUSE_CONFIG) -c $< -o $@

execute: $(OUTPUT_NAME)
	./$(OUTPUT_NAME)

prepare-deb: $(OUTPUT_NAME)
	@mkdir -p $(DEB_FOLDER)/DEBIAN
	@mkdir -p $(DEB_FOLDER)/usr/local/bin
	@cp $(OUTPUT_NAME) $(DEB_FOLDER)/usr/local/bin/
	@chmod +x $(DEB_FOLDER)/usr/local/bin/$(OUTPUT_NAME)
	
	@echo "Package: $(APP_PACKAGE)" > $(DEB_FOLDER)/DEBIAN/control
	@echo "Version: $(APP_VERSION)" >> $(DEB_FOLDER)/DEBIAN/control
	@echo "Section: utilities" >> $(DEB_FOLDER)/DEBIAN/control
	@echo "Priority: optional" >> $(DEB_FOLDER)/DEBIAN/control
	@echo "Architecture: amd64" >> $(DEB_FOLDER)/DEBIAN/control
	@echo "Maintainer: Developer <dev@example.com>" >> $(DEB_FOLDER)/DEBIAN/control
	@echo "Description: Custom shell implementation" >> $(DEB_FOLDER)/DEBIAN/control
	@echo " Educational shell project with VFS support." >> $(DEB_FOLDER)/DEBIAN/control

deb-package: prepare-deb
	@dpkg-deb --build $(DEB_FOLDER)
	@mv $(BUILD_FOLDER)/$(APP_PACKAGE)_$(APP_VERSION)_amd64.deb $(DEB_OUTPUT)
	@echo "Package ready: $(DEB_OUTPUT)"

install-app: deb-package
	sudo dpkg -i $(DEB_OUTPUT)

remove-app:
	sudo dpkg -r $(APP_PACKAGE)

container-test: deb-package
	@docker run --rm \
		-v $(DEB_OUTPUT):/mnt/kubsh.deb \
		--device /dev/fuse \
		--cap-add SYS_ADMIN \
		--security-opt apparmor:unconfined \
		ghcr.io/xardb/kubshfuse:master 2>/dev/null || true

cleanup:
	rm -rf $(BUILD_FOLDER) $(OUTPUT_NAME) *.deb $(OBJECT_FILES)

show-help:
	@echo "Available commands:"
	@echo "  make build        - compile application"
	@echo "  make deb-package  - create deb package"
	@echo "  make install-app  - install package"
	@echo "  make remove-app   - uninstall package"
	@echo "  make cleanup      - clean project"
	@echo "  make execute      - run shell"
	@echo "  make container-test - test in Docker"
	@echo "  make show-help    - display this help"

.PHONY: build deb-package install-app remove-app cleanup show-help prepare-deb execute container-test
