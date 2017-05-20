#include "../../include/syscalls.h"
#include "../../include/termfeatures.h"
#include "../../include/loadfont.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>

unsigned vspace(int fd, unsigned size, unsigned char color) {
    char line[3*1024];
    for (unsigned i=0;i<1024;i++) {
        line[3*i] = color;
        line[3*i+1] = color;
        line[3*i+2] = color;
    }
    for (unsigned c=0;c<size;c++) {
        _write(fd, line, 3*1024);
    }
    return size;
}


unsigned hspace(unsigned char* buffer, int h, int size, int offset, unsigned char color) {
    for(int j = 0; j<size; j++) {
        for(int i= 0; i<h; i++) {
            buffer[3*(i*1024+j+offset)] = color;
            buffer[3*(i*1024+j+offset)+1] = color;
            buffer[3*(i*1024+j+offset)+2] = color;
        }
    }
    return size;
}


char drawcharacter(unsigned char* buffer, unsigned char* font, int h, int w, int offset, char character, unsigned char ctext, unsigned char cback) {
    unsigned pos = h*w*character;
    unsigned char temp;
    for(int i = 0; i<h; i++) {
        for(int j = 0; j<w; j++) {
            if(font[3*(pos + i*w + j)]) {
                temp = ctext;
            }
            else {
                temp = cback;
            }
            buffer[(offset + i*1024 + j)*3] = temp;
            buffer[(offset + i*1024 + j)*3+1] = temp;
            buffer[(offset + i*1024 + j)*3+2] = temp;
        }
    }
    return w;
}


unsigned line(char* text, int fd, unsigned char* font, int h, int w, unsigned char ctext,unsigned char cback) {
    unsigned drawnlines = 0;
   //drawnlines += vspace(fd,10,25);
    unsigned drawncol = 0;

    unsigned char buff[3*1024*h];
    drawncol += hspace(buff,h,5,0,cback);
    while(*text) {
        drawncol += drawcharacter(buff,font,h,w,drawncol,*text++,ctext,cback);
        drawncol += hspace(buff,h,1,drawncol,cback);
    }
    hspace(buff,h,1024-drawncol,drawncol,cback);
    _write(fd,buff,3*1024*h);
    drawnlines += h;
   // drawnlines += vspace(fd,10,25);
    return drawnlines;
}

#include "../../include/termfeatures.h"

bool ischar(char i) {
	return (i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z');
}

int main() {
	printf("Starting miniterm and launching WESH\n");
	int pts_output_pipe[2];
	int pts_input_pipe[2];
	pipe(pts_output_pipe);
	pipe(pts_input_pipe);

	int pid = _fork();

	if (pid == 0) {
		dup2(pts_output_pipe[1], 2);
		dup2(pts_output_pipe[1], 1);
		dup2(pts_input_pipe[0], 0);
		_ioctl(1, IOCTL_BLOCKING, 1);
		_ioctl(0, IOCTL_BLOCKING, 1);

		char* argv_wesh[] = {"/bin/wesh",NULL};
		char* envp_wesh[] = {"PATH=/bin/",NULL};
		_execve("/bin/wesh", argv_wesh, envp_wesh);
	} else {
		int fd = _openat(AT_FDCWD, "/dev/fb",O_RDWR);
		int write_to_term_fd 	= pts_input_pipe[1];
		int read_from_term_fd 	= pts_output_pipe[0];

		_ioctl(fd, FB_OPEN, 0);
		_ioctl(fd, FB_SHOW, 0);
		_ioctl(fd, FB_BUFFERED, 0);

		unsigned height;
		unsigned width;
		unsigned char* font = NULL;

		unsigned screen_height = _ioctl(fd, FB_HEIGHT, 0);
		unsigned screen_width = _ioctl(fd, FB_WIDTH, 0);

		loadFont("/fonts/font2.bmp",&width,&height,&font);


		unsigned character = 2;
		unsigned char buff[3*1024*height];
		printf("%d %d\n", height, width);
		for(int i = 0; i<height; i++) {
			for(int j = 0; j<width; j++) {
				buff[3*(i*1024 + j)] = *(font + 3*(character*height*width + i*width + j));
				buff[3*(i*1024 + j)+1] = *(font + 3*(character*height*width + i*width + j)+1);
				buff[3*(i*1024 + j)+2] = *(font + 3*(character*height*width + i*width + j)+2);
			}
		}
		_write(fd,buff,3*1024*height);
		int n;
		char buffer[256];

		int cursor_x = 0;
		int cursor_y = 0;

		int cursor_x_bak = 0;
		int cursor_y_bak = 0;


		int n_chars = screen_width/width-15;
		int n_rows = 40;

		printf("%d\n", n_chars);
		perror("miniterm");

		char* lines[n_rows];
		for (int i=0;i<n_rows;i++) {
			lines[i] = malloc(n_chars+1);
			for (int j=0;j<n_chars-1;j++) {
				lines[i][j] = ' ';
			}
			lines[i][n_chars-1] = 0;
		}


		_ioctl(0, IOCTL_BLOCKING, 0);
		_ioctl(read_from_term_fd, IOCTL_BLOCKING, 0);


		term_raw_enable(true);
		while (1) {
			for(int i=0;i<1000000;i++);
			if (_waitpid(pid, NULL, 1) > 0) {
				//printf("Son exited\n");
				break;
			}

			n = _read(0, buffer, 256); // Read serial input.
			if (n > 0) {
				printf("main read %d\n", n);
				_write(write_to_term_fd, buffer, n);
			}

			n = _read(read_from_term_fd, buffer, 256);
			if (n > 0) {
				printf("Shell wrote %d chars. %d\n",n,cursor_y);
				_lseek(fd, cursor_y*screen_width*3*height, SEEK_SET);
				for (int i=0;i<n;i++) {
					char c = buffer[i];
					if (c == '\n' || c == '\r') {
						printf("%c", c);
					}
					else if (c == '\033') { // escape sequence.
						i++;
						c = buffer[i];
						if (c == '[') {  // [i1;i2;i3;i4res
							int params[4];
							int r=0;
							char* pos=buffer+i+1;
							char* end=pos;

							while (!ischar(end[0])) {
								end++;
							}

							while(!ischar(pos[0]) && sscanf(pos, "%d", params+r)>0) {
								r++;
								if (strchr(pos, ';') != NULL) {
									pos = 1+strchr(pos,';');
								} else {
									pos = end;
								}

								if (pos>end) {
									pos = end;
								}
							}

							char status_report[16];
							switch (pos[0]) {
								case 'D':
									cursor_x--;
									break;
								case 'n': // Device status deport
									_write(write_to_term_fd, status_report,
										sprintf("\033[%d;%dR", status_report, cursor_y, cursor_x));
									break;
								case 's':
									printf("backing up\n");
									cursor_x_bak = cursor_x;
									cursor_y_bak = cursor_y;
									break;
								case 'u':
									printf("restored\n");
									cursor_x = cursor_x_bak;
									cursor_y = cursor_y_bak;
									break;
								case 'J':
									if (params[0] == 0) { // From cursor to end
										printf("Not implemented.\n");
									} else if (params[0] == 1) { // From cursor to beg
										printf("Not implemented.\n");
									} else if (params[0] <= 3) { // Clear screen
										int r = _lseek(fd, 0, SEEK_SET);
										//printf("%d\n",r);
										printf("> clear\n");
										for (int j=0;j<n_rows;j++) {
											printf("%d\n",fd);
											for (int i=0;i<n_chars-1;i++)
												lines[j][i] = ' ';
											lines[j][n_chars-1] = 0;
											line(lines[j], fd, font, height, width, 255, 0);
										}
										printf("> cleared\n");
										_lseek(fd, 0, SEEK_SET);
										cursor_x = 0;
										cursor_y = 0;
									} else {
										printf("Not implemented.\n");
									}
									break;
								case 'K':
									if (params[0] == 0) {
										printf("Not implemented.\n");
									} else if (params[0] == 1) {
										printf("Not implemented.\n");
									} else if (params[0] == 2) {
										for (int i=0;i<n_chars;i++)
											lines[cursor_y][i] = 0;
										_lseek(fd, cursor_y*screen_width*3*height, SEEK_SET);
										line(lines[cursor_y], fd, font, height, width, 255, 0);
									}
									break;
								case 'f':
								case 'H':
									cursor_y = params[0];
									cursor_x = params[1];
									break;
								case 'm':
									//Not implemented
									break;
								default:
									printf("Control char not implemented: %c %d\n", pos[0], params[0]);
									break;
							}

							i = (int) (pos-buffer);


							if (cursor_x < 0)
								cursor_x++;
							if (cursor_y < 0)
								cursor_y++;
							if (cursor_x >= n_chars)
								cursor_x--;
						} else {
							printf("Failed reading control sequence: %c\n", buffer[i]);
						}
					} else {
						//printf("%d %d|%c|\n", cursor_x, cursor_y, buffer[i]);
						lines[cursor_y][cursor_x] = buffer[i];
						cursor_x++;
					}

					if (c == '\r' || c == '\n' || cursor_x == n_chars) {
						_lseek(fd, cursor_y*screen_width*3*height, SEEK_SET);
						line(lines[cursor_y], fd, font, height, width, 255, 0);

						cursor_y++;
						cursor_x=0;
					}

					if (cursor_y == n_rows) {
						cursor_y = n_rows/2;
						_lseek(fd, 0, SEEK_SET);
						for (int i=0;i<n_rows/2;i++) {
							memcpy(lines[i], lines[n_rows/2+i], n_chars);
							line(lines[i], fd, font, height, width, 255, 0);
						}

						for (int i=n_rows/2;i<n_rows;i++) {
							memset(lines[i], ' ', n_chars);
							line(lines[i], fd, font, height, width, 255, 0);
						}
						_lseek(fd, cursor_y*screen_width*3*height, SEEK_SET);
					}
				}

				if (cursor_x > 0) {
					_lseek(fd, cursor_y*screen_width*3*height, SEEK_SET);
					line(lines[cursor_y], fd, font, height, width, 255, 0);
				}
			}

		}
		_ioctl(fd, FB_CLOSE, 0);
		_close(fd);
	}
}
