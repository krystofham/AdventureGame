#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define VELIKOST 10
#define NUMBER_OF_MOBS 3
typedef struct{
    int x_ax;
    int y_ax;
    char fill;
}field_h;

typedef struct{
    field_h field[VELIKOST];
}row_h;

typedef struct{
    row_h row[VELIKOST];
}plan_h;

typedef struct{
    field_h position;
}player_h;

typedef struct{
    field_h position;
}gold_h;

typedef struct{
    field_h position;
    int iq;
    int speed;
}mob;

typedef struct{
    mob mob[NUMBER_OF_MOBS];
}mobs;


void draw_plan(plan_h plan, gold_h gold, player_h player){
    int xa, ya, gold_xa, gold_ya, player_xa, player_ya;
    gold_xa = gold.position.x_ax;
    gold_ya = gold.position.y_ax;
    player_xa = player.position.x_ax;
    player_ya = player.position.y_ax;
    for (int y = 0; y < VELIKOST; y++) {
        printf("\n");
        for (int x = 0; x < VELIKOST; x++) {
            printf("%c", plan.row[y].field[x].fill);
        }
    }
    printf("\n");
}
char get_direction(){
    char c;
    while (isspace(c = getchar()));
    return c;
}
int isequal(player_h player, gold_h gold){
    if (player.position.x_ax == gold.position.x_ax && player.position.y_ax == gold.position.y_ax){
        return 1;
    }
    return 0;
}
void victory(void){
    printf("Vyhral jsi!");
}
player_h check_if_valid(player_h player){
    if (player.position.x_ax < 0){player.position.x_ax++;}
    if (player.position.x_ax >= VELIKOST){player.position.x_ax--;}
    if (player.position.y_ax < 0){player.position.y_ax++;}
    if (player.position.y_ax >= VELIKOST){player.position.y_ax--;}
    return player;
}
plan_h fill_plan(plan_h plan, gold_h gold, player_h player, mobs mobs){
    for (int y = 0; y < VELIKOST; y++) {
        for (int x = 0; x < VELIKOST; x++) {
            if (player.position.x_ax == x && player.position.y_ax == y){
                plan.row[y].field[x].fill = 'P';
            }
            else if (gold.position.x_ax == x && gold.position.y_ax == y){
                plan.row[y].field[x].fill = 'G';
            }
            else {
                plan.row[y].field[x].fill = '#';
            }
            for (int mob = 0; mob < NUMBER_OF_MOBS; mob++){
                if (x == mobs.mob[mob].position.x_ax && y == mobs.mob[mob].position.y_ax){
                    plan.row[y].field[x].fill = 'M';
                }
            }

        }
    }
    return plan;
}
int main(){
    // init player

    int playerX, playerY, treasureX, treasureY;
    playerX = 0;
    playerY = 0;
    treasureX = 2;
    treasureY = 2;
    player_h player;
    gold_h gold;
    gold.position.x_ax = treasureX;
    gold.position.y_ax = treasureY;
    player.position.x_ax = playerX;
    player.position.y_ax = playerY;
    

    mobs mobs;
    // init mobs
    int iq[] = {1, 5, 3};
    int speed[] = {6, 2, 2};
    for (int x = 0; x < NUMBER_OF_MOBS; x++){
        mobs.mob[x].iq = iq[x];
        mobs.mob[x].speed = speed[x];
        mobs.mob[x].position.x_ax = 4+x;
        mobs.mob[x].position.y_ax = 4+x;
    }
    // define plan

    plan_h plan;
    for (int y = 0; y < VELIKOST; y++) {
        for (int x = 0; x < VELIKOST; x++) {
            plan.row[y].field[x].x_ax = x;
            plan.row[y].field[x].y_ax = y;
        }
    }
    plan = fill_plan(plan, gold, player, mobs);



    char where;
    int field_not_equal;
    do{
        draw_plan(plan, gold, player);
        where = get_direction();
        switch (where){
            case 'a': player.position.x_ax--; break;
            case 's': player.position.y_ax++; break;
            case 'd': player.position.x_ax++; break;
            case 'w': player.position.y_ax--; break;
        }
        player = check_if_valid(player);
        plan = fill_plan(plan, gold, player, mobs);
        field_not_equal = isequal(player, gold);
    }while(!field_not_equal);
    draw_plan(plan, gold, player);
    victory();
}
