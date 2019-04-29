#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#define _REENTRANT
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>
#include <semaphore.h>
struct timeval tv1,tv2,dtv;
struct timezone tz;
typedef struct {
pthread_t tid;
int num;
int kol;
sem_t sem;
} ThreadRecord
int N;
int sum=0;
int ob=1;
int Tek=0;
int end=0;
int nt;
double *A, *B, *X;
int *kluch, *Teek;
pthread_barrier_t barr1, barr2;
ThreadRecord *threads;
int done=0;
void InitializeMatr()
{
int i,j;
srand(time(NULL));
for (i=0; i<N; i++) {
B[i] = 0.;
for (j=0; j<N; j++)
{
A[i*N+j] =(((double) rand()) / ((double) RAND_MAX))*2-1;
B[i] += A[i*N+j];
};
}
}
void PrintMatr()
{
int i,j;
for (i=0; i<N; i++) {
for (j=0; j<N; j++){
fprintf(stdout, "%.3lf \t", A[i*N+j]);
}
fprintf(stdout, "%.3lf \n", B[i]);
}
fprintf(stdout, "\n");
}
void printX()
{
int i;
for (i=0; i<N; i++) {
fprintf(stdout, "%.3lf \n", X[i]);
}
fprintf(stdout, "\n");
}
void *mysolver (void *arg_p) {
int num;
int i, j,s,c;
s=0;
double k,l,p;
ThreadRecord* thr=( ThreadRecord* )arg_p;
pthread_barrier_wait(&barr1);
for(i=thr->num;i<N;i=i+thr->kol) //проходим по строкам принадлежащим данному
потоку
{
for(s=0;s<=i;s++) //проходим по элементам строки
{
if(s==Teek[thr->num]) //совпадает ли s с проверяемым диаг. элементом
{
if(s==i) //s – диагональный? Выполниться только у одного потока
{
/*деление строки на диагональный элемент*/
p=A[N*i+i];
B[i]=B[i]/p;
for(j=0;j<N;j++)
{
A[N*i+j]=A[N*i+j]/p;
}
sum=0;
//PrintMatr();
/* запуск заблокированных потоков*/
for(c=nt-1; c>=0; c--)
{
Teek[c]=Teek[c]+1;
if (kluch[c]==1)
{
kluch[c]=0;
sem_post(&threads[c].sem);
}
}
break;
}
else
{ /*блокирование потока –поток дошёл до элемента, который должен быть обработан,
но этот элемент ему не принадлежит*/
kluch[thr->num]=1;
sem_wait(&thr->sem);
}
}
// стандартные действия метода Гаусса
B[i]=B[i]-B[s]*A[N*i+s];
for(j=N-1;j>=0;j--)
{
A[N*i+j]=A[N*i+j]-A[N*i+s]*A[N*s+j];
}
}
}
pthread_barrier_wait(&barr1); //конец прямого хода – конец работы потока
}
void time_start() { gettimeofday(&tv1, &tz); }
int time_stop()
{
gettimeofday(&tv2, &tz);
dtv.tv_sec= tv2.tv_sec -tv1.tv_sec;
dtv.tv_usec=tv2.tv_usec-tv1.tv_usec;
if(dtv.tv_usec<0) { dtv.tv_sec--; dtv.tv_usec+=1000000; }
return dtv.tv_sec*1000+dtv.tv_usec/1000;
}
int main(int argc, char* argv[]) {
int i, j;
int ish;
int ms=0;
pthread_attr_t pattr;
if (argc != 3) {
fprintf(stderr, "Исп: %s #dimension #number of tread\n", argv[0]);
exit (1);
};
N = atoi(argv[1]);
nt = atoi(argv[2]);
if ( N%nt ) {
fprintf(stderr, "The dimension must be a multiple of the number of thread\n");
exit (2);
};
//подготавливаем переменные
A = (double *) malloc( sizeof(double)*N*N );
B = (doub#define _REENTRANT
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h> 
#include <sys/types.h>

#define BUFFERSIZE 64
#define DELTATIME 0.5
#define LENGTH 10.0
#define FATHSPEED 1.0


double deltax = 0.0;
double Q = 0.0;
double M = 0.0;


double **basicA;
double **A;
double *B;
double *Z;
double *prePreviousZ;
double *previousZ;
int *countInBlock;
int countPoints;
int countThreads;
double timeLimit;
double timer=0.0;
double startCondition;
double func;

pthread_barrier_t barr1;
pthread_barrier_t barr2;


typedef struct {
   	pthread_t tid;
	int first;
	int last;
	int size;
	int i;
} Block;



Block *arrBlocks;
Block downBlock;

void returnB(){
	for(int i = 0; i < countPoints; ++i){
		B[i]=0.0;
	}
	for(int i = 0; i < countThreads; ++i){								// Формирование вектора B
		for (int j = arrBlocks[i].first; j <= arrBlocks[i].last; ++j) {
			B[j] += ((2.0/(DELTATIME*DELTATIME))*previousZ[j]);
			B[j] -= (((FATHSPEED*FATHSPEED)/(deltax*deltax))*previousZ[j]);
			B[j] -= ((1.0/(DELTATIME*DELTATIME))*prePreviousZ[j]);
			if (j == arrBlocks[i].first){
				B[j] += (((FATHSPEED*FATHSPEED)/(2.0*deltax*deltax))*previousZ[j+1]);
				if(i != 0){
					B[j]+= (((FATHSPEED*FATHSPEED)/(2.0*deltax*deltax))*previousZ[downBlock.first+i-1]);
				}
				continue;
			}
			if (j == arrBlocks[i].last){
				B[j] += (((FATHSPEED*FATHSPEED)/(2.0*deltax*deltax))*previousZ[j-1]);
				if ( i != countThreads-1 ) {
					B[j]+= (((FATHSPEED*FATHSPEED)/(2.0*deltax*deltax))*previousZ[downBlock.first+i]);
				}
				continue;
			}
			B[j] += (((FATHSPEED*FATHSPEED)/(2.0*deltax*deltax))*previousZ[j+1]);
			B[j] += (((FATHSPEED*FATHSPEED)/(2.0*deltax*deltax))*previousZ[j-1]);
		}
	}
	int ff = 0;
	for(int i = downBlock.first; i <= downBlock.last; ++i){
		B[i] +=(2.0/(DELTATIME*DELTATIME))*previousZ[i];
		B[i] -= ((FATHSPEED*FATHSPEED)/(deltax*deltax))*previousZ[i];
        B[i] -= (1.0/(DELTATIME*DELTATIME))*prePreviousZ[i];
		B[i]+=(FATHSPEED*FATHSPEED*previousZ[arrBlocks[ff].last])/(2.0*deltax*deltax);
		B[i]+=(FATHSPEED*FATHSPEED*previousZ[arrBlocks[ff+1].first])/(2.0*deltax*deltax);
		ff++;
	}
}

void backGausForDownBlock(){
	for (int i = downBlock.last; i >= downBlock.first; i--){
		double temp = 0.0;
		for (int j = downBlock.last; j >= downBlock.first ; j-- ) {
			if(i != j){
				temp+=Z[j]*A[i][j];
			}
		}
		Z[i]=(B[i]-temp)/(A[i][i]);
	}
}
void forwardGausForDownBlock(){
	for (int j = downBlock.first; j <= downBlock.last; ++j){
		if (A[j][j] == 0.0){
			continue;
		}
		if(j != downBlock.last){
			double temp = A[j+1][j]/A[j][j];
			for (int k = 0; k < countPoints; ++k){
				A[j+1][k]-=A[j][k]*temp;
			}
			B[j+1]-=B[j]*temp;
		}
	}
}

void print_result(){
	for (int i = 0; i < countPoints; ++i){
		printf("Z[%d]=%.4lf \t preZ[%d]=%.4lf \t ppreZ[%d]=%.4lf \n", i, Z[i],i ,previousZ[i],i, prePreviousZ[i] );
	}
	printf("\n");
}
void print_B(){
	for (int i = 0; i < countPoints; ++i){
		printf("B[%d]=%.4lf\n", i, B[i] );
	}
	printf("\n");
}
void print_matrix(){
   for (int i = 0; i < countPoints; i++)
   {
    	for (int j = 0; j < countPoints; j++) 
      		printf("%.4lf\t",A[i][j]);
      	printf("\n");
   }
   printf("\n");
}

void* mysolver(void* arg){
    Block* bl = (Block*) arg;

   
    while(true){
    	pthread_barrier_wait(&barr1);
		for(int j = bl->first; j <= bl->last; ++j) {
			if (A[j][j] == 0.0){
				continue;
			}
			if(j != bl->last){
    			double temp = A[j+1][j]/A[j][j];
    			for (int k = j; k <= bl->last; ++k){
    				A[j+1][k]-=A[j][k]*temp;
    			}
    			for (int k = downBlock.first; k <= downBlock.last; ++k){
    			    A[j+1][k]-=A[j][k]*temp;
    			}
    			B[j+1]-=B[j]*temp;
				if(bl->i != 0){
    				double temp2 = A[downBlock.first+bl->i-1][j]/A[j][j];
    				for (int k = j; k <= bl->last; ++k){
    					A[downBlock.first+bl->i-1][k]-=A[j][k]*temp2;
    				}
    				A[downBlock.first+bl->i-1][downBlock.first+bl->i-1]-=A[j][downBlock.first+bl->i-1]*temp2;
    				B[downBlock.first+bl->i-1]-=B[j]*temp2;
    			}
			} else {
			    if (bl->i != 0 ){
			        double temp3 = A[downBlock.first+bl->i-1][j]/A[j][j];
			        A[downBlock.first+bl->i-1][j]-=A[j][j]*temp3;
			        A[downBlock.first+bl->i-1][downBlock.first+bl->i-1]-=A[j][downBlock.first+bl->i-1]*temp3;
    				B[downBlock.first+bl->i-1]-=B[j]*temp3;
			    }
			}
			if( bl->i != (countThreads-1)){
				if ( j == bl->last) {
					double temp3 = A[downBlock.first+bl->i][j]/A[j][j];
					A[downBlock.first+bl->i][j]-=A[j][j]*temp3;
					A[downBlock.first+bl->i][downBlock.first+bl->i]-=A[j][downBlock.first+bl->i]*temp3;
					B[downBlock.first+bl->i]-=B[j]*temp3;
				}
			}
		}
		pthread_barrier_wait(&barr2);
		pthread_barrier_wait(&barr1);
		for (int i = bl->last; i >= bl->first; i--){
			double temp = 0.0;
			for (int j = bl->first; j <= bl->last ; j++ ) {
				if(i != j){
					temp+=Z[j]*A[i][j];
				}
			}
			for(int k = downBlock.first; k <= downBlock.last; ++k){
			    temp+=Z[k]*A[i][k];
			}
			Z[i]=(B[i]-temp)/(A[i][i]);
		}
		pthread_barrier_wait(&barr2);
    }
}
int main(int argc, char const *argv[]) {
	
	char buffer[BUFFERSIZE];
	char buffer2[BUFFERSIZE];


	pthread_attr_t pattr;
	pthread_attr_init(&pattr);
	pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate (&pattr,PTHREAD_CREATE_JOINABLE);

	timer=DELTATIME;

	if (argc != 4) {					 // Проверка на число аргументов
		write(1,"Error\n",7);
		exit(-1);
	}

	countThreads = atoi(argv[1]);		// Количество поток (количесво блоков)
	timeLimit = atof(argv[2]);			// Время расчет
	countPoints = atoi(argv[3]);		// Количество узлов
	deltax=(LENGTH/(double)(countPoints+1));
	Q = ((1.0/(DELTATIME*DELTATIME))+((FATHSPEED*FATHSPEED)/(deltax*deltax)));
	M = (-FATHSPEED*FATHSPEED/(2.0*deltax*deltax));
	read(0,buffer,BUFFERSIZE);
	startCondition=atof(buffer);		//Начальное условие
	read(0,buffer2,BUFFERSIZE);
	func = atof(buffer2);				//Внешнее воздействие

	pthread_barrier_init(&barr1, NULL, countThreads+1);
  	pthread_barrier_init(&barr2, NULL, countThreads+1);


	A=(double**)malloc(sizeof(double*)*countPoints);	// Матрица для решения
	for (int i = 0; i < countPoints; ++i){				
		A[i]=(double*)malloc(sizeof(double)*countPoints);
		for(int j = 0; j <countPoints; ++j)
			A[i][j]=0.0;
	}
	basicA=(double**)malloc(sizeof(double*)*countPoints);	// Матрица коэффициентов
	for (int i = 0; i < countPoints; ++i){				
		basicA[i]=(double*)malloc(sizeof(double)*countPoints);
		for(int j = 0; j < countPoints; ++j)
			basicA[i][j]=0.0;
	}
	B=(double*)malloc(sizeof(double)*countPoints);			// Вектор свободных членов
	for (int i = 0; i < countPoints; ++i) {
		B[i] = 0.0;
	}

	prePreviousZ=(double*)malloc(sizeof(double)*countPoints); // Z на j-1 слое
	for (int i = 0; i < countPoints; ++i) {
		prePreviousZ[i] = startCondition;
	}

	previousZ=(double*)malloc(sizeof(double)*countPoints);		// Z на j слое
	for (int i = 0; i < countPoints; ++i) {
		previousZ[i] = startCondition;
	}

	Z=(double*)malloc(sizeof(double)*countPoints);				// Векторе решения
	for (int i = 0; i < countPoints; ++i) {
		Z[i] = 0.0;
	}

	
	arrBlocks=(Block*)malloc(sizeof(Block)*countThreads);
	int tempCountPoints = countPoints - countThreads+1;
	int tempCountInOneBlock = tempCountPoints/countThreads;
	int j = 0;
	while(1){			 													//Деление на блоки
		if(j+1 == countThreads || tempCountPoints <= tempCountInOneBlock){
			arrBlocks[j].i = j;
			arrBlocks[j].first = j*tempCountInOneBlock;
			arrBlocks[j].last = j*tempCountInOneBlock + tempCountPoints - 1; 
			arrBlocks[j].size = tempCountPoints;
			break;
		}
		arrBlocks[j].i = j;
		arrBlocks[j].first = j*tempCountInOneBlock;
		arrBlocks[j].last = j*tempCountInOneBlock + tempCountInOneBlock - 1; 
		arrBlocks[j].size = tempCountInOneBlock;
		tempCountPoints -= tempCountInOneBlock;
		j++;
	}

	downBlock.size = countThreads-1;									//Блок окаймления
	downBlock.first = arrBlocks[countThreads-1].last+1;
	downBlock.last = downBlock.first+downBlock.size-1;

	for(int i = 0; i < countThreads; ++i){								//Формирование матрицы А
		for (int j = arrBlocks[i].first; j <= arrBlocks[i].last; ++j) {
			if (j == arrBlocks[i].first){
				basicA[j][j] = Q;
				basicA[j][j+1] = M;
				if(i != 0){
					basicA[j][downBlock.first+i-1]=M;
					basicA[downBlock.first+i-1][j]=M;
				}
				continue;
			}
			if (j == arrBlocks[i].last){
				basicA[j][j] = Q;
				basicA[j][j-1] = M;
				if ( i != countThreads-1 ) {
					basicA[j][downBlock.first+i] = M;
					basicA[downBlock.first+i][j] = M;
				}
				continue;
			}
			basicA[j][j] = Q;
			basicA[j][j-1] = M;
			basicA[j][j+1] = M;
		}
	}

	
	for (int i = downBlock.first; i <= downBlock.last; ++i) {
		basicA[i][i]=Q;
	}
	for (int i = 0; i < countPoints; ++i){
		memcpy(A[i],basicA[i],sizeof(double)*countPoints);
	}
	returnB();
	B[(downBlock.last+downBlock.first)/2]+=func;
	for (int i = 0; i < countThreads; ++i) {
		if ( pthread_create (&(arrBlocks[i].tid), &pattr, mysolver, (void *) &(arrBlocks[i])) )
	  		perror("pthread_create");
	}

	struct timeval tim;  
        gettimeofday(&tim, NULL);  
        double t1=tim.tv_sec+(tim.tv_usec/1000000.0);
	pthread_barrier_wait(&barr1);

	for(; timer <= timeLimit; timer += DELTATIME){
		if (timer != DELTATIME){
			for (int i = 0; i < countPoints; ++i){
				memcpy(A[i],basicA[i],sizeof(double)*countPoints);
			}
			returnB();
		
			pthread_barrier_wait(&barr1);
		}

	    pthread_barrier_wait(&barr2);
    	//print_matrix();
		//print_B();
	    forwardGausForDownBlock();
	    backGausForDownBlock();
	    pthread_barrier_wait(&barr1);
	   	pthread_barrier_wait(&barr2);
	   
        //print_result();
		memcpy(prePreviousZ,previousZ,(countPoints)*sizeof(double));
		memcpy(previousZ,Z,(countPoints)*sizeof(double));
		for (int i = 0; i < countPoints; ++i) {
			Z[i] = 0.0;
		}

	}
	gettimeofday(&tim, NULL);  
        double t2=tim.tv_sec+(tim.tv_usec/1000000.0);
	printf("Time= %lf\n",t2-t1);
	return 0;
}le *) malloc( sizeof(double)*N );
X = (double *) malloc( sizeof(double)*N );
kluch = (int *) malloc( sizeof(int)*nt);
Teek = (int *) malloc( sizeof(int)*nt);
//инициализируем матрицу начальными значениями
InitializeMatr();
//подготавливаем структуру потока структуру потока
pthread_attr_init (&pattr);
pthread_attr_setscope (&pattr, PTHREAD_SCOPE_SYSTEM);
pthread_attr_setdetachstate (&pattr,PTHREAD_CREATE_JOINABLE);
threads = (ThreadRecord *) calloc (nt, sizeof(ThreadRecord));
pthread_barrier_init(&barr1, NULL, nt+1);
pthread_barrier_init(&barr2, NULL, nt+1);
j = N/nt;
//Создаём потоки
for (i=0; i<nt; i++) {
Teek[i]=0;
kluch[i]=0;
threads[i].num=i;
sem_init(&threads[i].sem, 0, 0);
threads[i].kol=nt;
if ( pthread_create (&(threads[i].tid), &pattr, mysolver, &threads[i]) )
perror("pthread_create");
};
pthread_barrier_wait(&barr1); //барьер начала работы потоков
time_start();
pthread_barrier_wait(&barr1); //барьер конца работы потоков
//обратный ход
for (i=N-1; i>=0; i--)
{
X[i]=B[i];
for (j=N-1;j>i;j--)
{
X[i]-=X[j]*A[i*N+j];
}
}
ms = time_stop();
printf("%f %f\n Time: %d milliseconds\n", A[(N-1)*N+N-1], B[N-1],ms);
printX();
exit (0);
}