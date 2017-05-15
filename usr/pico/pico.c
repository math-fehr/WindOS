#include "pico.h"

extern int argc;
extern char** argv;
extern char* environ[];


char** 	file;
char* 	path;
int 	nlines;

int 	row;
int 	col;


bool 	open_buffer(char* new_path) {
	FILE* fd = fopen(new_path, "rb");
	if (fd == NULL) {
		return false;
	}

	if (path != NULL)
		free(path);

	path = new_path;

	int size = _lseek(fd->_file,1,SEEK_END);
	_lseek(fd->_file,0,SEEK_SET);
	//printf("file of size %d\n", size);
	char* buf = malloc(size+1);
	fread(buf, 1, size+1, fd);

	nlines = 0;
	for (int i=0;i<size;i++) {
		if (buf[i] == '\n') {
			nlines++;
		}
	}

	nlines++;

	file = malloc(sizeof(char*)*(nlines+1));
	int position = 0;
	int dep = 0;
	for (int i=0;i<size;i++) {
		if (buf[i] == '\n') {
			file[position] = malloc(i-dep+1);
			memcpy(file[position], &buf[dep], i-dep);
			file[position][i-dep] = 0;
			dep = i+1;
			position++;
		}
	}

	file[position] = malloc(size-1-dep+1);
	memcpy(file[position], &buf[dep], size-dep);
	file[position][size-dep] = 0;
 	file[position+1] = 0;
	free(buf);
}



bool save_buffer() {
    if(path == NULL) {
        return false;
    }
    FILE* fd = fopen(path, "w+");
    char* jump_line = "\n";
    for(int i = 0; i<nlines; i++) {
        fwrite(file[i],1,strlen(file[i]),fd);
        fwrite(jump_line,1,1,fd);
    }
    fclose(fd);
    return true;
}



int main() {
	term_clear();
	fflush(stdout);
	term_get_size(&row, &col);
	
	fflush(stdout);
	term_raw_enable(true);

	bool empty_buffer = true;
	char* c_path;

	if (argc >= 2) {
		c_path = malloc(strlen(argv[1])+1);
		strcpy(c_path,argv[1]);
		if (open_buffer(c_path)) {
			empty_buffer = false;
		}
	} else {
		c_path = malloc(1);
		c_path[0] = 0;
	}
	if (empty_buffer) {
		path = c_path;
		// empty buffer.
		nlines = 1;
		file = malloc(sizeof(char*)*2);
		file[0] = malloc(1);
		file[0][0] = 0;
		file[1] = 0;
	}

	topbar_draw();
	editor_draw();
	term_move_cursor(3, 5);
	fflush(stdout);
	while(1) {
		char c = getc(stdin);

		if (c == 0x7F) { // delete
			editor_delete();
		} else if (c == 0x1B) { // escape code
			c = getc(stdin); // normally [
			c = getc(stdin);
			editor_move(c);
		} else if (c == 0x18) { // ctrl-X
			term_clear();
			term_move_cursor(1, 1);
			term_raw_enable(false);
			return 0;
		} else if (c == 0x13) { // ctrl-S
            save_buffer();
        }else { // basic char
			editor_putc(c);
		}
	}
}
