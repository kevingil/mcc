#include "level_data.h"

#include <stdio.h>

int main(void)
{
    LevelData level = { 0 };

    LevelDataInit(&level);
    printf("opencraft-server: common linked (%s), no world loop yet\n", level.name);
    return 0;
}
