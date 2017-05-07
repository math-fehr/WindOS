#include "wesh.h"

void scheduler_handler() {
  char* token = strtok(NULL," ");
  if (token == NULL) {
    serial_write("[SHED] Unrecognized command\n");
    return;
  }

  if(!strcmp("display",token)) {
      serial_write("[SHED] Displaying active processes\n");
      for(int i = 0; i<get_number_active_processes(); i++) {
          kernel_printf("pid: %d    dummy: %d",get_active_processes()[i],get_process_list()[get_active_processes()[i]]->dummy);
          if(get_process_list()[get_active_processes()[i]]->status == status_zombie) {
              kernel_printf(" zombie");
          }
          serial_newline();
      }
  }
  else if(!strcmp("create",token)) {
      int number = atoi(strtok(NULL," "));
      int i = 0;
      bool error = false;
      while(i < number && !error) {
        /*  int temp = create_process();
          if(temp == -1) {
              error = true;
          }
          i++;*/
      }
      if(error) {
          serial_write("[SHED] Limit of processes reached");
          serial_newline();
      }
  }
  else if(!strcmp("kill",token)) {
      int number = atoi(strtok(NULL," "));
      int temp= kill_process(number,0);
      if(temp == -1) {
          serial_write("[SHED] Can't kill that process");
          serial_newline();
      }
  }
  else if(!strcmp("step",token)) {
      int number = atoi(strtok(NULL," "));
      bool error = false;
      for(int i = 0; i<number && !error; i++) {
          int pid = get_next_process();
          if(pid == -1) {
              error = true;
          }
          else {
              get_process_list()[pid]->dummy++;
          }
      }
  }
  else {
      serial_write("[SHED] Unrecognized command");
      serial_newline();
  }
}

static char position[256];

void tree(char* pos, int depth) {
  vfs_dir_list_t* result = vfs_readdir(pos);
  while(result != 0) {
    if (strcmp(result->name,".") && strcmp(result->name,"..")) {
      for (int i=0;i<depth;i++) {
        kernel_printf("| ");
      }

      if (S_ISDIR(result->inode->st.st_mode))
        kernel_printf("|-\e[1m%s\e[0m\n",result->name);
      else
        kernel_printf("|-%s\n",result->name);


      if (S_ISDIR(result->inode->st.st_mode) && (strcmp(result->name,"lost+found"))) {
        char* buf = malloc(strlen(pos) + strlen(result->name) + 2);
        strcpy(buf,pos);
        strcat(buf,result->name);
        strcat(buf,"/");
        tree(buf, depth+1);
        free(buf);
      }
    }
    vfs_dir_list_t* prev = result;
    result = result->next;
    free(prev->inode);
    free(prev->name);
    free(prev);
  }
}

void vfs_handler() {
    char* token = strtok(NULL," ");
    if (token == NULL) {
        serial_write("[VFS] Unrecognized command\n");
        return;
    }

    if(!strcmp("ls",token)) {
        vfs_dir_list_t* result = vfs_readdir(position);

        while(result != 0) {
            perm_str_t res = vfs_permission_string(result->inode->st.st_mode);
            kernel_printf("%# 6d    %s   %# 7d   %s\n", result->inode->st.st_ino, res.str, result->inode->st.st_size,result->name);
            vfs_dir_list_t* prev = result;
            result = result->next;
            free(prev->inode);
            free(prev->name);
            free(prev);
        }
    } else if(!strcmp("tree",token)) {
        tree(position, 0);
    } else if(!strcmp("cd",token)) {
        token = strtok(NULL, " ");

        if (token == NULL) {
            strcpy(position,"/");
        } else if(token[0] == '/') { // Absolute path
            inode_t* data = vfs_path_to_inode(token);
            if (data == 0) {
                kernel_printf("cd: %s: no such file or directory.\n", token);
            } else if (S_ISDIR(data->st.st_mode)) {
                free(data);
                strcpy(position, token);
            } else {
                free(data);
                kernel_printf("cd: %s: this is not a directory.\n", token);
            }
        } else {
            if (!strcmp(token,".")) {
            } else if (!strcmp(token, "..")) {
                if (strcmp(position, "/") != 0) {
                    char obj[255];
                    strcpy(obj, position);
                    int n = strlen(obj);
                    int k;
                    for (k=n-2; obj[k] != '/'; k--) {}
                    obj[k+1] = 0;

                    inode_t *data = vfs_path_to_inode(obj);
                    if (data == 0) {
                        kernel_printf("cd: %s: no such file or directory.\n", token);
                    } else if (S_ISDIR(data->st.st_mode)) {
                        free(data);
                        strcpy(position, obj);
                    } else {
                        free(data);
                        kernel_printf("cd: %s: This is not a directory.\n", obj);
                    }
                }
            } else {
                char obj[255];
                strcpy(obj, position);
                strcat(obj, token);

                inode_t* data = vfs_path_to_inode(obj);
                if (data == 0) {
                    kernel_printf("cd: %s: no such file or directory.\n", token);
                } else if (S_ISDIR(data->st.st_mode)) {
                    free(data);
                    strcpy(position, obj);
                } else {
                    free(data);
                    kernel_printf("cd: %s: This is not a directory.\n", obj);
                }
            }

            int n = strlen(position);
            if (position[n-1] != '/') {
                position[n] = '/';
                position[n+1] = 0;
            }
        }
    } else if(!strcmp("pwd",token)) {
        kernel_printf("%s\n", position);
    } /*else if(!strcmp("read",token)) {
        char* token = strtok(NULL, " ");
        char obj[255];
        strcpy(obj, position);
        strcat(obj, token);

        int fd = vfs_fopen(obj);
        kernel_printf("fopen => %d: \n", fd);
        char buffer[255];
        kernel_printf("fread:\n");
        while (vfs_fread(fd,buffer,254) > 0) {
            kernel_printf(buffer);
        }
        kernel_printf("\n");
    } */else if(!strcmp("touch",token)) {
        char* name = strtok(NULL," ");
        vfs_mkfile(position, name, 0x7FF);
    } else if(!strcmp("mkdir",token)) {
        char* name = strtok(NULL," ");
        vfs_mkdir(position, name, 0x7FF);
    } else if(!strcmp("rm", token)) {
        char* name = strtok(NULL," ");
        vfs_rm(position, name);
    } else if(!strcmp("exec", token)) {
        char* name = strtok(NULL," ");
        char obj[255];
        strcpy(obj, position);
        strcat(obj, name);
        process* p = process_load(obj, NULL, NULL);
        p->fd[0].inode      = vfs_path_to_inode("/dev/serial");
        p->fd[0].position   = 0;
        p->fd[1].inode      = vfs_path_to_inode("/dev/serial");
        p->fd[1].position   = 0;
        sheduler_add_process(p);
        if (p != NULL) {
        //    process_switchTo(p);
        }
    } else {
        serial_write("[VFS] Unrecognized command\n");
    }
}



void wesh() {
  char command[256];
  for (int i=0;i<255;i++){
    position[i] = 0;
    command[i]=0;
  }
  position[0] = '/';

  while(1) {
    kernel_printf("%s$ ", position);
    serial_readline(command, 255);
    char* token = strtok(command," ");
    if(!strcmp("s",token)) {
      scheduler_handler();
    }
    else if(!strcmp("f",token)) {
      vfs_handler();
    }
    else if(!strcmp("h",token) || !strcmp("help",token)) {
      kernel_printf("#Wind Experimental SHell:\n# - s: scheduler functions\n# - f: vfs functions\n");
    } else {
      kernel_printf("Unrecognized command.\n");
    }
  }
}
