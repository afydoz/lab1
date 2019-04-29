#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
//#include <mpi.h>
#include "/usr/include/mpi/mpi.h"
#define ENABLE_GNUPLOT
double dx;
double dt;
#define A 0.5
#define LEFT_NODE 1
#define RIGHT_NODE 2
#define LENGTH_X 20
#define LENGTH_T 100
double dx2;
double dt2;
double a2 ;
FILE *out,*script;
struct timeval tv1,tv2,dtv;
struct timezone tz;
void writeInFile(double* Z,int lengthX, double currentTime) {
int i;
for(i = 0; i < lengthX; i++)
fprintf(out,"%d\t%lf\n",i * (int)dx, Z[i]);
fprintf(out,"\n");
fprintf(out, "\n\n");
}

void generateGnuplotScript(int timeInterval) {
out = fopen("out.txt","w");
script = fopen("script.dat","w");
fprintf(script, "set cbrange [0.9:1]\n");
fprintf(script, "set xrange [0:%d]\n", (int)LENGTH_X - (int)dx);
fprintf(script, "set yrange [-10:10]\n");
fprintf(script, "set palette defined (1 '#ce4c7d')\n");
fprintf(script, "set style line 1 lc rgb '#b90046' lt 1 lw 0.5\n");
fprintf(script, "do for [i=0:%d]{\n", (int) LENGTH_T - 1);
fprintf(script, "plot 'out.txt' index i using 1:2 \npause 0.1}\npause -1\n");
}

void timerStart() {
gettimeofday(&tv1, &tz);
}

int timerStop() {
gettimeofday(&tv2, &tz);
dtv.tv_sec = tv2.tv_sec -tv1.tv_sec;
dtv.tv_usec = tv2.tv_usec-tv1.tv_usec;
if (dtv.tv_usec < 0)
dtv.tv_sec--;
dtv.tv_usec += 1000000;
return dtv.tv_sec * 1000 + dtv.tv_usec / 1000;
}
void setInitialValues(double* Z0, int lengthX) {
int i; //  
for(i = 0; i < lengthX; i++) {
Z0[i] = 0;
}
writeInFile(Z0, lengthX, 0);
}

//  
double F(int x, double curtime) {
return (curtime <= 5 && (int)x == 3);
}

void calculate(double* Z0, int lengthX, double currentTime,int myRank, int totalProcesses, double* zleft, double* zright) {
int i;
int index;
if (myRank != 0) {
index = 0;
Z0[index] = dt2 * (a2*(*zleft-2*Z0[index]+Z0[index+1])/dx2 + F(myRank*lengthX +
index, currentTime)) + 2*Z0[index]-Z0[index];
*zleft = Z0[index];
}
for(i = 1; i < lengthX - 1; i++) { 
index = i;
Z0[index] = dt2 * (a2*(Z0[index-1]-2*Z0[index]+Z0[index+1])/dx2 +
F(myRank*lengthX + index, currentTime) ) + 2*Z0[index]-Z0[index];
}
if (myRank != totalProcesses - 1) {
index = lengthX-1;
Z0[index] = dt2 * (a2*(Z0[index-1]-2*Z0[index]+*zright)/dx2 + F(myRank*lengthX +
index, currentTime) ) + 2*Z0[index]-Z0[index];
*zright = Z0[index];
}
//   
if(myRank != totalProcesses - 1) { //     ,    
//     
MPI_Send((void*)zright, 1, MPI_DOUBLE, myRank + 1, LEFT_NODE,
MPI_COMM_WORLD);
//     ,    
 
MPI_Recv((void*)zright, 1, MPI_DOUBLE, myRank + 1, RIGHT_NODE,
MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
if(myRank != 0) { //     ,     
//     
MPI_Send((void*)zleft, 1, MPI_DOUBLE, myRank - 1,
RIGHT_NODE,MPI_COMM_WORLD);
//     ,    
 
MPI_Recv((void*)zleft, 1, MPI_DOUBLE, myRank-1, LEFT_NODE,
MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
}
int main(int argc, char **argv) {
int myRank, totalProcesses;
if (argc < 3) {
printf("./a.out lengthX timeInterval\n");
exit(0);
}
int lengthX = atoi(argv[1]);
double timeInterval = atof(argv[2]); //   
dx = (lengthX > LENGTH_X)?dx = 1.0:dx = LENGTH_X / lengthX;
dt = (timeInterval > LENGTH_T) ? dt = 1.0 : dt = timeInterval / LENGTH_T;
dx2 = dx * dx;
dt2 = dt * dt;
a2 = A * A;
double* Z0;
double currentTime = dt; //   
int znumber = 1; //    
//    MPI
MPI_Init (&argc, &argv);
MPI_Comm_size (MPI_COMM_WORLD, &totalProcesses);
MPI_Comm_rank (MPI_COMM_WORLD, &myRank);
int newlengthX = lengthX / totalProcesses; // E   
int lengthXmod = lengthX % totalProcesses;
double* z0;
double* z1;
double zleft;
double zright;
double* tmp;
if (myRank == 0) {
generateGnuplotScript(timeInterval);
Z0 = (double*) calloc (lengthX, sizeof(double)); //    
 
setInitialValues(Z0, Z0, lengthX);
}
if(myRank == totalProcesses - 1) {
newlengthX += lengthXmod;
}
z0 = (double*) calloc (newlengthX, sizeof(double));
z1 = (double*) calloc (newlengthX, sizeof(double));
//     
int* sendArr;
sendArr = (int *)calloc(totalProcesses,sizeof(int));
int i;
for(i = 0; i < totalProcesses; i++)
sendArr[i] = i * newlengthX;
int* sendArrCount;
sendArrCount = (int *)calloc(totalProcesses,sizeof(int));
for(i = 0; i < totalProcesses; i++)
sendArrCount[i] = newlengthX;
sendArrCount[totalProcesses - 1] += lengthXmod;
MPI_Scatterv((void *)(Z0), sendArrCount, sendArr, MPI_DOUBLE,(void *)(z0),
newlengthX, MPI_DOUBLE, 0, MPI_COMM_WORLD);
MPI_Scatterv((void *)(Z0), sendArrCount, sendArr, MPI_DOUBLE,(void *)(z1),
newlengthX, MPI_DOUBLE, 0, MPI_COMM_WORLD);
zleft = z0[0];
zright = z0[newlengthX-1];
if(myRank != totalProcesses - 1) { //     ,    
//     
MPI_Send((void*)&zright, 1, MPI_DOUBLE, myRank + 1, LEFT_NODE,
MPI_COMM_WORLD);
//     ,    
 
MPI_Recv((void*)&zright, 1, MPI_DOUBLE, myRank + 1, RIGHT_NODE,
MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
if(myRank!= 0) { //     ,     
//     
MPI_Send((void*)&zleft, 1, MPI_DOUBLE, myRank - 1, RIGHT_NODE,
MPI_COMM_WORLD);
//     ,    
 
MPI_Recv((void*)&zleft, 1, MPI_DOUBLE, myRank-1, LEFT_NODE,
MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
if (myRank == 0)
timerStart();
while(currentTime < timeInterval) {
if(znumber == 1)
calculate(z0, z1, newlengthX, currentTime, myRank, totalProcesses, &zleft, &zright);
else
calculate(z1, z0, newlengthX, currentTime, myRank, totalProcesses, &zleft, &zright);
#ifdef ENABLE_GNUPLOT
if(znumber == 1) {
MPI_Gatherv((void *)z1, newlengthX, MPI_DOUBLE,(void *)
(Z0),sendArrCount, sendArr, MPI_DOUBLE, 0, MPI_COMM_WORLD);
if (myRank == 0)
writeInFile(Z0, lengthX, currentTime);
}
else {
MPI_Gatherv((void *)z0, newlengthX, MPI_DOUBLE,(void *)
(Z0),sendArrCount, sendArr, MPI_DOUBLE, 0, MPI_COMM_WORLD); // 
if (myRank == 0)
writeInFile(Z0, lengthX, currentTime);
}
MPI_Barrier(MPI_COMM_WORLD);
#endif
if(!myRank) {
znumber *= -1;
currentTime += dt;
}
MPI_Bcast((void *)&znumber, 1, MPI_INT, 0, MPI_COMM_WORLD);
//        
MPI_Bcast((void *)&currentTime, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
}
if (myRank == 0) {
int ms = timerStop();
printf("Time: %d milliseconds\n", ms);
free(Z0);
}
free(z0);
free(z1);
free(sendArr);
free(sendArrCount);
MPI_Finalize();
exit(0);
}
