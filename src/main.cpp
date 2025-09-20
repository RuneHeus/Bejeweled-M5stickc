#include <M5StickCPlus.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "EEPROM.h"
#include <time.h>

#define MIN_TILT 0.15
#define speed 3
#define MEM_SIZE 512

float acc_x = 0, acc_y = 0, acc_z = 0;

uint8_t gameVariant = 0; // 1 = Variant 1, 2 = Variant 2

unsigned long buttonHoldStartTime = 0;
const unsigned long maxButtonHoldTime = 3000;
static bool buttonHeld3Seconds = false;

uint8_t selected = 0;

uint8_t gameLoop = 0;

struct Diamond { //This represents one diamond
    int x;
    int y;
    uint16_t color;
    char special;
};

struct Player {
    uint8_t score;
    uint8_t moves;
    Diamond* selectedDiamond;
    uint8_t  selectedDiamondX;
    uint8_t  selectedDiamondY;
} player;

const uint16_t colorOptions[] = {
    RED,
    YELLOW,
    GREEN,
    BLUE,
    ORANGE,
    PURPLE,
    PINK
};

uint16_t getRandomColor() {
    unsigned long seed = micros();
    randomSeed(seed);  // Initialize the random number generator with the seed
    return colorOptions[random(7)];
}

int color_size = 7; //This is the size of the array of colors

Diamond diamonds[8][8];

void fallDiamonds() {
    for (int i = 0; i < 8; ++i) {
        // Start from the bottom and move diamonds down
        for (int j = 6; j >= 0; --j) {
            if (diamonds[i][j].color != BLACK) {
                int k = j;
                // Move the diamond down until an empty space is found or the bottom is reached
                while (k < 7 && diamonds[i][k + 1].color == BLACK) {
                    diamonds[i][k + 1].color = diamonds[i][k].color;
                    diamonds[i][k + 1].special = diamonds[i][k].special;
                    diamonds[i][k].color = BLACK;
                    diamonds[i][k].special = 'N';
                    ++k;
                }
            }
        }
    }
    // Refill the empty spaces
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (diamonds[i][j].color == BLACK) {
                diamonds[i][j].color = getRandomColor();
            }
        }
    }
}

bool checkForMatches() {
    // Check for horizontal matches
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 6; ++j) {
            if (diamonds[i][j].color == diamonds[i][j + 1].color && diamonds[i][j].color == diamonds[i][j + 2].color) {
                return true;
            }
        }
    } 
    // Check for vertical matches
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (diamonds[i][j].color == diamonds[i + 1][j].color && diamonds[i][j].color == diamonds[i + 2][j].color) {
                return true;
            }
        }
    }
    return false;
}

void addDiamond(int x, int y) {
    if (x >= 0 && x < 8 && y >= 0 && y < 8) {
        diamonds[x][y].x = x;
        diamonds[x][y].y = y;
        uint16_t newColor = getRandomColor();
        
        // Controleer op horizontale match en sla de kleur op
        while ((x >= 2 && diamonds[x - 1][y].color == newColor && diamonds[x - 2][y].color == newColor) ||
               (x <= 5 && diamonds[x + 1][y].color == newColor && diamonds[x + 2][y].color == newColor)) {
            newColor = getRandomColor();
        }
        uint16_t verticalColor = newColor;

        // Controleer op verticale match met de opgeslagen kleur
        while ((y >= 2 && diamonds[x][y - 1].color == verticalColor && diamonds[x][y - 2].color == verticalColor) ||
               (y <= 5 && diamonds[x][y + 1].color == verticalColor && diamonds[x][y + 2].color == verticalColor)) {
            verticalColor = getRandomColor();
        }
        diamonds[x][y].color = verticalColor;
        diamonds[x][y].special = 'N';
    }
}


void generateLevel(){
    // Adding the 8x8 array of diamonds
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            addDiamond(i, j);
        }
    }
}

void drawLoseScreen(){
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(5, M5.Lcd.height() - 160);
    M5.Lcd.printf("Game Over!");
}

void updateGame(){ //Thiz is responsible for looking if the player has any moves left or if the player reached the score goal of the level
    if(player.score >= 100){
        generateLevel();
        player.moves = 15;
        player.score = 0;
    }else if(player.moves == 0){
        gameLoop = 0;
        drawLoseScreen();
    }
}

void drawDiamonds() {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            int centerX = (15 * (diamonds[i][j].x + 1)) + 7;
            int centerY = (15 * (diamonds[i][j].y + 1)) + 7;

            if (diamonds[i][j].special == 'N') {
                M5.Lcd.fillCircle(
                    (15 *((diamonds[i][j].x) + 1)) + 7, (15 *((diamonds[i][j].y)+ 1)) + 7,
                    7, diamonds[i][j].color
                ); 
            } else if (diamonds[i][j].special == 'V') {
                M5.Lcd.fillTriangle(centerX - 7, centerY, centerX, centerY - 7, centerX + 7, centerY, diamonds[i][j].color);
            } else if (diamonds[i][j].special == 'H') {
                M5.Lcd.fillTriangle(
                    centerX - 7, centerY,
                    centerX + 7, centerY - 7,
                    centerX + 7, centerY + 7,
                    diamonds[i][j].color
                );
            }
        }
    }
}

void displayPlayerMove(int move) {
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(5, M5.Lcd.height() - 30);
    M5.Lcd.printf("Moves: %d", move);
}

void displayPlayerScore(int score) {
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(5, M5.Lcd.height() - 50);
    M5.Lcd.printf("Score: %d", score);
}

void swapDiamonds(struct Diamond* diamond1, struct Diamond* diamond2) {
    int temp_color = diamond1->color;
    int temp_special = diamond1->special;
    diamond1->color = diamond2->color;
    diamond1->special = diamond2->special;
    diamond2->special = temp_special;
    diamond2->color = temp_color;
}

void markMatchingDiamonds(int x, int y, uint16_t color, bool marked[8][8]) {
    if (x < 0 || x >= 8 || y < 0 || y >= 8 || marked[x][y] || diamonds[x][y].color != color) {
        return;
    }

    marked[x][y] = true;

    // Check horizontally
    int countHorizontal = 1;
    int i = x + 1;
    while (i < 8 && diamonds[i][y].color == color) {
        marked[i][y] = true;
        countHorizontal++;
        i++;
    }

    i = x - 1;
    while (i >= 0 && diamonds[i][y].color == color) {
        marked[i][y] = true;
        countHorizontal++;
        i--;
    }

    // Check vertically
    int countVertical = 1;
    int j = y + 1;
    while (j < 8 && diamonds[x][j].color == color) {
        marked[x][j] = true;
        countVertical++;
        j++;
    }

    j = y - 1;
    while (j >= 0 && diamonds[x][j].color == color) {
        marked[x][j] = true;
        countVertical++;
        j--;
    }

    // If there are less than 2 diamonds in a row horizontally or vertically, unmark them
    if (countHorizontal < 2) {
        i = x + 1;
        while (i < 8 && marked[i][y]) {
            marked[i][y] = false;
            i++;
        }

        i = x - 1;
        while (i >= 0 && marked[i][y]) {
            marked[i][y] = false;
            i--;
        }
    }

    if (countVertical < 2) {
        j = y + 1;
        while (j < 8 && marked[x][j]) {
            marked[x][j] = false;
            j++;
        }

        j = y - 1;
        while (j >= 0 && marked[x][j]) {
            marked[x][j] = false;
            j--;
        }
    }

    markMatchingDiamonds(x + 1, y, color, marked);
    markMatchingDiamonds(x - 1, y, color, marked);
    markMatchingDiamonds(x, y + 1, color, marked);
    markMatchingDiamonds(x, y - 1, color, marked);
}

void removeMatchingDiamonds() {
    bool marked[8][8] = {false};

    markMatchingDiamonds(player.selectedDiamondX, player.selectedDiamondY, diamonds[player.selectedDiamondX][player.selectedDiamondY].color, marked);

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (marked[i][j] && !(i == player.selectedDiamondX && j == player.selectedDiamondY)) {
                int verticalCount = 1;
                int horizontalCount = 1;

                for (int k = 1; k < 4; ++k) {
                    if (j + k < 8 && diamonds[i][j + k].color == diamonds[i][j].color) {
                        verticalCount++;
                    } else {
                        break;
                    }
                }

                for (int k = 1; k < 4; ++k) {
                    if (i + k < 8 && diamonds[i + k][j].color == diamonds[i][j].color) {
                        horizontalCount++;
                    } else {
                        break;
                    }
                }

                if (verticalCount >= 4 && diamonds[player.selectedDiamondX][player.selectedDiamondY].special != 'H') {
                    Serial.println("4 of meer verticaal");
                    diamonds[player.selectedDiamondX][player.selectedDiamondY].special = 'V';
                }

                if (horizontalCount >= 4 && diamonds[player.selectedDiamondX][player.selectedDiamondY].special != 'V') {
                    Serial.println("4 of meer horizontaal");
                    diamonds[player.selectedDiamondX][player.selectedDiamondY].special = 'H';
                }
            }
        }
    }

    // Remove the diamonds
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (marked[i][j]) {
                if (diamonds[i][j].special == 'V') {
                    if(!(i == player.selectedDiamondX && j == player.selectedDiamondY)){
                        Serial.println("Verwijder alles verticaal");
                        diamonds[i][j].special = 'N';
                        for (int k = 0; k < 8; ++k) {
                            diamonds[i][k].color = BLACK;
                        }
                    }         
                } else if (diamonds[i][j].special == 'H'){
                    if(!(i == player.selectedDiamondX && j == player.selectedDiamondY)){
                        Serial.println("Verwijder alles horizontaal");
                        diamonds[i][j].special = 'N';
                        for (int k = 0; k < 8; ++k) {
                            diamonds[k][j].color = BLACK;
                        }
                    }
                }else{
                    diamonds[i][j].color = BLACK;
                }
            }
        }
    }

    fallDiamonds();
}

unsigned long buttonAPressStartTime = 0;
const unsigned long longPressDuration = 1000;
bool longPressHandled = false;
bool aButtonPressed = false;

enum MenuOption {
    BACK,
    SAVE,
    LOAD,
    RESET
};

enum VariantOption{
    FIRST,
    SECOND
};

MenuOption currentMenu = BACK;
VariantOption currentVariant = FIRST;
bool menuVisible = false;

void drawSelector(){
    if(selected == 1){
        M5.Lcd.drawRect(
            (15 * ((diamonds[player.selectedDiamondX][player.selectedDiamondY].x) + 1)),
            (15 * ((diamonds[player.selectedDiamondX][player.selectedDiamondY].y) + 1)),
            15, 15, GREEN
        );
    }else{
        M5.Lcd.drawRect(
            (15 * ((diamonds[player.selectedDiamondX][player.selectedDiamondY].x) + 1)),
            (15 * ((diamonds[player.selectedDiamondX][player.selectedDiamondY].y) + 1)),
            15, 15, WHITE
        );
    }
}

void accelerometer() {
    M5.IMU.getAccelData(&acc_x, &acc_y, &acc_z);

    if (menuVisible) {
        if (acc_y >= 0.30) {
            if (currentMenu != RESET) {
                currentMenu = static_cast<MenuOption>(static_cast<int>(currentMenu) + 1);
                delay(200);
            }
        } else if (acc_y <= -0.30) {
            if (currentMenu != BACK) {
                currentMenu = static_cast<MenuOption>(static_cast<int>(currentMenu) - 1);
                delay(200);
            }
        }
    }else if (gameVariant == 0){
        if (acc_y >= 0.30) {
            if (currentVariant != SECOND) {
                currentVariant = static_cast<VariantOption>(static_cast<int>(currentVariant) + 1);
                delay(200);
            }
        } else if (acc_y <= -0.30) {
            if (currentVariant != FIRST) {
                currentVariant = static_cast<VariantOption>(static_cast<int>(currentVariant) - 1);
                delay(200);
            }
        }
    }else{
        if (acc_x >= 0.30) {
            if (player.selectedDiamondX > 0 && gameVariant != 0) {
                if (selected) {
                    // You can only move 1 cell away to the right from the selected one
                    if (player.selectedDiamondX > player.selectedDiamond->x - 1 && player.selectedDiamondY == player.selectedDiamond->y) {
                        player.selectedDiamondX--;
                    }
                } else {
                    player.selectedDiamondX--;
                }
            }
        } else if (acc_x <= -0.30) {
            if (player.selectedDiamondX < 7 && gameVariant != 0) {
                if (selected) {
                    // You can only move 1 cell away to the left from the selected one
                    if (player.selectedDiamondX < player.selectedDiamond->x + 1 && player.selectedDiamondY == player.selectedDiamond->y) {
                        player.selectedDiamondX++;
                    }
                } else {
                    player.selectedDiamondX++;
                }
            }
        }

        if (acc_y >= 0.30) {
            if(gameVariant != 0){
                if (player.selectedDiamondY < 7) {
                    if (selected == 1) {
                        // You can only move 1 cell up from the selected one
                        if (player.selectedDiamondY < player.selectedDiamond->y + 1 && player.selectedDiamondX == player.selectedDiamond->x) {
                            player.selectedDiamondY++;
                        }
                    } else {
                        player.selectedDiamondY++;
                    }
                }
            }
            
        } else if (acc_y <= -0.30) {
            if(gameVariant != 0){
                if (player.selectedDiamondY > 0) {
                    if (selected == 1) {
                        // You can only move 1 cell down from the selected one
                        if (player.selectedDiamondY > player.selectedDiamond->y - 1 && player.selectedDiamondX == player.selectedDiamond->x) {
                            player.selectedDiamondY--;
                        }
                    } else {
                        player.selectedDiamondY--;
                    }
                }
            }
        }
    }
}

void saveColorToEEPROM(uint16_t color, int &addr) {
    EEPROM.writeByte(addr++, (byte)(color >> 8));
    EEPROM.writeByte(addr++, (byte)color);
}

uint16_t loadColorFromEEPROM(int &addr) {
    byte highByte = EEPROM.readByte(addr);
    addr++;
    byte lowByte = EEPROM.readByte(addr);
    addr++;
    return ((uint16_t)highByte << 8) | lowByte;
}

void saveGameState() {
    int addr = 0;

    EEPROM.writeByte(addr, gameVariant);
    addr++;
    EEPROM.writeByte(addr, player.score);
    addr++;
    EEPROM.writeByte(addr, player.moves);
    addr++;
    EEPROM.writeByte(addr, player.selectedDiamondX);
    addr++;
    EEPROM.writeByte(addr, player.selectedDiamondY);
    addr++;

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            saveColorToEEPROM(diamonds[i][j].color, addr);
            EEPROM.writeByte(addr, diamonds[i][j].special);
            addr++;
        }
    }

    EEPROM.commit();
}

void loadGameState() {
    int addr = 0;

    gameVariant = EEPROM.readByte(addr);
    addr++;
    player.score = EEPROM.readByte(addr);
    addr++;
    player.moves = EEPROM.readByte(addr);
    addr++;
    player.selectedDiamondX = EEPROM.readByte(addr);
    addr++;
    player.selectedDiamondY = EEPROM.readByte(addr);
    addr++;

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            diamonds[i][j].color = loadColorFromEEPROM(addr);
            diamonds[i][j].special = EEPROM.readByte(addr);
            addr++;
        }
    }
}

void displayBeginScreen(){
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);

    M5.Lcd.setCursor(5, M5.Lcd.height() - 130);
    M5.Lcd.setTextColor((currentVariant == FIRST) ? GREEN : WHITE);
    M5.Lcd.println("Variant 1");

    M5.Lcd.setCursor(5, M5.Lcd.height() - 70);
    M5.Lcd.setTextColor((currentVariant == SECOND) ? GREEN : WHITE);
    M5.Lcd.println("Variant 2");
}

void displayMenu() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);

    M5.Lcd.setCursor(5, M5.Lcd.height() - 160);
    M5.Lcd.println("Menu:");

    M5.Lcd.setCursor(5, M5.Lcd.height() - 130);
    M5.Lcd.setTextColor((currentMenu == BACK) ? GREEN : WHITE);
    M5.Lcd.println("Back");

    M5.Lcd.setCursor(5, M5.Lcd.height() - 100);
    M5.Lcd.setTextColor((currentMenu == SAVE) ? GREEN : WHITE);
    M5.Lcd.println("Save");

    M5.Lcd.setCursor(5, M5.Lcd.height() - 70);
    M5.Lcd.setTextColor((currentMenu == LOAD) ? GREEN : WHITE);
    M5.Lcd.println("Load");

    M5.Lcd.setCursor(5, M5.Lcd.height() - 40);
    M5.Lcd.setTextColor((currentMenu == RESET) ? GREEN : WHITE);
    M5.Lcd.println("Reset");
}

void reset(){
    player.moves = 15;
    player.score = 0;
    player.selectedDiamondX = 0;
    player.selectedDiamondY = 0;

    generateLevel();
}

void setup() {
    M5.begin();
    M5.IMU.Init();
    Serial.begin(115200);
    EEPROM.begin(512);

    M5.Lcd.fillScreen(BLACK); // Clear the screen for the main game

    player.moves = 15;
    player.score = 0;
    player.selectedDiamondX = 0;
    player.selectedDiamondY = 0;
    gameVariant = 0;

    generateLevel();
}

void handleMenuInput() {
    if (M5.BtnA.wasPressed()) {
        switch (currentMenu) {
            case BACK:
                menuVisible = false;
                break;
            case SAVE:
                menuVisible = false;
                saveGameState();
                break;
            case LOAD:
                menuVisible = false;
                loadGameState();
                break;
            case RESET:
                menuVisible = false;
                reset();
                break;
        }
    }
}

void handleButtonInput() {
    if(M5.BtnB.wasPressed()){
        buttonAPressStartTime = millis();
        aButtonPressed = true;  
    }

    if (M5.BtnB.wasReleased()) {
        aButtonPressed = false;
        if (millis() - buttonAPressStartTime >= longPressDuration && !longPressHandled) {
            menuVisible = true;
            currentMenu = BACK;
        }
    }

    if (M5.BtnA.wasReleased()) {
        if(gameVariant == 0){
            if(currentVariant == FIRST){
                gameVariant = 1;
            }else{
                gameVariant = 2;
            }
        }
        if (gameLoop == 0) {
            gameLoop = 1;
            reset();
        } else if (selected == 0) {
            player.selectedDiamond = &diamonds[player.selectedDiamondX][player.selectedDiamondY];
            selected = 1;
        } else {
            swapDiamonds(player.selectedDiamond, &diamonds[player.selectedDiamondX][player.selectedDiamondY]);
            if (checkForMatches()) {
                player.score += 10;
                player.moves -= 1;
                removeMatchingDiamonds();
            } else {
                if (gameVariant == 1) {
                    swapDiamonds(player.selectedDiamond, &diamonds[player.selectedDiamondX][player.selectedDiamondY]);
                    Serial.println("No matches found, swapping back!");
                }
            }
            selected = 0;
        }
    }
}

void loop() {
    M5.update();
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(10, 20);

    if (gameVariant == 0) {
        displayBeginScreen();
        accelerometer();
        handleButtonInput();
    } else if (menuVisible) {
        displayMenu();
        handleMenuInput();
        accelerometer();
    } else if (gameLoop == 0) {
        drawLoseScreen();
        handleButtonInput();
    } else {
        updateGame();
        /*if (checkForMatches()) {
            player.score += 10;
            removeMatchingDiamonds();
        }*/
        drawDiamonds();
        handleButtonInput();
        accelerometer();
        drawSelector();
        displayPlayerMove(player.moves);
        displayPlayerScore(player.score);
        delay(100);
    }
}