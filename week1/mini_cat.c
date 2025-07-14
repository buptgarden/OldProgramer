#include<stdio.h>
#include<stdlib.h>

#define BUFFER_SIZE 1024

void print_usage(const char* program_name){
    printf("Usage: %s [file1] [file2] ...\n", program_name);
    printf("       %s  (read from stdin)\n", program_name);
}

int cat_file(const char* filename){
    FILE* file;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    file = fopen(filename, "r");
    if (file == NULL){
        perror(filename);
        return 1;
    }

    while((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0){
        fwrite(buffer, 1, bytes_read, stdout);
    }

    if (ferror(file)){
        perror(filename);
        fclose(file);
        return 1;
    }

    fclose(file);
    return 0;
}

int cat_stdin() {
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    while((bytes_read = fread(buffer, 1, BUFFER_SIZE, stdin)) > 0){
        fwrite(buffer, 1, bytes_read, stdout);
    }

    if (ferror(stdin)){
        perror("stdin");
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    int exit_code = 0;

    if (argc == 1){
        return cat_stdin();
    }

    for (int i = 1; i < argc; i++){
        if (argv[i][0] == '-' && argv[i][1] == '\0'){
            if (cat_stdin() != 0){
                exit_code = 1;
            }
        }else {
            if (cat_file(argv[i]) != 0){
                exit_code = 1;
            }
        }
    }
    return exit_code;
}