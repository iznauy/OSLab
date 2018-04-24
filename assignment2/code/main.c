#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISK_SIZE 1474560 // 2880 sections and 512 byte per section
#define DIR_NAME_SIZE 9 // 8 + 1, 1 means \0
#define DIR_EXTENSION_SIZE 4 // 3 + 1, 1 means \0

#define LENGTH_PRE_ENTRY 32

#define FILE_ATTRIBUTE 0x20
#define DIR_ATTRIBUTE 0x10

#define BYTES_PER_SEC_OFFSET 11
#define BYTES_PER_SEC_LENGTH 2

#define SEC_PER_CLUS_OFFSET 13
#define SEC_PER_CLUS_LENGTH 1

#define RSVD_SEC_CNT_OFFSET 14
#define RSVD_SEC_CNT_LENGTH 2

#define NUM_FATS_OFFSET 16
#define NUM_FATS_LENGTH 1

#define ROOT_ENT_CNT_OFFSET 17
#define ROOT_ENT_CNT_LENGTH 2

#define FAT_SZ_16_OFFSET 22
#define FAT_SZ_16_LENGTH 2

#define _FILE 1
#define _DIRECTORY 2

#define true 1
#define false 0

#define BUFF_SIZE 1024

#define CAT "cat"
#define EXIT "exit"
#define COUNT "count"
#define LS "ls"

#define RED "\033[31m"
#define END "\033[0m"
#define GREEN "\033[32m"
#define BLUE  "\033[34m"
#define PURPLE "\033[35m"
#define YELLOW "\033[33m"

const char * filename = "a.img";

typedef int FileType;

typedef int _bool;

unsigned char image[DISK_SIZE]; // disk, use unsigned char to avoid bit extension

// some fields of BPB Table
struct BPB {
    int BytsPerSec;
    int SecPerClus;
    int RsvdSecCnt;
    int NumFATs;
    int RootEntCnt;
    int FATSz16;
} bpb;

// use 2-tree to substitute n-tree
struct Dir_Tree {
    char name[DIR_NAME_SIZE];
    char extension[DIR_EXTENSION_SIZE];
    FileType type;
    int first_clus;
    struct Dir_Tree * first_child;
    struct Dir_Tree * nextSibling;
    size_t file_size;
} dir_tree;

void _printf(char * arg);

void my_putchar(int c);

void itoa(int a, char * space); // a is greater than zero
char to_low_case(char c);
void load_img(const char * filename);
void load_BPB();
int parse_int(size_t offset, size_t length);
void build_dir_tree(struct Dir_Tree * tree, _bool is_root);
int get_next_clus(int current_clus);
void trim(char * str);
void show_prompt();
void show_file_content(char * file);
void show_count(char * path);
void show_system_structure(char * path);
const struct Dir_Tree * _find_logic_file(char * path);
void _show_file_entry(int _pre, const struct Dir_Tree * tree);
void _show_structure_entry(int _pre, const struct Dir_Tree * tree);
int _count_file(const struct Dir_Tree * tree);
int _count_dir(const struct Dir_Tree * tree);
void _set_color(const char * color);

int main()
{
    load_img(filename);
    load_BPB();
    build_dir_tree(&dir_tree, true);
    while(true) {
        show_prompt();
        char buffer[BUFF_SIZE];
        scanf("%s", buffer);
        if (strcmp(buffer, EXIT) == 0) {
            return 0;
        } else if (strcmp(buffer, CAT) == 0) {
            scanf("%s", buffer);
            trim(buffer);
            show_file_content(buffer);
        } else if (strcmp(buffer, COUNT) == 0) {
            scanf("%s", buffer);
            trim(buffer);
            show_count(buffer);
        } else if (strcmp(buffer, LS) == 0) {
            char c = getchar();
            if (c == '\n')
                show_system_structure(NULL);
            else {
                scanf("%s", buffer);
                trim(buffer);
                show_system_structure(buffer);
            }
        } else {
            // wrong
            _set_color(RED);
            _printf("Wrong Input, please check your instruction.");
            _set_color(END);
        }
    }
}

char to_low_case(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    else
        return c;
}

void load_img(const char * filename) // load specific disk image
{
    FILE * file;
    file = fopen(filename, "rb");
    fread(image, DISK_SIZE, 1, file);
    fclose(file);
}

void load_BPB()
{
    bpb.BytsPerSec = parse_int(BYTES_PER_SEC_OFFSET, BYTES_PER_SEC_LENGTH);
    bpb.SecPerClus = parse_int(SEC_PER_CLUS_OFFSET, SEC_PER_CLUS_LENGTH);
    bpb.RsvdSecCnt = parse_int(RSVD_SEC_CNT_OFFSET, RSVD_SEC_CNT_LENGTH);
    bpb.NumFATs = parse_int(NUM_FATS_OFFSET, NUM_FATS_LENGTH);
    bpb.RootEntCnt = parse_int(ROOT_ENT_CNT_OFFSET, ROOT_ENT_CNT_LENGTH);
    bpb.FATSz16 = parse_int(FAT_SZ_16_OFFSET, FAT_SZ_16_LENGTH);
}

int parse_int(size_t offset, size_t length)
{
    int result = 0;
    for (size_t i = offset + length - 1; i >= offset; i--) {
        result = (result << 8) + image[i];
    }
    return result;
}

void build_dir_tree(struct Dir_Tree * const tree, _bool is_root)
{
    size_t offset = (bpb.RsvdSecCnt + bpb.NumFATs * bpb.FATSz16) * bpb.BytsPerSec;
    int clus = tree->first_clus;
    if (!is_root) {
        offset += bpb.RootEntCnt * LENGTH_PRE_ENTRY + (clus - 2) * bpb.SecPerClus
                                                      * bpb.BytsPerSec;
    }
    for (size_t i = offset; ; i += LENGTH_PRE_ENTRY) {
        if (is_root == true && i >= offset + bpb.RootEntCnt * LENGTH_PRE_ENTRY) {
            break;
        } else if (!is_root && i >= offset + bpb.SecPerClus * bpb.BytsPerSec) {
            clus = get_next_clus(clus);
            if (clus >= 0xff7){
                break;
            }
            offset = i = bpb.RootEntCnt * LENGTH_PRE_ENTRY +
                         (bpb.RsvdSecCnt + bpb.NumFATs * bpb.FATSz16) * bpb.BytsPerSec
                         + (clus - 2) * bpb.SecPerClus * bpb.BytsPerSec;
        }
        if ((image[i + 0xB] == FILE_ATTRIBUTE || image[i + 0xB] == DIR_ATTRIBUTE)
                && ((image[i] >= 'A' && image[i] <= 'Z') || (image[i] >= 'a' && image[i] <= 'z') ||
                (image[i] >= '0' && image[i] <= '9'))) {
            // file or directory
            struct Dir_Tree * sub_tree = (struct Dir_Tree *) malloc(sizeof(struct Dir_Tree));
            size_t within_offset;
            for (within_offset = 0; within_offset < 8 && image[within_offset + i] != 0x20; within_offset += 1) {
                sub_tree->name[within_offset] = to_low_case(image[within_offset + i]);
            }
            sub_tree->name[within_offset] = '\0';

            if (image[i + 0xB] == FILE_ATTRIBUTE) {
                sub_tree->type = _FILE;
                for (within_offset = 8; within_offset < 0xB && image[within_offset + i] != 0x20; within_offset += 1) {
                    sub_tree->extension[within_offset - 8] = to_low_case(image[within_offset + i]);
                }
                sub_tree->extension[within_offset - 8] = '\0';
                sub_tree->file_size = parse_int(i + 28, 4);
            } else {
                sub_tree->extension[0] = '\0';
                sub_tree->type = _DIRECTORY;
            }
            sub_tree->first_clus = parse_int(i + 26, 2);
            sub_tree->first_child = NULL;
            sub_tree->nextSibling = NULL;
            if (tree->first_child == NULL) {
                tree->first_child = sub_tree;
            } else {
                struct Dir_Tree * current_tree = tree->first_child;
                while (current_tree->nextSibling != NULL) {
                    current_tree = current_tree->nextSibling;
                }
                current_tree->nextSibling = sub_tree;
            }
            if (sub_tree->type == _DIRECTORY) {
                build_dir_tree(sub_tree, false);
            }
        }

    }

}

int get_next_clus(int current_clus)
{
    size_t offset = bpb.RsvdSecCnt * bpb.BytsPerSec + (current_clus >> 1) * 3;
    if (current_clus % 2 == 0) {
        return parse_int(offset, 2) & 0xfff;
    } else {
        return parse_int(offset + 1, 2) >> 4;
    }
}

void show_prompt()
{
    _set_color(GREEN);
    _printf("iznauy: > ");
    _set_color(END);
}

void trim(char * str) {
    size_t length = strlen(str);
    while (str[length - 1] == '\n' || str[length - 1] == ' ' || str[length - 1] == '/') {
        str[--length] = 0;
    }
}

const struct Dir_Tree * _find_logic_file(char * path)
{
    if (path == NULL) // path = null means the root path
        return &dir_tree;
    char delims[] = "/";
    char * current_name = NULL;
    current_name = strtok(path, delims);
    const struct Dir_Tree * pre_tree = &dir_tree;
    const struct Dir_Tree * current_tree = dir_tree.first_child;
    while(current_name != NULL) {
        char full_name[DIR_NAME_SIZE + DIR_EXTENSION_SIZE];
        strncpy(full_name, current_tree->name, DIR_NAME_SIZE);
        if (current_tree->type == _FILE) {
            if (strlen(current_tree->extension) != 0) {
                strncat(full_name, ".", DIR_NAME_SIZE + DIR_EXTENSION_SIZE);
                strncat(full_name, current_tree->extension, DIR_EXTENSION_SIZE + DIR_NAME_SIZE);
            }
        }
        if (strcmp(current_name, full_name) != 0) {
            if (current_tree->nextSibling == NULL)
                return NULL;
            else {
                pre_tree = current_tree;
                current_tree = current_tree->nextSibling;
            }
        } else {
            pre_tree = current_tree;
            current_tree = current_tree->first_child;
            current_name = strtok(NULL, delims);
        }
    }
    return pre_tree;
}

void show_file_content(char * file)
{
    const struct Dir_Tree * tree = _find_logic_file(file);
    if (tree == NULL || tree->type == _DIRECTORY) {
        _set_color(RED);
        _printf("Error: ");
        _printf(file);
        _printf(" is not a valid file path.\n");
        _set_color(END);
        return;
    }
    size_t size = tree->file_size;
    int current_clus = tree->first_clus;
    size_t offset = bpb.RootEntCnt * LENGTH_PRE_ENTRY +
                    (bpb.RsvdSecCnt + bpb.NumFATs * bpb.FATSz16) * bpb.BytsPerSec
                    + (current_clus - 2) * bpb.SecPerClus * bpb.BytsPerSec;
    for (int within_count = 0; size > 0; size--, within_count++) {
        if (within_count == bpb.SecPerClus * bpb.BytsPerSec) {
            within_count = 0;
            current_clus = get_next_clus(current_clus);
            offset = bpb.RootEntCnt * LENGTH_PRE_ENTRY +
                     (bpb.RsvdSecCnt + bpb.NumFATs * bpb.FATSz16) * bpb.BytsPerSec
                     + (current_clus - 2) * bpb.SecPerClus * bpb.BytsPerSec;
        }
        int c = parse_int(offset + within_count, 1);
        my_putchar(c);
    }
    my_putchar('\n');
}


void _show_structure_entry(int _pre, const struct Dir_Tree * tree)
{   // all sub directories and file need to be print
    int pre_space = _pre << 2;
    tree = tree->first_child;
    while (tree) {
        for (int i = 0; i < pre_space; i++)
            my_putchar(' ');
        if (tree->type == _FILE) {
            // if it's a file
            _set_color(PURPLE);
            _printf(tree->name);
            if (strlen(tree->extension) != 0) {
                my_putchar('.');
                _printf(tree->extension);
                my_putchar('\n');
            }
            _set_color(END);
        } else {
            _set_color(BLUE);
            _printf(tree->name);
            my_putchar('/');
            my_putchar('\n');
            _show_structure_entry(_pre + 1, tree);
            _set_color(END);
        }
        tree = tree->nextSibling;
    }
}

void _show_file_entry(int _pre, const struct Dir_Tree * tree)
{
    if (tree->type == _FILE)
        return;
    int pre_space = _pre << 2;
    for (int i = 0; i < pre_space; i++)
        my_putchar(' ');
    int file_count = _count_file(tree);
    int dir_count = _count_dir(tree);
    _printf(tree->name);
    _printf(": ");
    char buff[10];
    itoa(file_count, buff);
    _printf(buff);
    if(file_count == 0 || file_count == 1) {
        _printf(" file, ");
    } else {
        _printf(" file(s), ");
    }
    itoa(dir_count, buff);
    _printf(buff);
    if (dir_count == 0 || dir_count == 1) {
        _printf(" dir\n");
    } else {
        _printf(" dir(s)\n");
    }
    tree = tree->first_child;
    while (tree != NULL) {
        _show_file_entry(_pre + 1, tree);
        tree = tree->nextSibling;
    }
}

void show_count(char * path)
{
    const struct Dir_Tree * tree = _find_logic_file(path);
    if (tree == NULL || tree->type == _FILE) {
        _set_color(RED);
        _printf("Error: ");
        _printf(path);
        _printf(" is not a valid directory.\n");
        _set_color(END);
        return;
    }
    _set_color(YELLOW);
    _show_file_entry(0, tree);
    _set_color(END);
}

void show_system_structure(char * path)
{
    const struct Dir_Tree * tree = _find_logic_file(path);
    if (!tree || tree->type == _FILE) {
        _set_color(RED);
        _printf("Error: ");
        _printf(path);
        _printf(" is not a valid directory.\n");
        _set_color(END);
        return;
    }
    _show_structure_entry(0, tree);
}


void itoa(int a, char * space)
{
    int count = 0;
    if (a == 0) {
        space[0] = '0';
        count += 1;
    }
    int t = a;
    while(t > 0) {
        int mod = t % 10;
        space[count++] = '0' + mod;
        t /= 10;
    }
    for (int i = 0; i < count / 2; i++) {
        char temp = space[i];
        space[i] = space[count - i - 1];
        space[count - i - 1] = temp;
    }
    space[count] = '\0';
}

int _count_file(const struct Dir_Tree * tree)
{
    int count = 0;
    tree = tree->first_child;
    while (tree) {
        if (tree->type == _FILE)
            count += 1;
        else
            count += _count_file(tree);
        tree = tree->nextSibling;
    }
    return count;
}

int _count_dir(const struct Dir_Tree * tree)
{
    int count = 0;
    tree = tree->first_child;
    while(tree) {
        if (tree->type == _DIRECTORY)
            count += 1 + _count_dir(tree);
        tree = tree->nextSibling;
    }
    return count;
}

void _set_color(const char * color)
{
    _printf(color);
}

void _printf(char * arg)
{
    char * current = arg;
    while (*current != 0) {
        my_putchar(*current);
        current++;
    }
}
