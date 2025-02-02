#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include <math.h>
#include <regex.h> 
#include <ctype.h>

#define MAP_WIDTH    80
#define MAP_HEIGHT   24
#define MAX_ROOMS    10
#define MAX_ENEMIES  10
#define MAX_ITEMS    20
#define MAX_USERNAME 20
#define MAX_PASSWORD 20
#define MAX_EMAIL 20

typedef enum {
    ITEM_NONE,
    ITEM_GOLD,
    ITEM_FOOD,
    ITEM_POTION,
    ITEM_WEAPON,
    ITEM_ARROW,
    ITEM_TREASURE  
} ItemType;

typedef struct {
    ItemType type;
    int value; 
    char name[20]; 
} Item;

typedef struct {
    int x, y;
    Item item;
    int active;
} MapItem;

typedef enum {
    WEAPON_MACE,
    WEAPON_DAGGER,
    WEAPON_MAGICWAND,
    WEAPON_SWORD
} WeaponType;

typedef struct {
    WeaponType type;
    char name[20];
    int damage;
    int range; 
    int throwable;
} Weapon;

typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int highScore;
} User;

typedef struct {
    int x, y; 
    int health;
    int gold;
    int food;
    Weapon currentWeapon;
    int arrows;
} Player;

typedef struct {
    int x, y;
    int health;
    int damage;
    char type[20];
} Enemy;

/* Global Variables */
char map[MAP_HEIGHT][MAP_WIDTH];
int fog[MAP_HEIGHT][MAP_WIDTH];
typedef struct {
    int x, y, w, h;
} Room;
Room rooms[MAX_ROOMS];
int roomCount = 0;
int isValidEmail(const char *email);
int isValidPassword(const char *password);
MapItem mapItems[MAX_ITEMS];
int itemCount = 0;
Enemy enemies[MAX_ENEMIES];
int enemyCount = 0;
Player player;
int last_dx = 0, last_dy = 0;

/* User Functions */
void registerUser() {
    FILE *fp = fopen("users.txt", "a");
    if (!fp) {
        mvprintw(5, 5, "Error opening users file!");
        getch();
        return;
    }
    User newUser;
    echo();
    mvprintw(5, 5, "Username: ");
    getnstr(newUser.username, MAX_USERNAME-1);
    char password[MAX_PASSWORD];
    int validPassword = 0;
    while (!validPassword) {
    mvprintw(6, 5, "Password (min 7 characters, 1 digit, 1 uppercase, 1 lowercase): ");
    getnstr(password, MAX_PASSWORD-1);
    if (isValidPassword(password)) {
            validPassword = 1;
            strcpy(newUser.password, password);
        }
        else {
            mvprintw(7, 5, "Password must be at least 7 characters long and contain 1 digit, 1 uppercase, and 1 lowercase letter.");
            clrtoeol();
            refresh();
            getch();
            return;
        }
    }
    char email[MAX_EMAIL];
    int validEmail = 0;
    while (!validEmail) {
        mvprintw(7, 5, "Email: ");
        getnstr(email, MAX_EMAIL - 1);
        if (isValidEmail(email)) {
            validEmail = 1; 
        }
        else {
            mvprintw(8, 5, "Invalid email format! Try again.");
            clrtoeol();
            refresh();
            getch();
            return;
        }
    }
    noecho();
    newUser.highScore = 0;
    fprintf(fp, "%s %s %d\n", newUser.username, newUser.password, newUser.highScore);
    fclose(fp);
    mvprintw(8, 5, "Registration successful! Press any key to continue...");
    getch();
}

//login
int loginUser(User *loggedUser) {
    FILE *fp = fopen("users.txt", "r");
    if (!fp) {
        mvprintw(5, 5, "Error opening users file!");
        getch();
        return 0;
    }

    char username[MAX_USERNAME], password[MAX_PASSWORD];
    echo();
    mvprintw(5, 5, "Username: ");
    getnstr(username, MAX_USERNAME-1);
    mvprintw(6, 5, "Password: ");
    getnstr(password, MAX_PASSWORD-1);
    noecho();

    User temp;
    int found = 0;
    while (fscanf(fp, "%s %s %d", temp.username, temp.password, &temp.highScore) == 3) {
        if (strcmp(username, temp.username) == 0 && strcmp(password, temp.password) == 0) {
            *loggedUser = temp;
            found = 1;
            break;
        }
    }
    fclose(fp);
    if (found) {
        mvprintw(8, 5, "Login successful! Press any key to continue...");
        getch();
        return 1;
    }
    else {
        mvprintw(8, 5, "Incorrect username or password! Press any key to try again...");
        getch();
        return 0;
    }
}

//room
void createRoom(int x, int y, int w, int h) {
    int i, j;
    for(i = y; i < y + h && i < MAP_HEIGHT - 1; i++){
        for(j = x; j < x + w && j < MAP_WIDTH - 1; j++){
            map[i][j] = '.';
        }
    }
    if(roomCount < MAX_ROOMS) {
        rooms[roomCount].x = x;
        rooms[roomCount].y = y;
        rooms[roomCount].w = w;
        rooms[roomCount].h = h;
        roomCount++;
    }
}

void connectRooms(Room a, Room b) {
    int x1 = a.x + a.w/2;
    int y1 = a.y + a.h/2;
    int x2 = b.x + b.w/2;
    int y2 = b.y + b.h/2;
    int i;
    if(x1 < x2) {
        for(i = x1; i <= x2; i++){
            if(i >= 0 && i < MAP_WIDTH && y1 >= 0 && y1 < MAP_HEIGHT)
                map[y1][i] = '.';
        }
    }
    else {
        for(i = x2; i <= x1; i++){
            if(i >= 0 && i < MAP_WIDTH && y1 >= 0 && y1 < MAP_HEIGHT)
                map[y1][i] = '.';
        }
    }
    if(y1 < y2) {
        for(i = y1; i <= y2; i++){
            if(x2 >= 0 && x2 < MAP_WIDTH && i >= 0 && i < MAP_HEIGHT)
                map[i][x2] = '.';
        }
    }
    else {
        for(i = y2; i <= y1; i++){
            if(x2 >= 0 && x2 < MAP_WIDTH && i >= 0 && i < MAP_HEIGHT)
                map[i][x2] = '.';
        }
    }
}

void generateMap() {
    int i, j;
    for(i = 0; i < MAP_HEIGHT; i++){
        for(j = 0; j < MAP_WIDTH; j++){
            map[i][j] = '#';
            fog[i][j] = 0;
        }
    }
    roomCount = 0;
    srand(time(NULL));
    int numRooms = 5 + rand() % (MAX_ROOMS - 4);
    for(i = 0; i < numRooms; i++){
        int w = 6 + rand() % 10;
        int h = 4 + rand() % 6;
        int x = 1 + rand() % (MAP_WIDTH - w - 2);
        int y = 1 + rand() % (MAP_HEIGHT - h - 2);
        createRoom(x, y, w, h);
    }
    for(i = 1; i < roomCount; i++){
        connectRooms(rooms[i-1], rooms[i]);
    }
    for(i = 0; i < roomCount; i++){
        int doorX = rooms[i].x + rooms[i].w/2;
        int doorY = rooms[i].y;
        if(doorY >= 0 && doorY < MAP_HEIGHT && doorX >= 0 && doorX < MAP_WIDTH)
            map[doorY][doorX] = '+';
    }
}

/* Map Item Functions */
void placeMapItem(int x, int y, ItemType type, int value, const char *name) {
    if(itemCount < MAX_ITEMS) {
        mapItems[itemCount].x = x;
        mapItems[itemCount].y = y;
        mapItems[itemCount].item.type = type;
        mapItems[itemCount].item.value = value;
        if(name)
            strncpy(mapItems[itemCount].item.name, name, sizeof(mapItems[itemCount].item.name)-1);

        else
            strcpy(mapItems[itemCount].item.name, "");
        mapItems[itemCount].active = 1;
        itemCount++;
    }
}

void placeItems() {
    int i;
    itemCount = 0;
    for(i = 0; i < roomCount; i++){
        int x = rooms[i].x + 1 + rand() % (rooms[i].w - 2);
        int y = rooms[i].y + 1 + rand() % (rooms[i].h - 2);
        int r = rand() % 100;
        if(r < 40) {
            placeMapItem(x, y, ITEM_GOLD, 10 + rand()%20, "Gold");
        } else if(r < 65) {
            placeMapItem(x, y, ITEM_FOOD, 5 + rand()%10, "Food");
        } else if(r < 80) {
            placeMapItem(x, y, ITEM_POTION, 20, "Potion");
        } else if(r < 90) {
            if(rand()%2)
                placeMapItem(x, y, ITEM_WEAPON, 0, "Dagger");
            else
                placeMapItem(x, y, ITEM_WEAPON, 0, "Sword");
        } else {
            placeMapItem(x, y, ITEM_ARROW, 1, "Arrow");
        }
    }
    int idx = rand() % roomCount;
    int gx = rooms[idx].x + 1 + rand() % (rooms[idx].w - 2);
    int gy = rooms[idx].y + 1 + rand() % (rooms[idx].h - 2);
    placeMapItem(gx, gy, ITEM_TREASURE, 0, "Treasure");
}

/* Fog Functions */
void displayMap() {
    int i, j;
    clear();
    for(i = 0; i < MAP_HEIGHT; i++){
        for(j = 0; j < MAP_WIDTH; j++){
            if (fog[i][j]) {
                mvaddch(i, j, map[i][j]);
            } else {
                mvaddch(i, j, ' ');
            }
        }
    }
    for(i = 0; i < itemCount; i++){
        if(mapItems[i].active && fog[ mapItems[i].y ][ mapItems[i].x ]) {
            char ch;
            switch(mapItems[i].item.type) {
                case ITEM_GOLD:      ch = '$'; break;
                case ITEM_FOOD:      ch = 'F'; break;
                case ITEM_POTION:    ch = 'P'; break;
                case ITEM_WEAPON:    ch = 'W'; break;
                case ITEM_ARROW:     ch = 'A'; break;
                case ITEM_TREASURE:  ch = 'T'; break;
                default:             ch = '?'; break;
            }
            mvaddch(mapItems[i].y, mapItems[i].x, ch);
        }
    }
    for(i = 0; i < enemyCount; i++){
        if(fog[ enemies[i].y ][ enemies[i].x ])
            mvaddch(enemies[i].y, enemies[i].x, 'X');
    }
    mvaddch(player.y, player.x, '@');
    mvprintw(MAP_HEIGHT, 0, "Health: %d  Gold: %d  Food: %d  Weapon: %s  Arrows: %d",
             player.health, player.gold, player.food, player.currentWeapon.name, player.arrows);
    mvprintw(MAP_HEIGHT+1, 0, "Controls: Move (W,A,S,D,X,E,Z,C) | t: Throw weapon (if Dagger) | p: Save | q: Quit | SPACE: Attack");
    refresh();
}

void updateFog() {
    int i, j;
    int radius = 5;
    for(i = -radius; i <= radius; i++){
        for(j = -radius; j <= radius; j++){
            int ny = player.y + i;
            int nx = player.x + j;
            if(ny >= 0 && ny < MAP_HEIGHT && nx >= 0 && nx < MAP_WIDTH)
                fog[ny][nx] = 1;
        }
    }
}

int canMoveTo(int x, int y) {
    if(x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return 0;
    if(map[y][x] == '#')//wall
        return 0;
    return 1;
}

void processInput(int ch) {
    int newX = player.x;
    int newY = player.y;
    last_dx = 0; last_dy = 0;

    switch(ch) {
        case KEY_UP:
        case 'w': case 'W':
            newY--;
            last_dy = -1;
            break;
        case KEY_DOWN:
        case 's': case 'S':
            newY++;
            last_dy = 1;
            break;
        case KEY_LEFT:
        case 'a': case 'A':
            newX--;
            last_dx = -1;
            break;
        case KEY_RIGHT:
        case 'd': case 'D':
            newX++;
            last_dx = 1;
            break;
        case 'x': case 'X':  // up-left
            newX--;
            newY--;
            last_dx = -1; last_dy = -1;
            break;
        case 'e': case 'E':  // up-right
            newX++;
            newY--;
            last_dx = 1; last_dy = -1;
            break;
        case 'z': case 'Z':  // down-left
            newX--;
            newY++;
            last_dx = -1; last_dy = 1;
            break;
        case 'c': case 'C':  // down-right
            newX++;
            newY++;
            last_dx = 1; last_dy = 1;
            break;
        default:
            break;
    }
    if(canMoveTo(newX, newY)) {
        player.x = newX;
        player.y = newY;
    }
}

void pickupItem() {
    int i;
    for(i = 0; i < itemCount; i++){
        if(mapItems[i].active && mapItems[i].x == player.x && mapItems[i].y == player.y) {
            switch(mapItems[i].item.type) {
                case ITEM_GOLD:
                    player.gold += mapItems[i].item.value;
                    mvprintw(MAP_HEIGHT+2, 0, "Picked up Gold! (+%d)", mapItems[i].item.value);
                    break;
                case ITEM_FOOD:
                    player.food += mapItems[i].item.value;
                    mvprintw(MAP_HEIGHT+2, 0, "Picked up Food! (+%d)", mapItems[i].item.value);
                    break;
                case ITEM_POTION:
                    player.health += mapItems[i].item.value;
                    if(player.health > 100) player.health = 100;
                    mvprintw(MAP_HEIGHT+2, 0, "Drank Potion! Health +%d", mapItems[i].item.value);
                    break;
                case ITEM_WEAPON:
                    if(strcmp(mapItems[i].item.name, "Dagger") == 0) {
                        player.currentWeapon.type = WEAPON_DAGGER;
                        strcpy(player.currentWeapon.name, "Dagger");
                        player.currentWeapon.damage = 15;
                        player.currentWeapon.range = 3;  // throwable
                        player.currentWeapon.throwable = 1;
                    } else if(strcmp(mapItems[i].item.name, "Sword") == 0) {
                        player.currentWeapon.type = WEAPON_SWORD;
                        strcpy(player.currentWeapon.name, "Sword");
                        player.currentWeapon.damage = 20;
                        player.currentWeapon.range = 1;
                        player.currentWeapon.throwable = 0;
                    }
                    mvprintw(MAP_HEIGHT+2, 0, "Picked up new weapon: %s", player.currentWeapon.name);
                    break;
                case ITEM_ARROW:
                    player.arrows += 1;
                    mvprintw(MAP_HEIGHT+2, 0, "Picked up an Arrow!");
                    break;
                case ITEM_TREASURE:
                    mvprintw(MAP_HEIGHT+2, 0, "Final Treasure found! You win!");
                    refresh();
                    getch();
                    endwin();
                    exit(0);
                    break;
                default:
                    break;
            }
            mapItems[i].active = 0;
            getch();
        }
    }
}

/* Enemy and Combat Functions */
const char *enemyTypesList[] = {"Demon", "Fire Monster", "Giant", "Snake", "Undead"};

void initEnemies() {
    enemyCount = 0;
    int i;
    for(i = 0; i < MAX_ENEMIES; i++){
        int ex = rand() % MAP_WIDTH;
        int ey = rand() % MAP_HEIGHT;
        if(map[ey][ex] == '.') {
            enemies[enemyCount].x = ex;
            enemies[enemyCount].y = ey;
            enemies[enemyCount].health = 30 + rand()%20;
            enemies[enemyCount].damage = 5 + rand()%5;
            strncpy(enemies[enemyCount].type, enemyTypesList[rand() % 5], sizeof(enemies[enemyCount].type)-1);
            enemyCount++;
        }
    }
}

void enemyTurn() {
    int i;
    for(i = 0; i < enemyCount; i++){
        int dir = rand() % 8;
        int dx = 0, dy = 0;
        switch(dir) {
            case 0: dy = -1; break;
            case 1: dy = 1; break;
            case 2: dx = -1; break;
            case 3: dx = 1; break;
            case 4: dx = -1; dy = -1; break;
            case 5: dx = 1; dy = -1; break;
            case 6: dx = -1; dy = 1; break;
            case 7: dx = 1; dy = 1; break;
        }
        int newX = enemies[i].x + dx;
        int newY = enemies[i].y + dy;
        if(newX >= 0 && newX < MAP_WIDTH && newY >= 0 && newY < MAP_HEIGHT && map[newY][newX] == '.') {
            enemies[i].x = newX;
            enemies[i].y = newY;
        }
        if(abs(enemies[i].x - player.x) <= 1 && abs(enemies[i].y - player.y) <= 1) {
            player.health -= enemies[i].damage;
            mvprintw(MAP_HEIGHT+2, 0, "Enemy (%s) attacked you! (-%d health)", enemies[i].type, enemies[i].damage);
            getch();
        }
    }
}

void playerAttack() {
    int i;
    int hit = 0;
    for(i = 0; i < enemyCount; i++){
        if(abs(enemies[i].x - player.x) <= 1 && abs(enemies[i].y - player.y) <= 1) {
            enemies[i].health -= player.currentWeapon.damage;
            mvprintw(MAP_HEIGHT+2, 0, "Attacked enemy (%s)! (-%d health)", enemies[i].type, player.currentWeapon.damage);
            hit = 1;
            if(enemies[i].health <= 0) {
                mvprintw(MAP_HEIGHT+2, 0, "Enemy (%s) defeated!", enemies[i].type);
                player.gold += 10;
                enemies[i] = enemies[enemyCount-1];
                enemyCount--;
            }
            break;
        }
    }
    if(!hit) {
        mvprintw(MAP_HEIGHT+2, 0, "No enemy nearby to attack!");
    }
    getch();
}

void throwWeapon() {
    if(player.currentWeapon.throwable && strcmp(player.currentWeapon.name, "Dagger") == 0) {
        int projX = player.x;
        int projY = player.y;
        int steps = player.currentWeapon.range;  // throwing range
        int hit = 0;
        while(steps-- > 0) {
            projX += last_dx;
            projY += last_dy;
            if(projX < 0 || projX >= MAP_WIDTH || projY < 0 || projY >= MAP_HEIGHT || map[projY][projX]=='#')
                break;
            int i;
            for(i = 0; i < enemyCount; i++){
                if(enemies[i].x == projX && enemies[i].y == projY) {
                    enemies[i].health -= player.currentWeapon.damage;
                    mvprintw(MAP_HEIGHT+2, 0, "Enemy (%s) hit by thrown Dagger! (-%d health)", enemies[i].type, player.currentWeapon.damage);
                    hit = 1;
                    if(enemies[i].health <= 0) {
                        mvprintw(MAP_HEIGHT+2, 0, "Enemy (%s) defeated!", enemies[i].type);
                        player.gold += 10;
                        enemies[i] = enemies[enemyCount-1];
                        enemyCount--;
                    }
                    break;
                }
            }
            if(hit)
                break;
            mvaddch(projY, projX, '*');
            refresh();
            napms(100);
        }
        if(!hit)
            mvprintw(MAP_HEIGHT+2, 0, "Thrown Dagger did not hit any enemy!");

        player.currentWeapon.type = WEAPON_MACE;
        strcpy(player.currentWeapon.name, "Mace");
        player.currentWeapon.damage = 10;
        player.currentWeapon.range = 1;
        player.currentWeapon.throwable = 0;
        getch();
    }
    else {
        mvprintw(MAP_HEIGHT+2, 0, "Your weapon is not throwable!");
        getch();
    }
}

/* Save Functions */
void saveGame() {
    FILE *fp = fopen("savegame.dat", "wb");
    if (!fp) {
        mvprintw(0, 0, "Error saving game!");
        return;
    }
    fwrite(&player, sizeof(Player), 1, fp);
    fwrite(&enemyCount, sizeof(int), 1, fp);
    fwrite(enemies, sizeof(Enemy), enemyCount, fp);
    fwrite(&itemCount, sizeof(int), 1, fp);
    fwrite(mapItems, sizeof(MapItem), itemCount, fp);
    fwrite(&roomCount, sizeof(int), 1, fp);
    fwrite(rooms, sizeof(Room), roomCount, fp);
    fwrite(map, sizeof(char), MAP_HEIGHT * MAP_WIDTH, fp);
    fclose(fp);
    mvprintw(0, 0, "Game saved!");
    refresh();
    napms(1000);
}
// Load game 
void loadGame() {
    FILE *fp = fopen("savegame.dat", "rb");
    if (!fp) {
        mvprintw(0, 0, "Error loading game!");
        return;
    }
    fread(&player, sizeof(Player), 1, fp);
    fread(&enemyCount, sizeof(int), 1, fp);
    fread(enemies, sizeof(Enemy), enemyCount, fp);
    fread(&itemCount, sizeof(int), 1, fp);
    fread(mapItems, sizeof(MapItem), itemCount, fp);
    fread(&roomCount, sizeof(int), 1, fp);
    fread(rooms, sizeof(Room), roomCount, fp);
    fread(map, sizeof(char), MAP_HEIGHT * MAP_WIDTH, fp);
    fclose(fp);
    memset(fog, 0, sizeof(fog));
    mvprintw(0, 0, "Game loaded!");
    refresh();
    napms(1000);
}

void mainMenu() {
    int ch;
    while(1) {
        clear();
        mvprintw(3, 10, "=== Rogue ===");
        mvprintw(5, 10, "1. Register");
        mvprintw(6, 10, "2. Login");
        mvprintw(7, 10, "3. Load Game");
        mvprintw(8, 10, "4. Quit");
        mvprintw(10, 10, "Enter your choice: ");
        refresh();

        ch = getch();
        if(ch == '1') {
            clear();
            registerUser();
        } else if(ch == '2') {
            User currentUser;
            clear();
            if(loginUser(&currentUser))
                break;
        } else if(ch == '3') {
            loadGame();
            break;
        } else if(ch == '4') {
            endwin();
            exit(0);
        }
    }
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    mainMenu();
    player.x = MAP_WIDTH/2;
    player.y = MAP_HEIGHT/2;
    player.health = 100;
    player.gold = 0;
    player.food = 100;
    player.arrows = 0;
    player.currentWeapon.type = WEAPON_MACE;
    strcpy(player.currentWeapon.name, "Mace");
    player.currentWeapon.damage = 10;
    player.currentWeapon.range = 1;
    player.currentWeapon.throwable = 0;
    generateMap();
    placeItems();
    updateFog();
    initEnemies();

    int ch;
    while(1) {
        displayMap();
        ch = getch();
        if(ch == 't' || ch == 'T') {
            throwWeapon();
        }
        else if(ch == 'p' || ch == 'P') {
            saveGame();
        }
        else if(ch == 'q' || ch == 'Q') {
            break;
        }
        else if(ch == ' ') {
            playerAttack();
        }
        else {
            processInput(ch);
        }
        pickupItem();
        updateFog();
        enemyTurn();
        if(player.health <= 0) {
            clear();
            mvprintw(MAP_HEIGHT/2, MAP_WIDTH/2 - 10, "You have been defeated!");
            refresh();
            getch();
            break;
        }
    }
    endwin();
    return 0;
}

int isValidEmail(const char *email) {
    regex_t regex;
    int result;
    const char *pattern = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";
    result = regcomp(&regex, pattern, REG_EXTENDED);
    if (result) {
        return 0; 
    }
    result = regexec(&regex, email, 0, NULL, 0);
    regfree(&regex);
    return (result == 0);
}

int isValidPassword(const char *password) {
    int length = strlen(password);
    if (length < 7) {
        return 0;  
    }
    int hasDigit = 0, hasUpper = 0, hasLower = 0;
    for (int i = 0; i < length; i++) {
        if (isdigit(password[i])) {
            hasDigit = 1;
        }
        else if (isupper(password[i])) {
            hasUpper = 1;
        }
        else if (islower(password[i])) {
            hasLower = 1;
        }
    }
    return hasDigit && hasUpper && hasLower;
}
//