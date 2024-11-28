# 編譯器和編譯選項
CC = gcc
CFLAGS = -Wall -Ilib -g

# 目錄
LIB_DIR = lib
SRC_DIR = src
BUILD_DIR = build

# 輸出檔案
LIBRARY = $(BUILD_DIR)/librwcsv.a
TARGET = pairup 

# librwcsv 靜態函式庫的目標和源文件
LIB_OBJECTS = $(BUILD_DIR)/librwcsv.o
LIB_SOURCES = $(LIB_DIR)/rw-csv.c

# pairup 相關的目標和源文件
PAIRUP_SOURCES = $(SRC_DIR)/pairup/pairup-algorithm.c $(SRC_DIR)/pairup/pairup-formatter.c $(SRC_DIR)/pairup/pairup-types.c
PAIRUP_OBJECTS = $(PAIRUP_SOURCES:$(SRC_DIR)/pairup/%.c=$(BUILD_DIR)/pairup/%.o)

# main 的目標和源文件
SRC_OBJECTS = $(BUILD_DIR)/main.o
SRC_SOURCES = $(SRC_DIR)/main.c

# version.h 的目標
VERSION_FILE = $(SRC_DIR)/version.h

# 編譯所有
all: $(VERSION_FILE) $(TARGET)

# 生成 version.h
$(VERSION_FILE):
	echo "#ifndef VERSION_H" > $@
	echo "#define VERSION_H" >> $@
	echo "/* Program version format: version <major>.<minor>.<patch> */" >> $@
	echo "#define PROGRAM_VERSION \"version 1.0.$(shell git log --pretty=format:%h -n 1) ($(shell date +%Y-%m-%d) nightly build)\"" >> $@
	echo "#endif  // VERSION_H" >> $@

# 生成靜態函式庫
$(LIBRARY): $(LIB_OBJECTS)
	mkdir -p $(BUILD_DIR)
	ar rcs $@ $^

# 編譯 librwcsv 物件檔
$(BUILD_DIR)/librwcsv.o: $(LIB_SOURCES)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 編譯 pairup 物件檔
$(BUILD_DIR)/pairup/%.o: $(SRC_DIR)/pairup/%.c
	mkdir -p $(BUILD_DIR)/pairup
	$(CC) $(CFLAGS) -c $< -o $@

# 編譯 main 物件檔
$(BUILD_DIR)/main.o: $(SRC_SOURCES)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 最終連結成可執行檔 main
$(TARGET): $(LIBRARY) $(SRC_OBJECTS) $(PAIRUP_OBJECTS)
	$(CC) $(CFLAGS) $(SRC_OBJECTS) $(PAIRUP_OBJECTS) $(LIBRARY) -o $@

# 清理生成的檔案
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(VERSION_FILE)

