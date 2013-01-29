/* Eddy Gao                         Serpens                                ICS3U
This game is based on the classic arcade game, "Snake". The player controls a
hungry snake, whose tail grows whenever it eats. If the snake collides with
itself or the wall, the game is over.
Additional features include:
    Ability to load custom maps 
    Separate map editor tool to create maps
    Special foods with decaying benefit
    Fancy centred viewmode
*/

#include <allegro.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#define BACKCOL makecol(color[0], color[1], color[2])
#define SNAKECOL makecol((color[0] + 128) % 256, (color[1] + 128) % 256, (color[2] + 128) % 256)
#define FOODCOL makecol((color[0] + 128) % 256, 255 - color[1], color[2])
#define MSGCOL makecol(255 - color[0], 255 - color[1], 255 - color[2])

//structure to store a snake segment, this is used in a linked list to represent the snake
typedef struct snake {
    int x;
    int y;
    struct snake *next;
} snake;

//screen constants
const int scrx = 480, scry = 640;
const int gridWidth = 24, gridHeight = 30;

//types of grid squares
enum GridSquare {
    Empty = 0,
    Snake,
    Food,
    Infertile,
    Special
};

BITMAP *buffer; // game buffer

//grid stores current state, map stores the initial state
GridSquare grid[gridHeight][gridWidth];
GridSquare  map[gridHeight][gridWidth];

//global variables
bool quit = false;
int spawnx, spawny, color[3];

//prototyping 
void placefood(int *foodx, int *foody, int *specx, int *specy);
snake *getElem(snake *L, int i);
void sappend(snake** L, int x, int y, snake *newNode);
void lose(int score, int color[]);
bool loadMap(const char* lastFile);
void reset(const char* lastFile, int *foodx, int *foody, int *posx, int *posy, int *velx, int *vely, int *score, int *extend, snake **tail, int *speed, int *numElem);
void close();
void load(char* out);
void game(char *lastFile);
void menu();


int main() {
    //seed RNG
    srand(time(0));
    
    //a bunch of allegro initialization routines
    allegro_init();
    install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL);
    install_keyboard();
    install_mouse();
    show_mouse(screen);
    set_color_depth(desktop_color_depth());
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, scrx, scry, 0, 0);
    set_close_button_callback(close);
    
    //play bg music
    MIDI *music = load_midi("tetris.mid");
    play_midi(music, 1);
    
    // Set the window title
    set_window_title("Serpens");
    
    buffer = create_bitmap(scrx, scry-40);
    
    menu();

    //clean up
    destroy_midi(music);
    destroy_bitmap(buffer);
    return 0;
}
END_OF_MAIN()

//this is called when the close button is clicked
void close() {
    quit=true;
	//quit some menus
	simulate_keypress(KEY_ESC << 8);
}

//This function replaces the food, and has a chance of spawning a special food
void placefood(int *foodx, int *foody, int *specx, int *specy) {
    do {
        *foodx = rand() % gridWidth;
        *foody = rand() % gridHeight;
    } while (grid[*foody][*foodx] != Empty);
    grid[*foody][*foodx] = Food;

    //20% chance of special food, and only if it isn't already there
    if (rand()%10<2 && *specx == -1) {
        do {
            *specx = rand() % gridWidth;
            *specy = rand() % gridHeight;
        } while (grid[*specy][*specx] != Empty);
        grid[*specy][*specx] = Special;
    }
}

//appends a snake element to the end of the linked list, or the beginning of the snake
void sappend(snake** L, int x, int y, snake *newNode) {
    if (newNode == NULL) newNode = (snake*)malloc(sizeof(snake));
    snake* temp = *L;
    if (temp != NULL) {
        while (temp->next != NULL) {
            temp = temp->next;
        }
        // Creates a node at the end of the list
        temp->next = newNode;
        temp = temp->next;
    } else {
        temp = *L = newNode;
    }
    temp->next = NULL;
    temp->x = x;
    temp->y = y;
}

//prints loss message and keeps it there
void lose(int score, int color[]) {
    textprintf_centre_ex(screen, font, scrx / 2, 200, MSGCOL, -1, "You Lose! Final Score: %d", score);
    textprintf_centre_ex(screen, font, scrx / 2, 250, MSGCOL, -1, "Press Space to Restart");
    while (true) {
        int key = readkey();
        if ((key & 0xFF) == ' ' || ((key >> 8) & 0xFF) == KEY_ESC) break;
    }
}

//handles map loading
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
                spawnx=x;
                spawny=y;
                //no break, we want to set it to empty as well
            case '.':
                grid[y][x] = Empty;
                map[y][x] = Empty;
                break;
            case '#':
                //note that the walls are to be represented as immobile snakes
                grid[y][x] = Snake;
                map[y][x] = Snake;
                rectfill(buffer, x*20, y*20, x*20 + 19, y*20 + 19, SNAKECOL);
                break;
            case 'x':
                //food cannot spawn on these tiles, otherwise similar to empty
                grid[y][x] = Infertile;
                map[y][x] = Infertile;
                break;
            default:
                grid[y][x] = Empty;
                map[y][x] = Empty;
            }
        }
    }

    fclose(inputfile);
    return true;
}

//resets several variables to initial values
void reset(int *foodx, int *foody, int *specx, int *specy, int *posx, int *posy, int *velx, int *vely, int *score, int *extend, snake **tail, float *speed, int *numElem) {
    *velx = 0;
    *vely = 0;
    color[0] = rand() % 128;
    color[1] = rand() % 128;
    color[2] = rand() % 128;
    if (rand() & 1) {
        color[0] += 128;
        color[1] += 128;
        color[2] += 128;
    }
    clear_to_color(buffer, BACKCOL);
    for (int i=0; i<gridHeight; i++)
        for (int j=0; j<gridWidth; j++) {
            grid[i][j] = map[i][j];
            if (map[i][j] == Snake) rectfill(buffer, j*20, i*20, j*20 + 19, i*20 + 19, SNAKECOL);
        }
    grid[spawny][spawnx] = Snake;
    *extend = 10;
    *score = 0;
    *posx = spawnx;
    *posy = spawny;
    *specx = -1;
    *specy = -1;
    *speed = 10;
    *numElem=1;
    sappend(tail, *posx, *posy, NULL);
    placefood(foodx, foody, specx, specy);
    circlefill(buffer, *posx*20+9, *posy*20+9, 9, SNAKECOL);
    if (*specx!=-1) {
        grid[*specy][*specx] = map[*specy][*specx];
        *specx=-1;
        *specy=-1;
    }
    circlefill(buffer, *foodx*20+9, *foody*20+9, 9, FOODCOL);
    rectfill(screen, 0, 600, 480, 640, BACKCOL);
    textprintf_ex(screen, font, 0, 620, MSGCOL, -1, "Score: 0");
}


//graphical text input to load a file
void load(char* out) {
    char text[PATH_MAX];
    char tmp[PATH_MAX];
    int pos = 0;
    BITMAP* temp = create_bitmap(370, 200);
    BITMAP* prompt = load_bitmap("prompt.bmp", NULL);
    
    draw_sprite(screen, prompt, 60, 160);
    
    while (!quit) {
        pos=0;
        text[0]='\0';
        while (!quit) {
            clear_to_color(temp, makecol(227, 227, 65));
            textprintf_centre_ex(temp, font,  370/2, 55, makecol(0, 130, 65), -1, "%s", text);
            blit(temp, screen, 0, 0, 60, 250, 370, 200);
            
            while (!keypressed()) rest(1);
                        int  key = readkey();
            char ASCII = key & 0xff;
            char scancode = key >> 8;

            // character key pressed
            if (ASCII >= 32 && ASCII <= 126) {
                if (pos < 29) {
                    text[pos] = ASCII;
                    pos++;
                    text[pos] = '\0';
                }
            } else if (scancode == KEY_BACKSPACE) {
                if (pos > 0) pos--;
                text[pos] = '\0';
            } else if (scancode == KEY_ENTER) {
                break;
            } else if (scancode == KEY_ESC) {
                strcpy(text, "quit");
                break;
            }
        }
        
        if (quit) break;
        if (strcmp(text, "quit")==0) {
            text[0] = '\0';
            break;
        }
        
        sprintf(tmp, "maps/%s", text);

        if (!loadMap(tmp)) allegro_message("Could not find map.\nMake sure your map is in Serpens/map/\nTry again, or type \"quit\"");
        else break;
        
    }
    //clean up
    destroy_bitmap(temp);
    destroy_bitmap(prompt);
    strcpy(out, text);
}

//the actual game
void game(char *lastFile) {
    //declare variables to be used
    int posx, posy, velx, vely, foodx, foody, specx, specy, extend, numElem, score, bonusCounter=300;
    float speed = 10;
    bool changed, fancy=false;
    snake* tail = NULL;
    SAMPLE *eat = load_sample("bite.wav");

    //initialize values
    reset(&foodx, &foody, &specx, &specy, &posx, &posy, &velx, &vely, &score, &extend, &tail, &speed, &numElem);

    //While the game isn't quitted
    while (!key[KEY_ESC]&&!quit) {
        
        //decrease bonusCounter that is used for special foods
        if (bonusCounter>50) bonusCounter--;
        
        //whether or not the direction changed
        changed=false;
        while (keypressed()) {
            int key = readkey();
            if ((key & 0xFF) == ' ') {
                // paused
                textprintf_centre_ex(screen, font, scrx / 2, 200, MSGCOL, -1, "GAME PAUSED");
                while (true) {
                    int k = readkey();
                    if ((k & 0xFF) == ' ' || ((k >> 8) & 0xFF) == KEY_ESC) {
                        break;
                    }
                }
                continue;
            }

            if (changed) break;
            switch ((key >> 8) & 0xFF) {
            case KEY_DOWN:
                if (vely == 0) {
                    vely = 1;
                    velx = 0;
                    changed=true;
                }
                break;
            case KEY_UP:
                if (vely == 0) {
                    vely = -1;
                    velx = 0;
                    changed=true;
                }
                break;
            case KEY_RIGHT:
                if (velx == 0) {
                    velx = 1;
                    vely = 0;
                    changed=true;
                }
                break;
            case KEY_LEFT:
                if (velx == 0) {
                    velx = -1;
                    vely = 0;
                    changed=true;
                }
                break;
            case KEY_M:
                fancy=!fancy;
                changed=true;
                break;
            }
        }
        
        //handles movement
        int prevx = posx, prevy = posy;
        if (velx != 0 || vely != 0) {
            snake *leftover = NULL;
            
            //if (snake isn't supposed to be growing in length this frame)
            if (extend == 0) {
                leftover = tail;
                tail = leftover->next;
                //draw over part to be deleted
                rectfill(buffer, tail->x*20, tail->y*20, tail->x*20 + 19, tail->y*20 + 19, BACKCOL);
                rectfill(buffer, leftover->x*20, leftover->y*20, leftover->x*20 + 19, leftover->y*20 + 19, BACKCOL);
                //some variable declarations to make the next part easier
                int dx = tail->next->x - tail->x;
                int dy = tail->next->y - tail->y;
                if (dx > 1) dx = -1;
                else if (dx < -1) dx = 1;
                if (dy > 1) dy = -1;
                else if (dy < -1) dy = 1;
                int hx = tail->x*20;
                int hy = tail->y*20;

                //fancy triangle tail
                triangle(buffer, dx+dy==-1?hx:hx+19, dy+1==dx?hy:hy+19, dy+1==dx?hx+19:hx, dy+dx==-1?hy:hy+19, hx+9 - 9*dx, hy+9 - 9*dy, SNAKECOL);
                
                //remove from grid
                grid[leftover->y][leftover->x] = map[leftover->y][leftover->x];
            } else {
                //decrease the length the snake should extend as the snake has just lengthened
                --extend;
                numElem++;
            }
            
            //move snake
            posx += velx;
            posy += vely;
            
            //wrap around the screen, if necessary
            if (posx < 0) posx += gridWidth;
            if (posx >= gridWidth) posx -= gridWidth;
            if (posy < 0) posy += gridHeight;
            if (posy >= gridHeight) posy -= gridHeight;
            
            //if snake gets food
            if (grid[posy][posx] == Food) {
                play_sample(eat, 255, 128, 1000, 0);
                //replace food, redraw
                placefood(&foodx, &foody, &specx, &specy);
                circlefill(buffer, foodx*20+9, foody*20+9, 9, FOODCOL);
                if (specx!=-1) {
                    circlefill(buffer, specx*20+9, specy*20+9, 9, FOODCOL);
                    circlefill(buffer, specx*20+9, specy*20+9, 6, SNAKECOL);
                }
                score += 10;
                extend++;
                if (speed>10) speed--;
                rectfill(screen, 0, 600, 480, 640, BACKCOL);
                textprintf_ex(screen, font, 0, 620, MSGCOL, -1, "Score: %d", score);

            } else if (grid[posy][posx] == Special) {
                play_sample(eat, 255, 128, 1000, 0);
                specx=-1;
                specy=-1;
                score+=bonusCounter/10;
                bonusCounter=300;
                extend++;
                if (speed>10) speed--;
                rectfill(screen, 0, 600, 480, 640, BACKCOL);
                textprintf_ex(screen, font, 0, 620, MSGCOL, -1, "Score: %d", score);
            } else if (grid[posy][posx] == Snake) {
                //lose the game

                lose(score, color);
                //destroy snake
                while (tail != NULL) {
                    snake *tmp = tail;
                    tail = tail->next;
                    free(tmp);
                }
                //start again
                reset(&foodx, &foody, &specx, &specy, &posx, &posy, &velx, &vely, &score, &extend, &tail, &speed, &numElem);
                continue;
            }
            //add to the beginning of the snake
            sappend(&tail, posx, posy, leftover);
            grid[posy][posx] = Snake;
        }

        //draw the snake's head
        circlefill(buffer, posx*20+9, posy*20+9, 9, SNAKECOL);

        //ensure the body looks good
        if (velx) {
            int x = posx * 20 + ((velx == -1) ? 10 : 0);
            rectfill(buffer, x, posy * 20, x + 9, posy * 20 + 19, SNAKECOL);
            x = prevx * 20 + ((velx == 1) ? 10 : 0);
            rectfill(buffer, x, posy * 20, x + 9, posy * 20 + 19, SNAKECOL);
        }
        if (vely) {
            int y = posy * 20 + ((vely == -1) ? 10 : 0);
            rectfill(buffer, posx * 20, y-2, posx * 20 + 19, y + 9, SNAKECOL);
            y = prevy * 20 + ((vely == 1) ? 10 : 0);
            rectfill(buffer, posx * 20, y-2, posx * 20 + 19, y + 9, SNAKECOL);
        }

        //if fancy centred mode is on, then show it as such
        if (fancy) {
            blit(buffer, screen, 0, 0, scrx/2 - posx*20, (scry-40)/2 - posy*20, buffer->w, buffer->h-((scry-40)/2 - posy*20));
            blit(buffer, screen, 0, 0, scrx/2 - posx*20 - (scrx/2>posx*20?scrx:-scrx), (scry-40)/2 - posy*20, buffer->w, buffer->h-((scry-40)/2 - posy*20));
            blit(buffer, screen, 0, 0, scrx/2 - posx*20, (scry-40)/2 - posy*20 - ((scry-40)/2>posy*20?scry-40:-scry+40), buffer->w, buffer->h-((scry-40)/2 - posy*20 - ((scry-40)/2>posy*20?scry-40:-scry+40)));
            blit(buffer, screen, 0, 0, scrx/2 - posx*20 - (scrx/2>posx*20?scrx:-scrx), (scry-40)/2 - posy*20 - ((scry-40)/2>posy*20?scry-40:-scry+40), buffer->w, buffer->h-((scry-40)/2 - posy*20 - ((scry-40)/2>posy*20?scry-40:-scry+40)));
        }
        else blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);

        rest(int(1000/speed));
    }
    //clean up
    while (tail != NULL) {
        snake *tmp = tail;
        tail = tail->next;
        free(tmp);
    }
    destroy_sample(eat);
}


void menu() {
    //declare and initialize
    BITMAP *menu = load_bitmap("controls.bmp", NULL);
    BITMAP *play = load_bitmap("play.bmp", NULL);
    BITMAP *map = load_bitmap("map.bmp", NULL);
    bool clicked=false;
    int selection = 0;
    char lastFile[30]="default.txt";
    char temp[30];
    SAMPLE *button = load_sample("button.wav");
    
    //show mouse
    if (show_os_cursor(MOUSE_CURSOR_ARROW) != 0) {
        enable_hardware_cursor();
        select_mouse_cursor(MOUSE_CURSOR_ARROW);
        show_mouse(screen);
    }
    //draw menu screen
    blit(menu, screen, 0, 0, 0, 0, 480, 640 );
    
    while (!quit) {
		rest(20);
        while (keypressed()) {
            int key = readkey();
            switch ((key >> 8) & 0xFF) {
            case KEY_UP:
            case KEY_LEFT:
                selection = 2;
                break;
            case KEY_DOWN:
            case KEY_RIGHT:
                selection=1;
                break;
            case KEY_ESC:
                quit=1;
                break;
            }
        }
        
        if ((mouse_x>58 && mouse_x<222 && mouse_y>472 && mouse_y<553) || selection==1) {
            //play sound and draw
            play_sample(button, 255, 128, 1000, 0);
            blit(play, screen, 0, 0, 0, 0, 480, 640);
            while ((mouse_x>58 && mouse_x<222 && mouse_y>472 && mouse_y<553) || selection==1 && !quit) {
                while (keypressed()) {
                    int key = readkey();
                    switch ((key >> 8) & 0xFF) {
                    case KEY_UP:
                    case KEY_LEFT:
                        selection = 0;
                        break;
                    case KEY_DOWN:
                    case KEY_RIGHT:
                        selection = 2;
                        break;
                    case KEY_ENTER:
                        //enter to select
                        game(lastFile);
                        selection = 0;
                        blit(menu, screen, 0, 0, 0, 0, 480, 640 );
                        break;
                    }
                }
                //click to select
                if (mouse_b & 1 && !clicked) {
                    game(lastFile);
                    selection = 0;
                    blit(menu, screen, 0, 0, 0, 0, 480, 640 );
                }
            }
            if (!selection) blit(menu, screen, 0, 0, 0, 0, 480, 640);
        }

        if ((mouse_x>262 && mouse_x<426 && mouse_y>472 && mouse_y<553) || selection==2) {
            //play sound and draw
            play_sample(button, 255, 128, 1000, 0);
            blit(map, screen, 0, 0, 0, 0, 480, 640);
            while ((mouse_x>262 && mouse_x<426 && mouse_y>472 && mouse_y<553) || selection==2 && !quit) {
                while (keypressed()) {
                    int key = readkey();
                    switch ((key >> 8) & 0xFF) {
                    case KEY_UP:
                    case KEY_LEFT:
                        selection=1;
                        break;
                    case KEY_DOWN:
                    case KEY_RIGHT:
                        selection=0;
                        break;
                    case KEY_ENTER:
                        //enter to select
                        load(temp);
                        if (strcmp(temp, "")!=0) strcpy(lastFile, temp);
                        selection = 0;
                        blit(menu, screen, 0, 0, 0, 0, 480, 640 );
                        break;
                    }
                }
                //click to select
                if (mouse_b & 1 && !clicked) {
                    load(temp);
                    if (strcmp(temp, "")!=0) strcpy(lastFile, temp);
                    selection = 0;
                    blit(menu, screen, 0, 0, 0, 0, 480, 640);
                }
            }
            if (!selection) blit(menu, screen, 0, 0, 0, 0, 480, 640 );
        }
        rest(50);
    }
    //clean up
    destroy_bitmap(menu);
    destroy_bitmap(play);
    destroy_bitmap(map);
}







