/* Pyramid Graph configure functions */
/* Change retrace() & rebuild() code */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "pyramid.h"

/* Polytop Graph static space definition */

static XVertex vertex[NVERT];
static XFace face[(NFACE+1)];
static XEdge edge[NEDGE];

/* Configure statics */

static XPoint face3[NF3][(8+1)];
static XPoint face4[NF4][(17+1)];
 /* 3-top faces top location */
static XPoint scale;             /* scale (pixel/cell) for x & y */ 

/* Associate graph address with graph space */

int assoc(XPolyGraph* pg) {
pg->vertex = vertex;
pg->edge = edge;
pg->face = face;
return(0);
} /* assoc */

/* Check Resize window when configure event */

int resize(unsigned w, unsigned h) {
static XRectangle bak = {0, 0, 0, 0 };
if((bak.width == w) && (bak.height == h))
	return(0);      /* remove window */
bak.width = w; bak.height = h; 
return(NFACE);          /* resize window */
} /* resize */

/* Check window scale when resize */

int rescale(unsigned w, unsigned h) {
int x, y;      /* pixel for cell by x & y */
x = (w / NUNIT) / 3; y = (h / NUNIT) / 3;
if((scale.x == x) && (scale.y == y))
  return(0);    /* small resize without change scale */
scale.x = x; scale.y = y;
return(NFACE);  /* change scale */
} /* rescale */

/* Rebuild graph configuration */

int rebuild() { /* depending on graph ! */
static XPoint vconf[] = { /* vertex location in x,y cells */
{3, 3}, {3, 17}, {17, 17}, {17, 3}, {5, 9}, {5, 11}, {9, 15}, {11, 15}, {15, 11}, {15, 9}, {11, 5}, {9, 5}, {7, 9}, {7, 11}, {9, 13}, {11, 13}, {13, 11}, {13, 9}, {11, 7}, {9, 7}, {9, 9}, {9, 11}, {11, 11}, {11, 9}
}; /* vconf */
static int fconf3[NF3][(3+1)] = {    /* Vertex index */
{1, 5, 6, 1}, 
{0, 4, 11, 0}, 
{10, 9, 3, 10}, 
{7, 2, 8, 7}, 
{12, 20, 19, 12}, 
{13, 21, 14, 13}, 
{22, 15, 16, 22}, 
{18, 23, 17, 18}
}; /* fconf3 */

static int fconf4[NF4][(4+1)] = {
{0, 1, 5, 4, 0}, 
{1, 6, 7, 2, 1}, 
{2, 8, 9, 3, 2},
{0, 11, 10, 3, 0},
{5, 4, 12, 13, 5}, 
{5, 13, 14, 6, 5}, 
{4, 12, 19, 11, 4},
{11, 19, 18, 10, 11},
{10, 18, 17, 9, 10},
{17, 16, 8, 9, 17},
{7, 15, 16, 8, 7},
{6, 7, 15, 14, 6},
{12, 13, 21, 20, 12},
{21, 14, 15, 22, 21},
{23, 22, 16, 17, 23},
{19, 18, 23, 20, 19},
{21, 20, 23, 22, 21}
}; /* fconf4 */

/* static int fconf4[NF4][(4+1)] = { ... };  & etc. */
static int econf[NEDGE][2] = { /* 2 Vertex index for each edge */
{0, 1},
 {1, 2},
 {0, 3},
 {2, 3},
 {0, 4},
 {1, 5},
 {4, 5},
 {1, 6},
 {5, 6},
 {2, 7},
 {6, 7},
 {2, 8},
 {7, 8},
 {3, 9},
 {8, 9},
 {3, 10},
 {9, 10},
 {0, 11},
 {4, 11},
 {10, 11},
 {4, 12},
 {5, 13},
 {12, 13},
 {6, 14},
 {13, 14},
 {7, 15},
 {14, 15},
 {8, 16},
 {15, 16},
 {9, 17},
 {16, 17},
 {10, 18},
 {17, 18},
 {11, 19},
 {12, 19},
 {18, 19},
 {12, 20},
 {19, 20},
 {13, 21},
 {14, 21},
 {20, 21},
 {15, 22},
 {16, 22},
 {21, 22},
 {17, 23},
 {18, 23},
 {20, 23},
 {22, 23}
}; /* edge */
int i, j;                          /* vertex, edge, face index */
for(i=0; i < NVERT; i++) {    /* compute vertex pixel location */
  vertex[i].x = scale.x * vconf[i].x;
  vertex[i].y = scale.y * vconf[i].y;
} /* for-vertex */
for(i=0; i < NEDGE; i++) {   /* vertex pixel location for edge */
  edge[i].x1 = vertex[econf[i][0]].x;
  edge[i].y1 = vertex[econf[i][0]].y;
  edge[i].x2 = vertex[econf[i][1]].x;
  edge[i].y2 = vertex[econf[i][1]].y;
} /* for-edge */
for(i=0; i < NF3; i++)                /* vertex pixel location */ 
  for(j=0; j<(3+1); j++) {                  /* for 3-top faces */    
    face3[i][j].x = vertex[fconf3[i][j]].x;
    face3[i][j].y = vertex[fconf3[i][j]].y;
  } /* for 3-top face */

for(i=0; i < NF4; i++)                /* vertex pixel location */ 
  for(j=0; j<(4+1); j++) {                  /* for 4-top faces */    
    face4[i][j].x = vertex[fconf4[i][j]].x;
    face4[i][j].y = vertex[fconf4[i][j]].y;
  } /* for 3-top face */
/* for(i=0; i < NF4; i++) */
/*   for(j=0; j<(4+1); j++) { ... } & etc */
return(0);
} /* rebuild */

/* Trace face array to join all n-face array */

int retrace() { /* depending on graph ! */
int i=0;  /* total face index */
int j;    /* n-top face index */
for(j=0; j<NF3; j++, i++) {    /* fix 3-top faces in face array */
  face[i].top = face3[j];       /* fix 3-top face array address */
  face[i].Cn = 3;                /* fix 3-top face top number=3 */
  face[i].tone = DEFTONE;        /* set face default tone color */
  face[i].regi = XCreateRegion();      /* Empty region for face */
} /* face3 */

for(j = 0; j<NF4; j++, i++) {
  face[i].top = face4[j];             
  face[i].Cn = 4;                       
  face[i].tone = DEFTONE;
  face[i].regi = XCreateRegion();
/* face4 */
} 
/* for(j=0; j<NF4; j++, i++) { ... } & etc. */
face[i].tone = DEFTONE;  /* store extern face default tone color */  
return(0);
} /* retrace */

/* Reconfigure graph when window resize & rescale */

int reconf(unsigned w, unsigned h) {
if(resize(w, h) == 0)
  return(0);
if(rescale(w, h) != 0)
  rebuild();
return(NFACE);
} /* reconf */

/* Ident face by inside point to repaint */

int zotone(int x, int y) {
static XPoint past = {0, 0}; /* past scale */
int f=0;                     /* face index */
if((past.x == scale.x) && (past.y == scale.y)) /* Scale control */
  f = NFACE;      /* when no change scale */
for( ; f < NFACE; f++) {          /* New regional zone for face */
  XDestroyRegion(face[f].regi);
  face[f].regi = XPolygonRegion(face[f].top, face[f].Cn, 0);
} /* for */
past.x = scale.x; past.y = scale.y;              /* Store scale */
for(f=0; f < NFACE; f++)   /* find face with (x,y) inside */
  if(XPointInRegion(face[f].regi, x, y) == True)
    break;
face[f].tone = (face[f].tone + 1) % NCOLOR;    /* new face tone */
return(f);                   /* return pointed face for repaint */
} /* zotone */
