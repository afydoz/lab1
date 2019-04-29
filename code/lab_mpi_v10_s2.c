//mpicc var10.c -lm
//mpiexec -n 4 ./a.out 20 100
//gnuplot script.dat
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include "/usr/include/mpich/mpi.h"
#define GNUPLOT // комментирование этой строки приведет к остутсвию вывода, для замера скорости
double dx;
double dt;
#define A 0.5
#define LEFT 1
#define RIGHT 2
#define LENGTHX 20
#define LENGTHT 100
double dx2;
double dt2;
double a2 ;
FILE *out,*fp;
struct timeval tv1,tv2,dtv;
struct timezone tz;
void time_start()
{
 gettimeofday(&tv1, &tz);
}
int time_stop()
{
 gettimeofday(&tv2, &tz);
 dtv.tv_sec= tv2.tv_sec -tv1.tv_sec;
 dtv.tv_usec=tv2.tv_usec-tv1.tv_usec;
 if(dtv.tv_usec<0)
 dtv.tv_sec--; dtv.tv_usec+=1000000;
 return dtv.tv_sec*1000+dtv.tv_usec/1000;
}
void writeIntoFile(double* Z,int lengthX, double CurrentTime)
{
 int i,k;
 for(i=0;i<lengthX;i++)
 fprintf(out,"%d\t%lf\n",i*(int)dx,Z[i]);
 fprintf(out,"\n");
 fprintf(out, "\n\n");  
 }
// Генерация скрипта Gnuplot
void GenerateGnuplot(int timeInterval)
{
 out = fopen("out.txt","w");
 fp = fopen("script.dat","w"); //открытие файла для графического представления в gnuplot
 fprintf(fp,"set cbrange [0.9:1]\n");
 fprintf(fp,"set xrange [0:%d]\n", (int)LENGTHX-(int) dx);
 fprintf(fp,"set yrange [-10:10]\n");
 fprintf(fp,"set palette defined (1 '#ce4c7d')\n");
 fprintf(fp,"set style line 1 lc rgb '#b90046' lt 1 lw 0.5\n");
 fprintf(fp, "do for [i=0:%d]{\n", (int)LENGTHT-1);
 fprintf(fp,"plot 'out.txt' index i using 1:2 smooth bezier title 'powered by Artur'\npause 0.1}\npause -1\n");
}
// Задание начальных значений
void FirstCalculation(double* Z0,double* Z1,int lengthX)
{
 int i; // i номер столбца

 for(i=0;i<lengthX;i++)
 {
 Z0[i] = Z1[i] = 0; //sin(i*dx*M_PI / (LENGTHX-1));
 }
 writeIntoFile(Z0,lengthX,0);
}
// Внешняя сила
double F(int x, double curtime) {
 if (curtime <= 5 && (int)x == 3) {
 return 1;
 }
 return 0;
}
void calculate(double* Z0,double* Z1,int lengthX, double CurrentTime,int
myrank,int total,double* zleft, double* zright)
{
 // аргумент Z0 всегда текущий а Z1 предыдущий. но Z0 может быть как z0 так и z1;

 int i;
 int index;
 if ( myrank!= 0 )
 {
 index = 0;
 Z1[index] = dt2 * (a2*(*zleft-2*Z0[index]+Z0[index+1])/dx2 + F(myrank*lengthX + index, CurrentTime)) + 2*Z0[index]-Z1[index];
 *zleft = Z1[index];
 }
 for(i=1;i<lengthX-1;i++) // левый и правый столбец тоже для него не хватает данных
 {
     index = i;
 Z1[index] = dt2 * ( a2*(Z0[index-1]-2*Z0[index]+Z0[index+1])/dx2 + F(myrank*lengthX + index, CurrentTime) ) + 2*Z0[index]-Z1[index];
 }
 if ( myrank!= total-1 )
 {
 index = lengthX-1;
 Z1[index] = dt2 * ( a2*(Z0[index-1]-2*Z0[index]+*zright)/dx2 + F(myrank*lengthX + index, CurrentTime) ) + 2*Z0[index]-Z1[index];
 *zright = Z1[index];
 }

 // высылаем крайние узлы
 if( myrank!= total-1) // если это не последний процесс то нужен правый ряд
 {
 // кидаем правую часть следующему процессу
 MPI_Send((void*)zright,1,MPI_DOUBLE,myrank+1,LEFT,MPI_COMM_WORLD);
 // ловим левую часть следующего процесса, для вычисляющего процесса это будет правой
 MPI_Recv((void*)zright,1,MPI_DOUBLE,myrank+1,RIGHT,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
 }
 if( myrank!= 0) // если это не нулевой процесс то нужно получить левый ряд
 {
 // кидаем левую часть предыдущему процессу
 MPI_Send((void*)zleft,1,MPI_DOUBLE,myrank-1,RIGHT,MPI_COMM_WORLD);
 // ловим правую часть предыдущего процесса, для вычисляющего процесса это будет левой
 MPI_Recv((void*)zleft,1,MPI_DOUBLE,myrank-1,LEFT,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
 }
}
int main(int argc, char **argv)
{
 int myrank, total; // номер процесса, кол-во процессов
 if ( argc < 3)
 {
 printf("./a.out #lengthX #timeInterval\n");
 exit(0);
 }
 int lengthX = atoi(argv[1]); // сетка по x
 double timeInterval = atof(argv[2]); // временной интервал моделирования
 if(lengthX > LENGTHX)
 dx = 1.0;
 else
 dx = LENGTHX / lengthX;
 if(timeInterval > LENGTHT)
 dt = 1.0;
 else
 dt = timeInterval / LENGTHT;
 dx2 = dx*dx;
 dt2 = dt*dt;
 a2 = A*A;
 double* Z0;
 double* Z1;
 double CurrentTime = dt; // текущий момент времени
 int znumber=1; // в какой массив записывать
 // Инициализация коммуникационных средств MPI
 MPI_Init (&argc, &argv);
 MPI_Comm_size (MPI_COMM_WORLD, &total);
 MPI_Comm_rank (MPI_COMM_WORLD, &myrank);
 int newlengthX = lengthX / total; // узлов на один процесс
 int lengthXmod = lengthX % total; // остаток
 double* z0;
 double* z1;
 double zleft;
 double zright;
 double* tmp;
 if (myrank == 0)
 {
 GenerateGnuplot(timeInterval);
 Z0 = (double*) calloc (lengthX,sizeof(double)); // значение узлов в текущий момент времени
 Z1 = (double*) calloc (lengthX,sizeof(double)); // значение узлов в предыдущий момент времени
 FirstCalculation(Z0,Z1,lengthX);
 }
 if(myrank == total-1)
 {
 newlengthX += lengthXmod;
 }

 // для каждого процесса(включая нулевой) выделяем память
 z0 = (double*) calloc (newlengthX,sizeof(double));
 z1 = (double*) calloc (newlengthX,sizeof(double));
 // каждому процессу передадим необходимую часть
 int* sendArr;
 sendArr = (int *)calloc(total,sizeof(int));
 int i;
 for(i = 0; i < total; i++)
 sendArr[i] = i * newlengthX;
 int* sendArrCount;
 sendArrCount = (int *)calloc(total,sizeof(int));
 for(i = 0; i < total; i++)
 sendArrCount[i] = newlengthX;
 sendArrCount[total-1] += lengthXmod;
 MPI_Scatterv((void *)(Z0), sendArrCount, sendArr, MPI_DOUBLE,(void *)(z0), newlengthX, MPI_DOUBLE, 0, MPI_COMM_WORLD); // разбрызгиватель
 MPI_Scatterv((void *)(Z1), sendArrCount, sendArr, MPI_DOUBLE,(void *)(z1), newlengthX, MPI_DOUBLE, 0, MPI_COMM_WORLD);
 zleft = z0[0];
 zright = z0[newlengthX-1];
 if( myrank!= total-1) // если это не последний процесс то нужен правый ряд
 {
 // кидаем правую часть следующему процессу
 MPI_Send((void*)&zright,1,MPI_DOUBLE,myrank+1,LEFT,MPI_COMM_WORLD);
 // ловим левую часть следующего процесса, для вычисляющего процесса это будет правой

MPI_Recv((void*)&zright,1,MPI_DOUBLE,myrank+1,RIGHT,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
 }
 if( myrank!= 0) // если это не нулевой процесс то нужно получить левый ряд
 {
 // кидаем левую часть предыдущему процессу
 MPI_Send((void*)&zleft,1,MPI_DOUBLE,myrank-1,RIGHT,MPI_COMM_WORLD);
 // ловим правую часть предыдущего процесса, для вычисляющего процесса это будет левой
 MPI_Recv((void*)&zleft,1,MPI_DOUBLE,myrank-1,LEFT,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
 }
 if ( myrank == 0)
 time_start();
 while(CurrentTime < timeInterval)
 {
 if(znumber == 1)
 calculate(z0,z1,newlengthX,CurrentTime,myrank,total,&zleft,&zright);
 else
 calculate(z1,z0,newlengthX,CurrentTime,myrank,total,&zleft,&zright);
 #ifdef GNUPLOT
 if(znumber == 1) {
 MPI_Gatherv((void *)z1, newlengthX, MPI_DOUBLE,(void*)(Z1),sendArrCount, sendArr, MPI_DOUBLE, 0, MPI_COMM_WORLD);
 if (myrank == 0)
 writeIntoFile(Z1,lengthX,CurrentTime); // запись в gnuplot
 }
 else {
 MPI_Gatherv((void *)z0, newlengthX, MPI_DOUBLE,(void*)(Z0),sendArrCount, sendArr, MPI_DOUBLE, 0, MPI_COMM_WORLD); // совок
 if (myrank == 0)
 writeIntoFile(Z0,lengthX,CurrentTime); // запись в gnuplot
}
 MPI_Barrier(MPI_COMM_WORLD); // ждем запись
 #endif
 if(!myrank)
 {
 znumber *= -1;
 CurrentTime += dt;
 }
 MPI_Bcast((void *)&znumber, 1, MPI_INT, 0, MPI_COMM_WORLD);
// Делимся со всеми процессами значением времени на текущем шаге, нужно для того, чтобы правильно рассчитывалось внешнее воздействие на каждый узел, которое зависит от координаты и времени
 MPI_Bcast((void *)&CurrentTime, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
 }

 // Вывод времени работы
 if ( myrank == 0)
 {
 int ms = time_stop();
 printf("Time: %d milliseconds\n", ms);
 free(Z0);
 free(Z1);
 }

 // Освобождение выделенной памяти
 free(z0);
 free(z1);
 free(sendArr);
 free(sendArrCount);
 MPI_Finalize();
 exit(0);
}