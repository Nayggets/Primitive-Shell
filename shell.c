#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

typedef unsigned int bool;
#define TRUE 1
#define FALSE 0

#define BUFFER_SIZE 4096


int numberArgs = 0;

/* prend en paramettre la commande et ca taille 
repartis tout les arguments de la commande dans un tableau de chaine de caracteres allouer dynamiquement*/
char** splitCommandArgs(char* command,int commandSize)
{
    /* recuperation du nombre d'argument et allocation des tableau*/
    int* sizePerArg = malloc(sizeof(int) * numberArgs + 1);
    char** commandArgs = malloc(sizeof(char*) * numberArgs + 1);

    int i = 0;
    int currentArg = 0;
    int sizeCommand = 0;
    while(i-1 < commandSize){ // on va recuperer et stocker la taille nescessaire pour chaque chaine de caracteres de chaque argument
        while(command[i] == ' ' && i-1 < commandSize){
            i++;
        }
        if(command[i] == '\"'){
            i++;
            while(command[i] != '\"'){
                sizeCommand++;
                i++;
            }
            sizePerArg[currentArg] = sizeCommand;
            i++;
        }
        else{
            while(command[i] != ' ' && i-1 < commandSize){
                sizeCommand++;
                i++;
            }
            sizePerArg[currentArg] = sizeCommand;
        }
        currentArg++;
        sizeCommand = 0;
    }
    for(int i = 0 ; i < numberArgs ; i++){ //allocation des chaines par taille recuperer 
        commandArgs[i] = malloc(sizeof(char) * sizePerArg[i] + 1);
    }

    i = 0;
    sizeCommand = 0;
    currentArg = 0;
    while(i-1 < commandSize){// on va recuperer chaque arguments caracteres par caracteres pour les stockers dans leur chaine allouer precedemment
        while(command[i] == ' ' && i-1 < commandSize){
            i++;
        }
        if(command[i] == '\"'){
            i++;
            while(command[i] != '\"'){
                commandArgs[currentArg][sizeCommand] = command[i];
                sizeCommand++;
                i++;
            }
            commandArgs[currentArg][sizeCommand] = '\0';
            i++;
        }
        else{
            while(command[i] != ' ' && i-1 < commandSize){
                commandArgs[currentArg][sizeCommand] = command[i];
                sizeCommand++;
                i++;
            }
            commandArgs[currentArg][sizeCommand] = '\0';
        }
        sizeCommand = 0;
        currentArg++;
    }
    free(sizePerArg);
    commandArgs[currentArg] = NULL;//on fixe la fin du tableau de chaine
    return commandArgs; 
}

/* prend en paramettre la command et sa taille
permet d'actualiser la variable global numberArgs pour connaitre le nombre d'argument qui compose commande*/
void updateNumberWord(char* command,int commandSize)
{
    numberArgs = 0;
    int i = 0;
    while(i-1 < commandSize){//on ce deplace d'argument en argument en ajoutant ++ a nbWord a chaque nouvelle argument en comptant que une chaine de caractères avec des "" est un seul argument.
        while(command[i] == ' ' && i-1 < commandSize){
            i++;
        }
        numberArgs++;
        if(command[i] == '\"'){
            i++;
            while(command[i] != '\"'){
                i++;
            }
            i++;
        }
        else{
            while(command[i] != ' ' && i-1 < commandSize){
                i++;
            }
        }
    }
}


/* prend en paramettre le nom du process a lancer 
si nom du process == exit , emet un signal exit(0) pour mettre fin au shell*/ 
bool checkIfExit(char* processName)
{
    return(strcmp(processName,"exit") == 0 ) ? TRUE : FALSE;
}

/*affichage du repertoire courrent du nom de la machine et du login */
void displayUserCredentials()
{
    char currentDirectory[1024];
    char machineName[128];
    char* userName;

    userName = getlogin();
    gethostname(machineName,128);
    getcwd(currentDirectory,1024);
    
    printf("\033[1;32m%s@%s\033[0m:\033[1;34m%s\033[1;36m$===>",userName,machineName,currentDirectory);
    printf("\033[0m"); 
    fflush(stdout);//flush de l'entrée standars pour affichage correcte
}

/* prend en paramettre le buffer de lecture et le nombre d'octet enleve le \n de fin et les espaces indesirables en mettant a jour byteInCommand*/
int eraseEndSpacing(char* command,int byteInCommand)
{
    byteInCommand = byteInCommand - 2;
    while(command[byteInCommand] == ' '){
        byteInCommand--;
        if(byteInCommand == 0){
            break;
        }
    }

    command[byteInCommand+1] = '\0';
    return byteInCommand;
}
/* prend en paramettre une command
   recupere la commande en lisant sur l'entrée standars du terminal la place dans le command enleve les espaces et saut a la ligne indesirable*/
int readCommand(char* command)
{
    int byteInCommand = 0;
    
    byteInCommand = read(0,command,BUFFER_SIZE);
    byteInCommand = eraseEndSpacing(command,byteInCommand);
    
    return byteInCommand;
}


/*ne prend pas de paramettre apelle differente fonction pour recuperer tout les args de la commande*/
char** getCommandArgs()
{
    char* command = malloc(sizeof(char) * BUFFER_SIZE);
    int byteInCommand = readCommand(command);
    updateNumberWord(command,byteInCommand);
    char** commandArgs = splitCommandArgs(command,byteInCommand);
    free(command);
    return commandArgs;
}

/*prend en argument le derniere arguments de la commande
  verifier si le dernier argument de la commande est & si oui renvois TRUE sinon renvois FALSE */
bool backgroundProcess(char* lastArg)
{
    return(strcmp(lastArg,"&") != 0) ? FALSE : TRUE;
}

/*prend en paramettres les arguments du process et l'execute 
  si l'execution echoue affiche le message d'erreur et tue le processus crée*/
pid_t execProcess(char** commandArgs)
{
    int errorCode = 0;
    pid_t pid = fork(); // fork pour crée un nouveau processus
    if(pid == 0){
        errorCode = execvp(commandArgs[0],commandArgs); // transformation du processus dupliquer avec la nouvelle image 
        if(errorCode != 0){
            printf("%s\n",strerror(errno));
            kill(getpid(),-1);
        }
    }
    return pid;
}

/* run le shell*/
void run()
{
    pid_t pid = 0;
    char** commandArgs;
    bool background = FALSE;
    while(TRUE){
        displayUserCredentials();
        commandArgs = getCommandArgs();
        if(checkIfExit(commandArgs[0]) == TRUE){
            for(int i = 0 ; i < numberArgs ; i++){
                free(commandArgs[i]);
            }
            free(commandArgs);
            exit(0);
        } 

        if(backgroundProcess(commandArgs[numberArgs - 1]) == TRUE){

            background = TRUE;
            commandArgs[numberArgs - 1] = NULL;
            numberArgs--;
        }

        pid = execProcess(commandArgs);

        if(background == FALSE){
            waitpid(pid,NULL,NULL);
        }

        while(numberArgs != 0){
            numberArgs--;
            free(commandArgs[numberArgs]);
        }
        free(commandArgs);
        background = FALSE;
    }
}

int main()
{
    run();
    return 0;      
}

