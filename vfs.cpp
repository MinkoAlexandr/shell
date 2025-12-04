#define FUSE_USE_VERSION 35

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <sys/types.h>
#include <cerrno>
#include <ctime>
#include <string>
#include <sys/wait.h>
#include <fuse3/fuse.h>
#include <pthread.h>
#include <fcntl.h>

#include "vfs.hpp"

int execute_command(const char* program, char* const arguments[]) {
    pid_t process_id = fork();

    if (process_id == 0) {
        execvp(program, arguments);
        _exit(127);
    }

    int exit_code;
    waitpid(process_id, &exit_code, 0);

    return WIFEXITED(exit_code) && WEXITSTATUS(exit_code) == 0 ? 0 : -1;
}

bool check_shell_compatibility(struct passwd* user_entry) {
    if (!user_entry || !user_entry->pw_shell) 
        return false;
    size_t shell_len = strlen(user_entry->pw_shell);
    return shell_len >= 2 && strcmp(user_entry->pw_shell + shell_len - 2, "sh") == 0;
}

int get_file_attributes(const char* path, struct stat* file_stats, struct fuse_file_info* file_info) {
    (void)file_info;
    memset(file_stats, 0, sizeof(struct stat));
    
    time_t current_time = time(nullptr);
    file_stats->st_atime = file_stats->st_mtime = file_stats->st_ctime = current_time;

    if (strcmp(path, "/") == 0) {
        file_stats->st_mode = S_IFDIR | 0755;
        file_stats->st_uid = getuid();
        file_stats->st_gid = getgid();
        return 0;
    }

    char user_name[256];
    char file_name[256];

    if (sscanf(path, "/%255[^/]/%255[^/]", user_name, file_name) == 2) {
        struct passwd* user_entry = getpwnam(user_name);

        if (strcmp(file_name, "id") != 0 &&
            strcmp(file_name, "home") != 0 &&
            strcmp(file_name, "shell") != 0) {
            return -ENOENT;
        }

        if (user_entry != nullptr) {
            file_stats->st_mode = S_IFREG | 0644;
            file_stats->st_uid = user_entry->pw_uid;
            file_stats->st_gid = user_entry->pw_gid;
            file_stats->st_size = 256;
            return 0;
        }
        return -ENOENT;
    }

    if (sscanf(path, "/%255[^/]", user_name) == 1) {
        struct passwd* user_entry = getpwnam(user_name);
        if (user_entry != nullptr) {
            file_stats->st_mode = S_IFDIR | 0755;
            file_stats->st_uid = user_entry->pw_uid;
            file_stats->st_gid = user_entry->pw_gid;
            return 0;
        }
        return -ENOENT;
    }

    return -ENOENT;
}

int list_directory_contents(
    const char* directory_path,
    void* buffer, 
    fuse_fill_dir_t add_entry, 
    off_t position, 
    struct fuse_file_info* file_info, 
    enum fuse_readdir_flags flags
) {
    (void)position;
    (void)file_info;
    (void)flags;

    add_entry(buffer, ".", nullptr, 0, FUSE_FILL_DIR_PLUS);
    add_entry(buffer, "..", nullptr, 0, FUSE_FILL_DIR_PLUS);

    if (strcmp(directory_path, "/") == 0) {
        struct passwd* user_entry;
        setpwent();

        while ((user_entry = getpwent()) != nullptr) {
            if (check_shell_compatibility(user_entry)) {
                add_entry(buffer, user_entry->pw_name, nullptr, 0, FUSE_FILL_DIR_PLUS);
            }
        }

        endpwent();
        return 0;
    }

    char user_name[256] = {0};
    if (sscanf(directory_path, "/%255[^/]", user_name) == 1) {
        struct passwd* user_entry = getpwnam(user_name);
        if (user_entry != nullptr) {
            add_entry(buffer, "id", nullptr, 0, FUSE_FILL_DIR_PLUS);
            add_entry(buffer, "home", nullptr, 0, FUSE_FILL_DIR_PLUS);
            add_entry(buffer, "shell", nullptr, 0, FUSE_FILL_DIR_PLUS);
            return 0;
        }
    }

    return -ENOENT;
}

int read_file_content(const char* file_path, char* data_buffer, size_t buffer_size, 
                     off_t read_offset, struct fuse_file_info* file_info) {
    (void)file_info;

    char user_name[256];
    char file_name[256];

    sscanf(file_path, "/%255[^/]/%255[^/]", user_name, file_name);

    struct passwd* user_entry = getpwnam(user_name);
    if (!user_entry) return -ENOENT;
    
    char content_buffer[256];
    content_buffer[0] = '\0';

    if (strcmp(file_name, "id") == 0) {
        snprintf(content_buffer, sizeof(content_buffer), "%d", user_entry->pw_uid);
    } else if (strcmp(file_name, "home") == 0) {
        snprintf(content_buffer, sizeof(content_buffer), "%s", user_entry->pw_dir);
    } else {
        snprintf(content_buffer, sizeof(content_buffer), "%s", user_entry->pw_shell);
    }

    size_t content_length = strlen(content_buffer);
    if (content_length > 0 && content_buffer[content_length-1] == '\n') {
        content_buffer[content_length-1] = '\0';
        content_length--;
    }

    if ((size_t)read_offset >= content_length) {
        return 0;
    }

    if (read_offset + buffer_size > content_length) {
        buffer_size = content_length - read_offset;
    }

    memcpy(data_buffer, content_buffer + read_offset, buffer_size);
    return buffer_size;
}

int create_user_directory(const char* directory_path, mode_t permissions) {
    (void)permissions;

    char user_name[256];

    if (sscanf(directory_path, "/%255[^/]", user_name) == 1) {
        struct passwd* user_entry = getpwnam(user_name);
        
        if (user_entry != nullptr) {
            return -EEXIST;
        }

        char* const cmd_args[] = {
            (char*)"adduser", 
            (char*)"--disabled-password",
            (char*)"--gecos", 
            (char*)"", 
            (char*)user_name, 
            nullptr
        };

        if (execute_command("adduser", cmd_args) != 0) return -EIO;
    }

    return 0;
}

int remove_user_directory(const char* directory_path) {
    char user_name[256];
    
    if (sscanf(directory_path, "/%255[^/]", user_name) == 1) {
        if (strchr(directory_path + 1, '/') == nullptr) {
            struct passwd* user_entry = getpwnam(user_name);
            
            if (user_entry != nullptr) {
                char* const cmd_args[] = {
                    (char*)"userdel", 
                    (char*)"--remove", 
                    (char*)user_name, 
                    nullptr
                };

                if (execute_command("userdel", cmd_args) != 0) return -EIO;
                return 0;
            }
            return -ENOENT;
        }
        return -EPERM;
    }
    return -EPERM;
}

struct fuse_operations fs_operations = {};

void setup_fs_operations() {
    fs_operations.getattr = get_file_attributes;
    fs_operations.readdir = list_directory_contents;
    fs_operations.mkdir   = create_user_directory;
    fs_operations.rmdir   = remove_user_directory;
    fs_operations.read    = read_file_content;
}

void* start_fs_thread(void* thread_arg) {
    (void)thread_arg;

    setup_fs_operations();

    int null_device = open("/dev/null", O_WRONLY);
    int saved_stderr = dup(STDERR_FILENO);
    dup2(null_device, STDERR_FILENO);
    close(null_device);

    char* fuse_arguments[] = {
        (char*)"kubsh_fs",
        (char*)"-f",
        (char*)"-odefault_permissions",
        (char*)"-oauto_unmount",
        (char*)"/opt/users"
    };

    int arg_count = sizeof(fuse_arguments) / sizeof(fuse_arguments[0]);

    fuse_main(arg_count, fuse_arguments, &fs_operations, nullptr);

    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);

    return nullptr;
}

void init_virtual_fs() {
    pthread_t fs_thread;
    pthread_create(&fs_thread, nullptr, start_fs_thread, nullptr);
    pthread_detach(fs_thread);
}
