/* Eddy Gao                  Serpens - Mapmaker                            ICS3U
This is a map editor that makes text files the main game, Serpens.cpp, can load.
Maps can include four different types of tiles:
    Empty tiles
    Walls
    A spawn tile
    Infertile tiles where food cannot spawn
*/
#include <allegro.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
const int scrx = 480, scry = 640;

//enum the tile types
enum {
	Empty = 0,
	Wall,
	Spawn,
	Infertile
};

//constants
const int gridWidth = 24, gridHeight = 30;

// global variables
BITMAP *buffer; 
int map[gridHeight][gridWidth];
int colors[4];
bool quit;

//prototype
void redraw(int x, int y);
void tofile();
void close();
void openfile();

int main() {
    //declare and initialize
    int brush=0;
    int x=0, y=0, spawnx=-1, spawny=-1, drawx, drawy, mdx, mdy, oldz=0, newz;
    bool draw=false;
    quit=false;
    BITMAP *selection, *infobar;
    
    //initialize allegro
    allegro_init();
	install_keyboard();
	install_mouse();
    
	set_color_depth(desktop_color_depth());
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, scrx, scry, 0, 0);
	set_close_button_callback(close);
	
	set_window_title("Serpens - Map Maker");
    colors[0] = makecol(200, 200, 200);
    colors[1] = makecol(10, 10, 10);
    colors[2] = makecol(50, 20, 230);
    colors[3] = makecol(200, 60, 20);

	buffer = create_bitmap(scrx, scry);
	selection = create_bitmap(20,20);
	infobar = load_bitmap("bar.bmp", 0);
	
	clear_to_color(selection, makecol(200, 20, 10));
	clear_to_color(buffer, colors[0]);
	
	blit(infobar, buffer, 0, 0, 0, 600, infobar->w, infobar->h);

    while (!key[KEY_ESC]&&!quit) {
        get_mouse_mickeys(&mdx, &mdy);
        if ((mdx!=0 || mdy!=0) && (mouse_x > 0 && mouse_y > 0 && mouse_x < 480 && mouse_y < 640)) {
            redraw(x, y);
            x = mouse_x/20;
            y = (mouse_y < 600) ? (mouse_y/20) : (29);
        }
        if (mouse_b & 1) {
            if (brush == 2) {
                if (spawnx != -1 && map[spawny][spawnx]==2) {
                    map[spawny][spawnx] = 0;
                    redraw(spawnx, spawny);
                }
                spawnx = x;
                spawny = y;
            }
            map[y][x] = brush;
        }
        if (mouse_b & 2) {
            if (brush==2) {
                
                allegro_message("You can't have more than one spawn point!");
            }
            else if (drawx!=x || drawy!=y) {               
                if(!draw){
                    //set initial coordinates
                    drawx = x;
                    drawy = y;
                }
                else 
                    //draw everything from initial coordinates all the way to current coordinates
                    for (int i = drawy; drawy<y ? i<=y: i>=y; drawy<y ? i++ : i--) {
                        for (int j = drawx; drawx<x ? j<=x: j>=x; drawx<x ? j++ : j--) {
                                map[i][j] = brush;
                                redraw(j, i);
                        }
                    }
                //toggle drawing
                draw=!draw;
                drawx = x;
                drawy = y;
            }
        }
        if (mouse_z!=oldz) {
            newz = mouse_z;
            brush+=(newz-oldz);
            if (brush>3) brush -= 4;
            if (brush<0) brush+=4;
            if (brush == 2 && draw) brush+=(newz-oldz);
            oldz = newz;
        }
    	if (keypressed()) {
    		int key = readkey();
    		switch ((key >> 8) & 0xFF) {
                //navigation
    			case KEY_DOWN:
                    redraw(x, y);
                    if (y+1<gridHeight) y++;
    				break;
    			case KEY_UP:
                    redraw(x, y);
                    if (y-1>=0) y--;
    				break;
    			case KEY_RIGHT:
                    redraw(x, y);
                    if (x+1<gridWidth) x++;
    				break;
    			case KEY_LEFT:
                    redraw(x, y);
    				if (x-1>=0) x--;
    				break;
    			//change brush
    			case KEY_Z:
                    brush++;
                    if (brush == 2 && draw) brush++;
                    if (brush > 3) brush = 0;
                    break;
                //draw
                case KEY_SPACE:
                    if (brush == 2) {
                        if (spawnx != -1 && map[spawny][spawnx]==2) {
                            map[spawny][spawnx] = 0;
                            redraw(spawnx, spawny);
                        }
                        spawnx = x;
                        spawny = y;
                    }
                    map[y][x] = brush;
                    break;
                //draw large blocks
                case KEY_X:
                    if (brush==2) {
                        allegro_message("You can't have more than one spawn point!");
                        break;
                    }
                        
                    if(!draw){
                        //set initial coordinates
                        drawx = x;
                        drawy = y;
                    }
                    else 
                        //draw everything from initial coordinates all the way to current coordinates
                        for (int i = drawy; drawy<y ? i<=y: i>=y; drawy<y ? i++ : i--) {
                            for (int j = drawx; drawx<x ? j<=x: j>=x; drawx<x ? j++ : j--) {
                                    map[i][j] = brush;
                                    redraw(j, i);
                            }
                        }
                    //toggle drawing
                    draw=!draw;
                    break;
                case KEY_S:
                    tofile();
                    clear_keybuf();
                    break;
                case KEY_D:
                    openfile();
                    clear_keybuf();
                    break;
                case KEY_H:
                     allegro_message("'z' to change brushes.\nSpace to draw.\n'x' to draw a block.\n's' to save, and 'd' to load a file.");
        	}
        }
        
        //change selection outline colour to reflect the value of draw
        clear_to_color(selection, !draw ? makecol(20, 10, 200) : makecol(200, 20, 10));
        
        //change selection colour
        rectfill(selection, 2, 2, 17, 17, colors[brush]);
        
        //draw initial coordinate location
        if (draw) {
            draw_sprite(buffer, selection, drawx*20, drawy*20);
        }
        
        //change selection outline color once more
        clear_to_color(selection, draw ? makecol(20, 10, 200) : makecol(200, 20, 10));
        rectfill(selection, 2, 2, 17, 17, colors[brush]);
        
        draw_sprite(buffer, selection, x*20, y*20);
        

		blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);

		//rest(10);
	}

	return 0;
}
END_OF_MAIN()


void redraw(int x, int y) {
    rectfill(buffer, x*20,y*20, x*20 + 19, y*20 + 19, colors[map[y][x]]);
}

void tofile() {
    char name[20];
    char tmp[20];
    printf("Enter a name for this file: ");
    fflush(stdin);
    gets(tmp);
    sprintf(name, "maps/%s", tmp);
    FILE *fptr = fopen(name, "w");
    if (!fptr) { 
        allegro_message("oh shit"); 
        exit(1);
    }
    
    for (int i=0; i<gridHeight; i++) {
        for (int j=0; j<gridWidth; j++) {
            switch(map[i][j]) {
                case 0:
                    fprintf(fptr, ".");
                    break;
                case 1:
                    fprintf(fptr, "#");
                    break;
                case 2:
                    fprintf(fptr, "s");
                    break;
                case 3:
                    fprintf(fptr, "x");
                    break;
            }
        }
        fprintf(fptr, "\n");
    }
    fclose(fptr);
}

bool loadMap(char* input) {
    char tile;
    FILE* inputfile = fopen(input, "r");
    
    //make sure that we get a valid file
    if (inputfile==NULL) {
        return false;
    }
    
    //read in map data
    for (int y=0; y<30; y++) {
        for (int x=0; x<24; x++) {
            if (fscanf(inputfile, "%c", &tile) != 1) {
                fclose(inputfile);
                return false;
            }
            switch (tile) {
                case '\r':
                case '\n':
                    x--;
                    continue;
                case 's':
                    map[y][x] = Spawn;
                    break;
                case '.':
                    map[y][x] = Empty;
                    break;
                case '#':
                    map[y][x] = Wall;
                    break;
                case 'x':
                    map[y][x] = Infertile;
                    break;
                default:
                    map[y][x] = Empty;
            }
            redraw(x, y);
        }
    }

    fclose(inputfile);
    return true;
}

void openfile() {
    char tmp[30];
    char input[30];
    
    do {
        printf("Please enter a file to open or 'quit': ");
        scanf("%s", tmp);
        if (strcmp(tmp, "quit") == 0) return;
        sprintf(input, "maps/%s", tmp);
    }
    while (!loadMap(input));
    
}
            
//this is called when the close button is clicked
void close() {
    quit=true;
}
