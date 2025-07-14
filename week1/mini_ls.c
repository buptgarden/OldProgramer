#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>

void print_permissions(mode_t mode)
{
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

void print_file_info(const char *filename, const char *path, int show_details)
{
    struct stat file_stat;
    char full_path[1024];

    // 构建完整路径
    if (strcmp(path, ".") == 0)
    {
        strcpy(full_path, filename);
    }
    else
    {
        snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);
    }

    if (stat(full_path, &file_stat) == -1)
    {
        perror("stat");
        return;
    }

    if (show_details)
    {
        // 显示详细信息 (类似 ls -l)
        print_permissions(file_stat.st_mode);
        printf(" %2ld", file_stat.st_nlink);

        // 用户名和组名
        struct passwd *pwd = getpwuid(file_stat.st_uid);
        struct group *grp = getgrgid(file_stat.st_gid);
        printf(" %s %s", pwd ? pwd->pw_name : "unknown", grp ? grp->gr_name : "unknown");

        // 文件大小
        printf(" %8ld", file_stat.st_size);

        // 修改时间
        char time_str[64];
        struct tm *time_info = localtime(&file_stat.st_mtime);
        strftime(time_str, sizeof(time_str), "%b %d %H:%M", time_info);
        printf(" %s", time_str);

        // 文件名
        printf(" %s", filename);

        // 如果是目录，添加 / 标识
        if (S_ISDIR(file_stat.st_mode))
        {
            printf("/");
        }
        printf("\n");
    }
    else
    {
        // 简单显示文件名
        printf("%s", filename);
        if (S_ISDIR(file_stat.st_mode))
        {
            printf("/");
        }
        printf("  ");
    }
}

void mini_ls(const char *path, int show_all, int show_details)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(path);
    if (dir == NULL)
    {
        perror("opendir");
        return;
    }

    printf("Directory: %s\n", path);
    if (show_details)
    {
        printf("total files in directory:\n");
    }

    while ((entry = readdir(dir)) != NULL)
    {
        // 跳过隐藏文件（除非使用 -a 选项）
        if (!show_all && entry->d_name[0] == '.')
        {
            continue;
        }

        print_file_info(entry->d_name, path, show_details);
    }

    if (!show_details)
    {
        printf("\n");
    }

    closedir(dir);
}

void print_usage(const char *program_name)
{
    printf("Usage: %s [options] [directory]\n", program_name);
    printf("Options:\n");
    printf("  -a    显示所有文件（包括隐藏文件）\n");
    printf("  -l    显示详细信息\n");
    printf("  -h    显示帮助信息\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s           # 列出当前目录\n", program_name);
    printf("  %s /tmp      # 列出 /tmp 目录\n", program_name);
    printf("  %s -l        # 详细列出当前目录\n", program_name);
    printf("  %s -a -l .   # 详细列出当前目录的所有文件\n", program_name);
}

int main(int argc, char *argv[])
{
    int show_all = 0;
    int show_details = 0;
    const char *directory = ".";

    // 解析命令行参数
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            for (int j = 1; argv[i][j] != '\0'; j++)
            {
                switch (argv[i][j])
                {
                case 'a':
                    show_all = 1;
                    break;
                case 'l':
                    show_details = 1;
                    break;
                case 'h':
                    print_usage(argv[0]);
                    return 0;
                default:
                    fprintf(stderr, "Unknown option: -%c\n", argv[i][j]);
                    print_usage(argv[0]);
                    return 1;
                }
            }
        }
        else
        {
            directory = argv[i];
        }
    }

    mini_ls(directory, show_all, show_details);

    return 0;
}