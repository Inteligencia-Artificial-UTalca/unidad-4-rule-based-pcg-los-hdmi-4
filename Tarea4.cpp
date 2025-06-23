#include <iostream>
#include <vector>
#include <random>
#include <ctime>

//Settings del mapa
const int FILAS = 20;
const int COLUMNAS = 40;
const double PROB_MURO = 0.45; // probabilidad del 45% de que salga un muro


void GenerarMapa(std::vector<std::vector<char>>& mapa, double probabilidadMuro)
{
    std::mt19937 rng(static_cast<unsigned int>(time(nullptr)));
    std::uniform_real_distribution<> dist(0.0, 1.0);

    for(int i = 0; i < FILAS; i++)
    {
      for(int j = 0; j < COLUMNAS; j++)
      {
        if(dist(rng) < probabilidadMuro)
        {
          mapa[i][j] = '#'; //un muro
        }
        else
        {
          mapa[i][j] = '.'; //camino
        }
      }
    }
}


int contarMurosVecinos(const std::vector<std::vector<char>>& mapa, int x, int y)
{
  int contadorMuros = 0;
  for(int i = -1; i <= 1; i++)
  {
    for(int j = -1; j <= 1; j++)
    {
      int nx= x + i;
      int ny = y + j;

      if(i == 0 && j == 0)//no cuenta la celda central
      { 
        continue;
      }

      if(nx >= 0 && nx < FILAS && ny >= 0 && ny < COLUMNAS)
      {
        contadorMuros++;
      }
      else //fuera del borde
      {
        contadorMuros++;
      }

    }
  }
  return contadorMuros;
}


//iteración de automata celular
void SimularPaso(std::vector<std::vector<char>>& mapa)
{
    std::vector <std::vector<char>> nuevoMapa = mapa;

     for(int i = 0; i < FILAS; i++)
    {
      for(int j = 0; j < COLUMNAS; j++)
      {
        int murosVecinos = contarMurosVecinos(mapa, i, j);

        if(murosVecinos > 4)
        {
          nuevoMapa[i][j] = '#'; //lo convierte en muro
        }
        else if(murosVecinos < 4)
        {
          nuevoMapa[i][j] = '.'; //Lo convierte en camino pero si son 4 no cambia
        }

      }
    }
    mapa = nuevoMapa;
}

//imprimir el mapa en la consola
void MostrarMapa(const std::vector<std::vector<char>>& mapa)
{
  for(const auto& fila : mapa)
  {
    for(char celda : fila)
    {
       std::cout << celda;
    }
    std::cout << std::endl;
  }
}


int main() 
{
   //Primero crear el mapa
   std::vector<std::vector<char>> mapa(FILAS, std::vector<char>(COLUMNAS));

   GenerarMapa(mapa, PROB_MURO);

   //Despues hay que suavizar el mapa iterandolo varias veces
   int iteraciones = 5;

   for(int i = 0; i < iteraciones; i++)
   {
     SimularPaso(mapa);
   }

   //Finalmente se muestra el mapa por consola
   MostrarMapa(mapa);
    return 0;
}