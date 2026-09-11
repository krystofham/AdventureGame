#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int width_height;

#define WIDTHEIGHT_OF_FIELD 10
#define NUMBER_OF_MOBS 3
#define SAFEZONE 2

typedef struct {
  int x_ax;
  int y_ax;
  char fill;
} field_t;

typedef struct {
  field_t field[WIDTHEIGHT_OF_FIELD];
} row_t;

typedef struct {
  row_t row[WIDTHEIGHT_OF_FIELD];
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

typedef struct {
  mob_t mob[NUMBER_OF_MOBS];
} mobs_t;

void draw_plan(const plan_t *plan) {
  for (int y = 0; y < WIDTHEIGHT_OF_FIELD; y++) {
    printf("\n");
    for (int x = 0; x < WIDTHEIGHT_OF_FIELD; x++) {
      printf("%c", plan->row[y].field[x].fill);
    }
  }
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
  if (player->position.x_ax >= WIDTHEIGHT_OF_FIELD) {
    player->position.x_ax--;
  }
  if (player->position.y_ax < 0) {
    player->position.y_ax++;
  }
  if (player->position.y_ax >= WIDTHEIGHT_OF_FIELD) {
    player->position.y_ax--;
  }
}
void fill_plan(plan_t *plan, const gold_t *gold, const player_t *player,
               const mobs_t *mobs) {
  for (int y = 0; y < WIDTHEIGHT_OF_FIELD; y++) {
    for (int x = 0; x < WIDTHEIGHT_OF_FIELD; x++) {
      if (player->position.x_ax == x && player->position.y_ax == y) {
        plan->row[y].field[x].fill = 'P';
      } else if (gold->position.x_ax == x && gold->position.y_ax == y) {
        plan->row[y].field[x].fill = 'G';
      } else {
        plan->row[y].field[x].fill = ' ';
      }
      for (int mob = 0; mob < NUMBER_OF_MOBS; mob++) {
        if (x == mobs->mob[mob].position.x_ax &&
            y == mobs->mob[mob].position.y_ax) {
          plan->row[y].field[x].fill = 'M';
        }
      }
    }
  }
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
    if (mob->position.x_ax <= WIDTHEIGHT_OF_FIELD) {
      mob->position.x_ax++;
    }
    break;
  case 2:
    if (mob->position.y_ax > 0) {
      mob->position.y_ax--;
    }
    break;
  case 3:
    if (mob->position.y_ax <= WIDTHEIGHT_OF_FIELD) {
      mob->position.y_ax++;
    }
    break;
  default:
    break;
  }
}
void move_mobs(mobs_t *mobs, const gold_t *gold, const player_t *player) {
  for (int i = 0; i < NUMBER_OF_MOBS; i++) {
    mob_t mob = mobs->mob[i];
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
    if (isinSafezone(gold, mobs->mob[i].position.x_ax,
                     mobs->mob[i].position.y_ax)) {
      int ygo = gold->position.y_ax;
      int xgo = gold->position.x_ax;
      int ymo = mobs->mob[i].position.y_ax;
      int xmo = mobs->mob[i].position.x_ax;

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
    mobs->mob[i] = mob;
  }
}
int lost(const player_t *player, const mobs_t *mobs) {
  for (int i = 0; i < NUMBER_OF_MOBS; i++) {
    if (mobs->mob[i].position.x_ax == player->position.x_ax &&
        mobs->mob[i].position.y_ax == player->position.y_ax) {
      return 1;
    }
  }
  return 0;
}
int main() {
  srand((unsigned int)time( // NOLINT
      NULL));               // NOLINT
  // init player

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

  mobs_t mobs;
  // init mobs
  int iq[] = {1, 5, 3};
  int speed[] = {6, 2, 2};
  for (int x = 0; x < NUMBER_OF_MOBS; x++) {
    mobs.mob[x].iq = iq[x];
    mobs.mob[x].speed = speed[x];
    mobs.mob[x].position.x_ax = 4 + x;
    mobs.mob[x].position.y_ax = 4 + x;
  }
  // define plan

  plan_t plan;
  for (int y = 0; y < WIDTHEIGHT_OF_FIELD; y++) {
    for (int x = 0; x < WIDTHEIGHT_OF_FIELD; x++) {
      plan.row[y].field[x].x_ax = x;
      plan.row[y].field[x].y_ax = y;
    }
  }
  fill_plan(&plan, &gold, &player, &mobs);

  int where;
  int field_is_equal;
  int victory;
  do {
    draw_plan(&plan);
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
      exit(0);
    default:
      break;
    }
    check_if_valid(&player);
    fill_plan(&plan, &gold, &player, &mobs);
    field_is_equal = isequal(&player, &gold);
    victory = isequal(&player, &gold);
    if (!field_is_equal) {
      field_is_equal = lost(&player, &mobs);
      victory = !field_is_equal;
    }
    move_mobs(&mobs, &gold, &player);
  } while (!field_is_equal);
  draw_plan(&plan);
  if (victory) {
    isvictory();
  } else {
    islost();
  }
}
