
#include "Enemy.h"
#include <iostream>
#include <queue>
#include <utility>
#include <vector>
#include <random>
#include <algorithm>



void PlacedEnemyPerlin(std::vector<std::vector<char>>& map, double frecuencia, double umbral, char enemy_char, char floor_char,
                                               double porcentajeSpawnMax, int securityRatio)
{
 PerlinNoise pn;
    int enemigos_colocados = 0;
    std::vector<std::pair<int, int>> celdas_disponibles;

    // Encuentra al player y la salida
    int player_x = -1, player_y = -1, exit_x = -1, exit_y = -1;
    for (int y = 0; y < (int)map.size(); ++y) 
    {
        for (int x = 0; x < (int)map[y].size(); ++x) 
        {
            if (map[y][x] == 'P') { player_x = x; player_y = y; }
            if (map[y][x] == 'S') { exit_x = x; exit_y = y; }
        }
    }

    std::cout << "=== DEBUG PERLIN ENEMY PLACEMENT ===\n";
    std::cout << "Player: (" << player_x << ", " << player_y << ")\n";
    std::cout << "Salida: (" << exit_x << ", " << exit_y << ")\n";
    std::cout << "Parametros - Freq: " << frecuencia << ", Umbral: " << umbral 
              << ", MaxSpawn: " << porcentajeSpawnMax << ", Security: " << securityRatio << "\n";

    // Contar todas las celdas de suelo primero
    int total_floor_cells = 0;
    for (int y = 0; y < (int)map.size(); ++y) 
    {
        for (int x = 0; x < (int)map[y].size(); ++x)
         {
            if (map[y][x] == floor_char) 
            {
                total_floor_cells++;
            }
        }
    }
    std::cout << "Total celdas de suelo: " << total_floor_cells << "\n";

    // Junta posibles posiciones para enemigos (SIN área de seguridad por ahora para debug)
    int excluded_by_player = 0, excluded_by_exit = 0;
    for (int y = 0; y < (int)map.size(); ++y) {
        for (int x = 0; x < (int)map[y].size(); ++x) 
        {
            if (map[y][x] == floor_char) 
            {
                bool skip = false;
                
                if (player_x != -1 && player_y != -1) 
                {
                    int dist_player = std::abs(x - player_x) + std::abs(y - player_y);
                    if (dist_player <= securityRatio) 
                    {
                        skip = true;
                        excluded_by_player++;
                    }
                }
                
                if (exit_x != -1 && exit_y != -1) 
                {
                    int dist_exit = std::abs(x - exit_x) + std::abs(y - exit_y);
                    if (dist_exit <= securityRatio) 
                    {
                        if (!skip) excluded_by_exit++;  // Solo contar si no fue excluido ya por player
                        skip = true;
                    }
                }
                
                if (!skip) 
                {
                    celdas_disponibles.push_back({x, y});
                }
            }
        }
    }

    std::cout << "Excluidas por player: " << excluded_by_player << "\n";
    std::cout << "Excluidas por salida: " << excluded_by_exit << "\n";
    std::cout << "Celdas disponibles: " << celdas_disponibles.size() << "\n";

    if (celdas_disponibles.empty()) 
    {
        std::cout << "[ERROR] No hay celdas disponibles. Prueba reducir securityRatio.\n";
        return;
    }

    // Aplicar Perlin noise y calcular estadísticas
    std::vector<std::tuple<double, int, int>> all_noise_values;
    double min_noise = 1.0, max_noise = 0.0, sum_noise = 0.0;
    
    for (auto& p : celdas_disponibles) 
    {
        double n = pn.noise(p.first * frecuencia, p.second * frecuencia);
        n = (n + 1.0) / 2.0;  // Normalizar de [-1,1] a [0,1]
        
        all_noise_values.push_back({n, p.first, p.second});
        min_noise = std::min(min_noise, n);
        max_noise = std::max(max_noise, n);
        sum_noise += n;
    }
    
    double avg_noise = sum_noise / celdas_disponibles.size();
    std::cout << "Ruido - Min: " << min_noise << ", Max: " << max_noise << ", Promedio: " << avg_noise << "\n";

    // Contar cuántos superan el umbral
    std::vector<std::tuple<double, int, int>> above_threshold;
    for (auto& item : all_noise_values) 
    {
        if (std::get<0>(item) > umbral) 
        {
            above_threshold.push_back(item);
        }
    }
    
    std::cout << "Posiciones sobre umbral " << umbral << ": " << above_threshold.size() << "\n";

    // Si muy pocas o ninguna supera el umbral, usar un enfoque adaptativo
    std::vector<std::tuple<double, int, int>> selected_positions;
    
    if (above_threshold.empty()) 
    {
        std::cout << "[INFO] Umbral muy alto, usando posiciones con mayor ruido...\n";
        // Ordenar todas por ruido descendente
        std::sort(all_noise_values.rbegin(), all_noise_values.rend());
        // Tomar al menos el 25% de las mejores posiciones
        int min_count = std::max(1, (int)(celdas_disponibles.size() * 0.25));
        for (int i = 0; i < min_count && i < (int)all_noise_values.size(); ++i) 
        {
            selected_positions.push_back(all_noise_values[i]);
        }
    } else if (above_threshold.size() < 3) 
    {
        std::cout << "[INFO] Pocas posiciones sobre umbral, complementando...\n";
        // Usar las que superan el umbral + algunas de las mejores restantes
        selected_positions = above_threshold;
        std::sort(all_noise_values.rbegin(), all_noise_values.rend());
        
        int needed = std::min(5, (int)celdas_disponibles.size()) - (int)above_threshold.size();
        for (auto& item : all_noise_values) 
        {
            if (needed <= 0) break;
            
            // Verificar que no esté ya en selected_positions
            bool already_selected = false;
            for (auto& selected : selected_positions) 
            {
                if (std::get<1>(selected) == std::get<1>(item) && 
                    std::get<2>(selected) == std::get<2>(item)) 
                {
                    already_selected = true;
                    break;
                }
            }
            
            if (!already_selected) 
            {
                selected_positions.push_back(item);
                needed--;
            }
        }
    } else 
    {
        selected_positions = above_threshold;
    }

    // Ordenar por un score descendente
    std::sort(selected_positions.rbegin(), selected_positions.rend());

    // Calcular cuantos enemigos colocar
    int max_enemigos = std::max(50, (int)(celdas_disponibles.size() * porcentajeSpawnMax));
    int cantidad = std::min((int)selected_positions.size(), max_enemigos);

    std::cout << "Enemigos a colocar: " << cantidad << " (max permitido: " << max_enemigos << ")\n";

    // Colocar enemigos
    for (int i = 0; i < cantidad; ++i) 
    {
        int x = std::get<1>(selected_positions[i]);
        int y = std::get<2>(selected_positions[i]);
        double noise_val = std::get<0>(selected_positions[i]);
        
        std::cout << "Colocando enemigo " << (i+1) << " en (" << x << ", " << y << ") con ruido " << noise_val << "\n";
        
        map[y][x] = enemy_char;
        enemigos_colocados++;
    }

    std::cout << "=== RESULTADO: " << enemigos_colocados << " enemigos colocados ===\n\n";
}