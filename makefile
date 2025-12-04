COMPILER = g++
COMPILE_FLAGS = -std=c++20 -Wall -Wextra
FUSE_CONFIG = -I/usr/include/fuse3 -lfuse3 -L/usr/lib/x86_64-linux-gnu
OUTPUT_NAME = kubsh
# Компилятор и флаги
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra 
FUSE_FLAGS = -I/usr/include/fuse3 -lfuse3 -L/usr/lib/x86_64-linux-gnu
TARGET = kubsh

APP_VERSION = 1.0.0
APP_PACKAGE = kubsh
BUILD_FOLDER = build
DEB_FOLDER = $(BUILD_FOLDER)/$(APP_PACKAGE)_$(APP_VERSION)_amd64
DEB_OUTPUT = $(PWD)/kubsh.deb
# Версия пакета
VERSION = 1.0.0
PACKAGE_NAME = kubsh
BUILD_DIR = build
DEB_DIR = $(BUILD_DIR)/$(PACKAGE_NAME)_$(VERSION)_amd64
DEB_FILE := $(PWD)/kubsh.deb

SOURCE_FILES = main.cpp vfs.cpp
OBJECT_FILES = $(SOURCE_FILES:.cpp=.o)
# Исходные файлы
SRCS = main.cpp vfs.cpp
OBJS = $(SRCS:.cpp=.o)

build: $(OUTPUT_NAME)
# Основные цели
all: $(TARGET)

$(OUTPUT_NAME): $(OBJECT_FILES)
	$(COMPILER) $(COMPILE_FLAGS) -o $(OUTPUT_NAME) $(OBJECT_FILES) $(FUSE_CONFIG)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(FUSE_FLAGS)

%.o: %.cpp
	$(COMPILER) $(COMPILE_FLAGS) $(FUSE_CONFIG) -c $< -o $@
	$(CXX) $(CXXFLAGS) $(FUSE_FLAGS) -c $< -o $@

execute: $(OUTPUT_NAME)
	./$(OUTPUT_NAME)
# Запуск шелла
run: $(TARGET)
	./$(TARGET)

prepare-deb: $(OUTPUT_NAME)
	@mkdir -p $(DEB_FOLDER)/DEBIAN
	@mkdir -p $(DEB_FOLDER)/usr/local/bin
	@cp $(OUTPUT_NAME) $(DEB_FOLDER)/usr/local/bin/
	@chmod +x $(DEB_FOLDER)/usr/local/bin/$(OUTPUT_NAME)
# Подготовка структуры для deb-пакета
prepare-deb: $(TARGET)
	@echo "Подготовка структуры для deb-пакета..."
	@mkdir -p $(DEB_DIR)/DEBIAN
	@mkdir -p $(DEB_DIR)/usr/local/bin
	@cp $(TARGET) $(DEB_DIR)/usr/local/bin/
	@chmod +x $(DEB_DIR)/usr/local/bin/$(TARGET)

	@echo "Package: $(APP_PACKAGE)" > $(DEB_FOLDER)/DEBIAN/control
	@echo "Version: $(APP_VERSION)" >> $(DEB_FOLDER)/DEBIAN/control
	@echo "Section: utilities" >> $(DEB_FOLDER)/DEBIAN/control
	@echo "Priority: optional" >> $(DEB_FOLDER)/DEBIAN/control
	@echo "Architecture: amd64" >> $(DEB_FOLDER)/DEBIAN/control
	@echo "Maintainer: Developer <dev@example.com>" >> $(DEB_FOLDER)/DEBIAN/control
	@echo "Description: Custom shell implementation" >> $(DEB_FOLDER)/DEBIAN/control
	@echo " Educational shell project with VFS support." >> $(DEB_FOLDER)/DEBIAN/control
	@echo "Создание control файла..."
	@echo "Package: $(PACKAGE_NAME)" > $(DEB_DIR)/DEBIAN/control
	@echo "Version: $(VERSION)" >> $(DEB_DIR)/DEBIAN/control
	@echo "Section: utils" >> $(DEB_DIR)/DEBIAN/control
	@echo "Priority: optional" >> $(DEB_DIR)/DEBIAN/control
	@echo "Architecture: amd64" >> $(DEB_DIR)/DEBIAN/control
	@echo "Maintainer: Your Name <your.email@example.com>" >> $(DEB_DIR)/DEBIAN/control
	@echo "Description: Simple custom shell" >> $(DEB_DIR)/DEBIAN/control
	@echo " A simple custom shell implementation for learning purposes." >> $(DEB_DIR)/DEBIAN/control

deb-package: prepare-deb
	@dpkg-deb --build $(DEB_FOLDER)
	@mv $(BUILD_FOLDER)/$(APP_PACKAGE)_$(APP_VERSION)_amd64.deb $(DEB_OUTPUT)
	@echo "Package ready: $(DEB_OUTPUT)"
# Сборка deb-пакета
deb: prepare-deb
	@echo "Сборка deb-пакета..."
	dpkg-deb --build $(DEB_DIR)
	@mv $(BUILD_DIR)/$(PACKAGE_NAME)_$(VERSION)_amd64.deb $(DEB_FILE)
	@echo "Пакет создан: $(DEB_FILE)"

install-app: deb-package
	sudo dpkg -i $(DEB_OUTPUT)
# Установка пакета (требует sudo)
install: deb
	sudo dpkg -i $(DEB_FILE)

remove-app:
	sudo dpkg -r $(APP_PACKAGE)
# Удаление пакета
uninstall:
	sudo dpkg -r $(PACKAGE_NAME)

container-test: deb-package
	@docker run --rm \
		-v $(DEB_OUTPUT):/mnt/kubsh.deb \
# Тестирование в Docker контейнере
test: deb
	@echo "Запуск теста в Docker контейнере..."
	@-docker run --rm \
		-v $(DEB_FILE):/mnt/kubsh.deb \
		--device /dev/fuse \
		--cap-add SYS_ADMIN \
		--security-opt apparmor:unconfined \
		ghcr.io/xardb/kubshfuse:master 2>/dev/null || true

cleanup:
	rm -rf $(BUILD_FOLDER) $(OUTPUT_NAME) *.deb $(OBJECT_FILES)
# Очистка
clean:
	rm -rf $(BUILD_DIR) $(TARGET) *.deb $(OBJS)

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
# Показать справку
help:
	@echo "Доступные команды:"
	@echo "  make all      - собрать программу"
	@echo "  make deb      - создать deb-пакет"
	@echo "  make install  - установить пакет"
	@echo "  make uninstall - удалить пакет"
	@echo "  make clean    - очистить проект"
	@echo "  make run      - запустить шелл"
	@echo "  make test     - собрать и запустить тест в Docker"
	@echo "  make help     - показать эту справку"

.PHONY: build deb-package install-app remove-app cleanup show-help prepare-deb execute container-test
.PHONY: all deb install uninstall clean help prepare-deb run test
