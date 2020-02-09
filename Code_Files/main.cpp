#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <time.h>
#include <fstream>
using namespace std;

//initializing the mutex variables
pthread_mutex_t mutex_for_server = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

//declaring the global variables
bool *wanted = NULL;
bool *reserved = NULL;
int seatnumber;
//opening the output file
ofstream out("output.txt");

//using it for message passing from main thread to clients
typedef struct _thread_data_t {
  int tid;
} thread_data_t;

//server method
void *server(void *param) {
    //taking the coming messages
    int *comingmessages = (int *) param;
    //the client id
    int peerid = *comingmessages;
    //client's desired seat number
    int peerwants = *(comingmessages + 1);

    //locking since the global variables are being changed in here
    pthread_mutex_lock(&mutex_for_server);
    //if the seat is not reserved
    if(reserved[peerwants] == false){
        //reserve the seat for the client 
        reserved[peerwants] = true;
        //also mark it as wanted even if it is marked already in client's method
        wanted[peerwants] = true;
        //print the current reservation to the file
        out << "Client" << peerid << " reserves Seat" << peerwants << endl;
    }
    // unlocking back
    pthread_mutex_unlock(&mutex_for_server);
    return NULL;
}

// client method
void *client(void *myid) {

    //to make random function work better but actually doesn't make much difference :(
    srand((unsigned)time(NULL));
    //make the thread sleep for a random time
    sleep((random() % 151 + 50) / 1000);

    // getting the data from main thread
    thread_data_t *data = (thread_data_t *)myid;
    int myidnum = data->tid;
    //selecting a random seat
    int wanttoselect = (random() % seatnumber + 1);
    //while wanted seat is not empty, try to find empty seat
    while(wanted[wanttoselect]){ 
        wanttoselect =  (random() % seatnumber + 1);
    }
    //since the wanted list will be changed it is criticak section
    pthread_mutex_lock(&m);
    //check if the selected seat is wanted by another client, before sending the seat number to server
    if(!wanted[wanttoselect]){
        wanted[wanttoselect] = true;
    }
    //if it is selected by another client before, even if it is not reserved yet, 
    //this client should select another seat which is empty
    else{
        while(wanted[wanttoselect]){
            wanttoselect = (random() % seatnumber) + 1;
        }
        wanted[wanttoselect] = true;
    }
    // no longer critical section since the global variables are not being changed
    pthread_mutex_unlock(&m);

    //initilazing the array which will be sent to server thread
    int sendtoserver[2];
    //sending the id
    sendtoserver[0] = myidnum;
    //sending the selected seat
    sendtoserver[1] = wanttoselect;
    //the server thread for this client thread
    pthread_t serverthread;
    //creating the server for this client thread and sending the id and seat number
    pthread_create(&serverthread, NULL, server, (void*) &sendtoserver);

    // wait for server thread to finish its work
    pthread_join(serverthread, NULL);

    return NULL;
}



int main(int argc, char *argv[]) {

    //if given arguments are not proper, print a message and return
    if (argc != 2 || atoi(argv[1])<50 || atoi(argv[1])>100) {
        cout << "Run the code with the following command: ./project2 'an integer between 50 and 100' " << endl;
        return 1;
    }
    //reading the seatnumber
    seatnumber = atoi(argv[1]);

    //printing the first row of the output file
    out << "Number of total seats: " << seatnumber << endl;
    //dynamically allocating memory for wanted and reserved
    wanted = (bool *) malloc ((seatnumber+1) * sizeof (bool));
    reserved = (bool *) malloc ((seatnumber+1) * sizeof (bool));

    //since there is no seat numbered as zero
    wanted[0] = true;
    reserved[0] = true;

    //to make random function work better but actually doesn't make much difference :(
    srand((unsigned)time(NULL));

    //client threads' array
    pthread_t threadarray[seatnumber];
    //thread id numbers' array
    int threadids[seatnumber];
    //to use while sending data between threads
    thread_data_t thr_data[seatnumber];

    //creating client threads with their ids
    for(int i=0; i<seatnumber; i++){
        thr_data[i].tid = i+1;
        threadids[i] = i+1;
        pthread_create(&threadarray[i], NULL, client, (void *) &thr_data[i]);
    }
    //to make main thread wait for client threads to finish
    for(int i=0; i<seatnumber; i++){
        pthread_join(threadarray[i], NULL);
    }
    //printing the last row
    out << "All seats are reserved." ;
    //deleting the dynamically allocated arrays
    delete[] wanted;
    delete[] reserved;
    return 0;
}
