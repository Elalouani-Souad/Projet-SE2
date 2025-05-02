#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
#include<time.h>


#define NB_BUS 10
#define NB_BUS_X 5
#define NB_BUS_Y 4
#define NB_ALLER_RETOUR 10

// Sémaphores et variables de controle
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //protéger les variables partagées
sem_t tunnel;
int sens = 0; // 0:vide,  1: X--> Y,  -1:Y--> X
int bus_dans_tunnel=0; // cmpt du nbr de bus actuellement dans le tunnel
int attente_x = 0; //nbr de bus en attente à X
int attente_y = 0; //nbr de bus en attente à Y


//chaque thread va recevoir un objet Bus (id + ville_depart) 
typedef struct {
  int id;
  char ville_depart;
} Bus;  


// FONCTION DU TRAJET D'UN BUS

void entrer_tunnel(char ville_depart){
  pthread_mutex_lock(&mutex); //Bus demande à entrer dans le tunnel
  
  if(ville_depart == 'X'){
    attente_x++; //On signale que ce bus ettend à X
    
    // s'il y a des bus dans l'autre sens (sens == -1) il attend
    // si le tunnel est vide (sens ==0) on donne la priorité à la ville qui attend depuis plus longtemps
    //une fois les condition remplies ,on autorise l'entrée dans le tunnel et on définit le sens
    while(sens == -1 || (sens ==0 && attente_y > 0 )){
      pthread_mutex_unlock(&mutex);
      usleep(100000); //Attente de 0.1 seconde
      pthread_mutex_lock(&mutex);
    } 
    attente_x--;  //ce bus n'attend plus, il va entrer
    sens = 1;
  
  }else{ 
    attente_y++;
    while(sens == 1 || (sens ==0 && attente_x > 0 )){
      pthread_mutex_unlock(&mutex);
      usleep(100000);
      pthread_mutex_lock(&mutex);
    } 
    attente_y--;  //ce bus n'attend plus, il va entrer
    sens = -1;
    
  }
  // on augmente le nbr de bus dans le tunnel, et on libère le verrou
  bus_dans_tunnel++;
  pthread_mutex_unlock(&mutex);
}




void sortir_tunnel(){

  pthread_mutex_lock(&mutex);  
  bus_dans_tunnel--; // Un bus sort du tunnel
  
  if(bus_dans_tunnel==0){
    if((sens ==1 && attente_y > 0) || (sens ==-1 && attente_x > 0)){
      sens = -sens;
    } else if(attente_x ==0 && attente_y ==0){
      sens = 0; //tunnel libre
    }
  }
  pthread_mutex_unlock(&mutex); //On libère le verrou
}


//FONCTION PRINCIPALE DE CHAQUE THREAD

void* trajet(void* arg){
  Bus* bus = (Bus*) arg; //Récupération des infos du bus
  char ville_depart = bus->ville_depart;
  char ville_arrivee = (ville_depart == 'X') ? 'Y' : 'X';
  
  for(int i=0; i< NB_ALLER_RETOUR; i++){
    // Annonce qu'il attend
    printf("Bus %d attend pour aller de %c vers %c \n", bus->id, ville_depart, ville_arrivee);
    
    // Entrée dans le tunnel
    entrer_tunnel(ville_depart);
    printf("Bus %d entre dans le tunnel pour aller de %c vers %c \n", bus->id, ville_depart, ville_arrivee);
    
    // Simulation du trajet 
    // Simulation du trajet (aléatoire entre 1 et 1.5 sec)
    int temps = 1000000 + rand() %500001; 
    usleep(temps);
    
    // Sortir du tunnel
    sortir_tunnel();
    printf("Bus %d est sorti du tunnel à %c \n", bus->id, ville_arrivee);
    
    // changement de direction pour le trajet retour
    char temp = ville_depart;
    ville_depart = ville_arrivee;
    ville_arrivee = temp;
  }
  pthread_exit(NULL); // Fin du thread
}


// Variables globales
pthread_t threads[NB_BUS];
Bus bus[NB_BUS];

int main(){
  srand(time(NULL)); // 
  
  printf("===== DEMARRAGE DU SYSTEME ======\n \n");
  
  //1- Création des threads/bus
  for(int i=0; i< NB_BUS; i++){
    bus[i].id = i+1;
    bus[i].ville_depart = (rand() % 2 == 0) ? 'X' : 'Y';
    
    printf("[MAIN] Création du bus %d au départ de %c \n", bus[i].id, bus[i].ville_depart);
    
    if(pthread_create(&threads[i], NULL, trajet, (void*) &bus[i]) !=0){
      perror("Erreur lors de la création du thread");
      exit(EXIT_FAILURE);
    }
  }
  
  printf("\n [MAIN] Tous les threads ont été créés. Attente de la fin....\n \n");
  
  //2- on attend la fin de tous les threads
  for(int i=0; i< NB_BUS; i++){
    pthread_join(threads[i], NULL);
    printf("[MAIN] Bus %d a terminé tous ses trajets. \n", bus[i].id);
  }

  printf("\n ======== TOUS LES BUS ONT TERMINE======\n");
  
  return 0;
}





