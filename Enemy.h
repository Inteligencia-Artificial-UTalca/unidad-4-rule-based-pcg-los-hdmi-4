
#ifndef ENEMIES_H
#define ENEMIES_H

#include <vector>
#include "perlin_noise.h"

void PlacedEnemyPerlin(std::vector<std::vector<char>>& map, double frecuencia, double umbral, char enemy_char, char floor_char,
                                                        double porcentajeSpawnMax, int securityRatio );

#endif 