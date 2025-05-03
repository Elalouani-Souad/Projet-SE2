#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define NB_BUS 9
#define NB_BUS_X 5
#define NB_BUS_Y 4
#define NB_ALLER_RETOUR 10

// Variables de contrôle
int sens = 0;  // 0 = vide, 1 = X->Y, -1 = Y->X
int bus_dans_tunnel = 0;
int attente_x = 0;
int attente_y = 0;

// Synchronisation
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_x;
sem_t sem_y;

// Structure pour passer les infos à chaque thread
typedef struct {
  int id;
  char ville_depart;
} Bus;

void entrer_tunnel(char ville_depart) {
  pthread_mutex_lock(&mutex);

  if (ville_depart == 'X') {
    attente_x++;
    while (sens == -1 || (sens == 0 && attente_y > 0)) {
      pthread_mutex_unlock(&mutex);
      sem_wait(&sem_x);
      pthread_mutex_lock(&mutex);
    }
    attente_x--;
    sens = 1;
  } else {
    attente_y++;
    while (sens == 1 || (sens == 0 && attente_x > 0)) {
      pthread_mutex_unlock(&mutex);
      sem_wait(&sem_y);
      pthread_mutex_lock(&mutex);
    }
    attente_y--;
    sens = -1;
  }

  bus_dans_tunnel++;
  pthread_mutex_unlock(&mutex);
}

void sortir_tunnel() {
  pthread_mutex_lock(&mutex);
  bus_dans_tunnel--;

  if (bus_dans_tunnel == 0) {
    if (sens == 1 && attente_y > 0) {
      sens = -1;
      for (int i = 0; i < attente_y; i++) sem_post(&sem_y);
    } else if (sens == -1 && attente_x > 0) {
      sens = 1;
      for (int i = 0; i < attente_x; i++) sem_post(&sem_x);
    } else {
      sens = 0;
    }
  }

  pthread_mutex_unlock(&mutex);
}

void* trajet(void* arg) {
  Bus* bus = (Bus*) arg;
  char depart = bus->ville_depart;
  char arrivee = (depart == 'X') ? 'Y' : 'X';

  for (int i = 0; i < NB_ALLER_RETOUR; i++) {
    printf("Bus %d attend pour aller de %c vers %c\n", bus->id, depart, arrivee);
    entrer_tunnel(depart);
    printf("Bus %d entre dans le tunnel pour aller de %c vers %c\n", bus->id, depart, arrivee);

    int temps = 1000000 + rand() % 500001;
    usleep(temps); // Simulation du trajet

    sortir_tunnel();
    printf("Bus %d est sorti du tunnel à %c\n", bus->id, arrivee);

    // Changement de direction
    char tmp = depart;
    depart = arrivee;
    arrivee = tmp;
  }

  pthread_exit(NULL);
}

int main() {
  pthread_t threads[NB_BUS];
  Bus bus[NB_BUS];

  srand(time(NULL));
  sem_init(&sem_x, 0, 0);
  sem_init(&sem_y, 0, 0);

  printf("===== DEMARRAGE DU SYSTEME ======\n\n");

  // Création des 5 bus de X
   printf("[MAIN] Création les 5 bus de 'X' \n");
  for (int i = 0; i < NB_BUS_X; i++) {
    bus[i].id = i + 1;
    bus[i].ville_depart = 'X';
    printf("[MAIN] Création du bus %d au départ de %c \n", bus[i].id, bus[i].ville_depart);
    if (pthread_create(&threads[i], NULL, trajet, &bus[i]) != 0) {
      perror("Erreur création thread");
      exit(EXIT_FAILURE);
    }
  }

  // Création des 4 bus de Y
  printf("[MAIN] Création les 4 bus de 'Y' \n");
  for (int i = 0; i < NB_BUS_Y; i++) {
    int idx = NB_BUS_X + i;
    bus[idx].id = idx + 1;
    bus[idx].ville_depart = 'Y';
    printf("[MAIN] Création du bus %d au départ de %c \n", bus[idx].id, bus[idx].ville_depart);
    
    if (pthread_create(&threads[idx], NULL, trajet, &bus[idx]) != 0) {
      perror("Erreur création thread");
      exit(EXIT_FAILURE);
    }
  }

  // Attente des threads
  for (int i = 0; i < NB_BUS; i++) {
    pthread_join(threads[i], NULL);
    printf("[MAIN] Bus %d a terminé.\n", bus[i].id);
  }

  sem_destroy(&sem_x);
  sem_destroy(&sem_y);

  printf("\n===== TOUS LES BUS ONT TERMINE =====\n");
  return 0;
}






