#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SAFEZONE 2

//YET to make - fog of war
//Randim pozice
//Bitové příznaky bunky
//Nepouzivat abs, heuristicke fce
//Djiskra
//Raw terminal input
// FIELD SHOULD HAVE bit operation 
//      0001 - seen 
//      0010 - blocked
//      0100 - MINE or smt like that

typedef struct {
  int x_ax;
  int y_ax;
  char fill;
} field_t;

typedef struct {
  field_t field;
} plan_t;

typedef struct {
  field_t position;
} player_t;

typedef struct {
  field_t position;
} gold_t;

typedef struct {
  field_t position;
  int iq;
  int speed;
} mob_t;

int width_height;
int mobs_num;

int distance_to(field_t posA, field_t posB);
void draw_plan(const plan_t *plan);
int get_direction();
int isequal(const player_t *player, const gold_t *gold);
void isvictory(void);
void islost(void);
void check_if_valid(player_t *player);
void fill_plan(plan_t *plan, const gold_t *gold, const player_t *player,
               const mob_t *mobs);
int isinSafezone(const gold_t *gold, int positionX, int positionY);
void move_mobs_move(mob_t *mob, const gold_t *gold);
void move_mobs_know_where(const player_t *player, mob_t *mob);
void move_mobs_dont_know(mob_t *mob);
void move_mobs(mob_t *mobs, const gold_t *gold, const player_t *player);
int lost(const player_t *player, const mob_t *mobs);

int main() {
  srand((unsigned int)time( // NOLINT
      NULL));               // NOLINT
  // init player
  printf("Enter width and height of game field: ");
  scanf("%d", &width_height); // NOLINT

  printf("Enter number of mobs: ");
  scanf("%d", &mobs_num); // NOLINT

  int playerX, playerY, treasureX, treasureY;
  playerX = 0;
  playerY = 0;
  treasureX = 2;
  treasureY = 2;
  player_t player;
  gold_t gold;
  gold.position.x_ax = treasureX;
  gold.position.y_ax = treasureY;
  player.position.x_ax = playerX;
  player.position.y_ax = playerY;

  mob_t *mobs = malloc(sizeof *mobs * (size_t)mobs_num);
  if (mobs == NULL) {
    printf("Insuficient memory for mobs");
    return 1;
  }
  // init mobs
  int iq[] = {1, 5, 3};
  int speed[] = {6, 2, 2};
  for (size_t x = 0; x < (size_t)mobs_num; x++) {
    mobs[x].iq = iq[x < (sizeof(iq) / sizeof(iq[0])) ? 3 : x];
    mobs[x].speed = speed[x < (sizeof(speed) / sizeof(speed[0])) ? 2 : x];
  }
  for (int x = 0; x < mobs_num; x++) {
    mobs[x].position.x_ax = 4 + x;
    mobs[x].position.y_ax = 4 + x;
  }
  // define plan

  plan_t *plan =
      malloc(sizeof *plan * (size_t)width_height * (size_t)width_height);
  if (plan == NULL) {
    printf("Insuficient memory for plan");
    return 1;
  }
  int *p = malloc(sizeof(*p));
  *p = 0;
  for (int y = 0; y < width_height; y++) {
    for (int x = 0; x < width_height; x++) {
      plan[*p].field.x_ax = x;
      plan[*p].field.y_ax = y;
      (*p)++;
    }
  }
  free(p);

  fill_plan(plan, &gold, &player, mobs);

  int where;
  int field_is_equal;
  int victory;
  do {
    draw_plan(plan);
    where = get_direction();
    switch (where) {
    case 97: // s
      player.position.x_ax--;
      break;
    case 115: // w
      player.position.y_ax++;
      break;
    case 100: // d
      player.position.x_ax++;
      break;
    case 119: // a
      player.position.y_ax--;
      break;
    case 113: // q
      return 0;
    default:
      break;
    }
    check_if_valid(&player);
    fill_plan(plan, &gold, &player, mobs);
    field_is_equal = isequal(&player, &gold);
    victory = isequal(&player, &gold);
    if (!field_is_equal) {
      field_is_equal = lost(&player, mobs);
      victory = !field_is_equal;
    }
    move_mobs(mobs, &gold, &player);
  } while (!field_is_equal);
  draw_plan(plan);
  if (victory) {
    isvictory();
  } else {
    islost();
  }
  free(plan);
  free(mobs);
  return 0;
}

void draw_plan(const plan_t *plan) {
  int *p = malloc(sizeof(*p));
  *p = 0;
  for (int y = 0; y < width_height; y++) {
    printf("\n");
    for (int x = 0; x < width_height; x++) {
      printf("%c", plan[*p].field.fill);
      (*p)++;
    }
  }
  free(p);
  printf("\n");
}
int get_direction() {
  int c = getchar();
  while (
      c != EOF &&
      isspace((unsigned char)c)) { // NOLINT(clang-analyzer-security.ArrayBound)
    c = getchar();
  }
  return c;
}
int isequal(const player_t *player, const gold_t *gold) {
  if (player->position.x_ax == gold->position.x_ax &&
      player->position.y_ax == gold->position.y_ax) {
    return 1;
  }
  return 0;
}
void isvictory(void) { printf("You won!"); }
void islost(void) { printf("You lost!"); }
void check_if_valid(player_t *player) {
  if (player->position.x_ax < 0) {
    player->position.x_ax++;
  }
  if (player->position.x_ax >= width_height) {
    player->position.x_ax--;
  }
  if (player->position.y_ax < 0) {
    player->position.y_ax++;
  }
  if (player->position.y_ax >= width_height) {
    player->position.y_ax--;
  }
}
int distance_to(const field_t posA, const field_t posB) {
  int len = abs(posA.x_ax - posB.x_ax) + abs(posA.y_ax - posB.y_ax);  
  return len;
}
void fill_plan(plan_t *plan, const gold_t *gold, const player_t *player,
               const mob_t *mobs) {
  int *p = malloc(sizeof(*p));
  *p = 0;
  for (int y = 0; y < width_height; y++) {
    for (int x = 0; x < width_height; x++) {
      if (player->position.x_ax == x && player->position.y_ax == y) {
        plan[*p].field.fill = 'P';
      } else if (gold->position.x_ax == x && gold->position.y_ax == y) {
        plan[*p].field.fill = 'G';
      } else {
        plan[*p].field.fill = ' ';
      }
      for (int mob = 0; mob < mobs_num; mob++) {
        if (x == mobs[mob].position.x_ax && y == mobs[mob].position.y_ax) {
          plan[*p].field.fill = 'M';
        }
      }
      if (distance_to(player->position, plan[*p].field) > 2) {
        plan[*p].field.fill = '?';
      }
      (*p)++;
    }
  }
  free(p);
}
int isinSafezone(const gold_t *gold, int positionX, int positionY) {
  if (abs(gold->position.x_ax - positionX) < SAFEZONE &&
      abs(gold->position.y_ax - positionY) < SAFEZONE) {
    return 1;
  }
  return 0;
}
void move_mobs_move(mob_t *mob, const gold_t *gold) {
  if (abs(gold->position.y_ax - mob->position.y_ax) >
      abs(gold->position.x_ax - mob->position.x_ax)) {
    if (gold->position.y_ax - mob->position.y_ax > 0) {
      mob->position.y_ax++;
    } else {
      mob->position.y_ax--;
    }
  } else {
    if (gold->position.x_ax - mob->position.x_ax > 0) {
      mob->position.x_ax++;
    } else {
      mob->position.x_ax--;
    }
  }
}

void move_mobs_know_where(const player_t *player, mob_t *mob) {
  if (abs(player->position.y_ax - mob->position.y_ax) >
      abs(player->position.x_ax - mob->position.x_ax)) {
    if (player->position.y_ax - mob->position.y_ax > 0) {
      mob->position.y_ax++;
    } else {
      mob->position.y_ax--;
    }
  } else {
    if (player->position.x_ax - mob->position.x_ax > 0) {
      mob->position.x_ax++;
    } else {
      mob->position.x_ax--;
    }
  }
}
void move_mobs_dont_know(mob_t *mob) {
  int a = rand() % 4; // NOLINT
  switch (a) {
  case 0:
    if (mob->position.x_ax > 0) {
      mob->position.x_ax--;
    }
    break;
  case 1:
    if (mob->position.x_ax <= width_height) {
      mob->position.x_ax++;
    }
    break;
  case 2:
    if (mob->position.y_ax > 0) {
      mob->position.y_ax--;
    }
    break;
  case 3:
    if (mob->position.y_ax <= width_height) {
      mob->position.y_ax++;
    }
    break;
  default:
    break;
  }
}
void move_mobs(mob_t *mobs, const gold_t *gold, const player_t *player) {
  for (int i = 0; i < mobs_num; i++) {
    mob_t mob = mobs[i];
    if (mob.speed > 3 || rand() % 2 == 1) { // NOLINT
      // move
      if (mob.iq > 5 || rand() % 6 == 1) { // NOLINT
        move_mobs_move(&mob, gold);
      } else if (mob.iq > 3 || rand() % 3 == 1) { // NOLINT
        // know where
        move_mobs_know_where(player, &mob);
      } else {
        // dont know
        move_mobs_dont_know(&mob);
      }
    }
    if (isinSafezone(gold, mobs[i].position.x_ax, mobs[i].position.y_ax)) {
      int ygo = gold->position.y_ax;
      int xgo = gold->position.x_ax;
      int ymo = mobs[i].position.y_ax;
      int xmo = mobs[i].position.x_ax;

      if (abs(ygo - ymo) > SAFEZONE) {
        if (ygo - ymo > 0) {
          mob.position.y_ax--;
        } else {
          mob.position.y_ax++;
        }
      } else if (abs(xgo - xmo) > SAFEZONE) {
        if (xgo - xmo > 0) {
          mob.position.x_ax--;
        } else {
          mob.position.x_ax++;
        }
      }
    }
    mobs[i] = mob;
  }
}
int lost(const player_t *player, const mob_t *mobs) {
  for (int i = 0; i < mobs_num; i++) {
    if (mobs[i].position.x_ax == player->position.x_ax &&
        mobs[i].position.y_ax == player->position.y_ax) {
      return 1;
    }
  }
  return 0;
}
