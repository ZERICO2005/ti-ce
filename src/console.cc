#include <string>
#include <stdio.h>
#include "console.h"
//#include "menu_config.h"
#include "menuGUI.h"
#include "textGUI.h"
#include "file.h"
#include "main.h"
#ifndef GIAC_FAKE
namespace xcas {
  bool eqws(char * s,bool eval);
}
#endif
#ifdef TICE
#include <sys/lcd.h>
#else
inline void raw_set_pixel(unsigned x,unsigned y,int c){
  os_set_pixel(x,y,c);
}
#endif
#if defined TICE && !defined std
#define std ustl
#endif
using namespace std;

#ifdef XLIGHT
//const int lang=1;
#else
int lang=1;
#endif


//#define POPUP_PRETTY 1
#define POPUP_PRETTY_STR "Pretty print"

struct line * Line;
const int maxfmenusize=16;
char menu_f1[maxfmenusize]={0},menu_f2[maxfmenusize]={0},menu_f3[maxfmenusize]={0},menu_f4[maxfmenusize]={0},menu_f5[maxfmenusize]={0},menu_f6[maxfmenusize],menu_f7[maxfmenusize]={0},menu_f8[maxfmenusize]={0},menu_f9[maxfmenusize]={0},menu_f10[maxfmenusize]={0},menu_f11[maxfmenusize]={0},menu_f12[maxfmenusize]={0},menu_f13[maxfmenusize]={0},menu_f14[maxfmenusize]={0},menu_f15[maxfmenusize]={0},menu_f16[maxfmenusize]={0},menu_f17[maxfmenusize]={0},menu_f18[maxfmenusize]={0},menu_f19[maxfmenusize]={0},menu_f20[maxfmenusize]={0};
char session_filename[MAX_FILENAME_SIZE+1]="session";
char * FMenu_entries_name[]={menu_f1,menu_f2,menu_f3,menu_f4,menu_f5,menu_f6,menu_f7,menu_f8,menu_f9,menu_f10,menu_f11,menu_f12,menu_f13,menu_f14,menu_f15,menu_f16,menu_f17,menu_f18,menu_f19,menu_f20};
struct location Cursor;
static Char *Edit_Line;
int Start_Line, Last_Line,editline_cursor;
static int Case;
int console_changed=0; // 1 if something new in history
int dconsole_mode=1; // 0 disables dConsole commands

#define Current_Line (Start_Line + Cursor.y)
#define Current_Col (Line[Cursor.y + Start_Line].start_col + Cursor.x)

int xthetat;

  int asc_sort_int(const void * vptr,const void *wptr){
    const vector<int> * v=(const vector<int> * )vptr;
    const vector<int> * w=(const vector<int> * )wptr;
    for (size_t i=0;i<v->size();++i){
      int vi=(*v)[i];
      int wi=(*w)[i];
      if (vi!=wi)
	return vi<wi?-1:1;
    }
    return 0;
  }

  int asc_sort_int_(const vector<int> & v,const vector<int> & w){
    for (size_t i=0;i<v.size();++i){
      int vi=v[i];
      int wi=w[i];
      if (vi!=wi)
	return vi<wi?-1:1;
    }
    return 0;
  }

  int asc_sort_double(const void * vptr,const void *wptr){
    const vector<double> * v=(const vector<double> * )vptr;
    const vector<double> * w=(const vector<double> * )wptr;
    for (size_t i=0;i<v->size();++i){
      double vi=(*v)[i];
      double wi=(*w)[i];
      if (fabs(vi-wi)>1e-6*fabs(wi))
        return vi<wi?-1:1;
    }
    return 0;
  }

void raw_set_pixel(unsigned x,unsigned y,int c){
  if (x<0 || x>=LCD_WIDTH_PX || y<0 || y>=LCD_HEIGHT_PX)
    return;
  ((unsigned char *) lcd_Ram)[x+y*LCD_WIDTH_PX] = c;
}
  

  // L might be modified by closing the polygon
  void draw_filled_polygon(vector< vector<int> > &L,int xmin,int xmax,int ymin,int ymax,int color){
    color=convertcolor(color);
    int n=L.size();
    // close polygon if it is open
    if (L[n-1]!=L[0])
      L.push_back(L[0]);
    else
      n--;
    // ordered list of ymin,x,index (ordered by ascending ymin)
    vector< vector<int> > om(n,vector<int>(4)); // size==12K for n==384 
    for (int j=0;j<n;j++){
      int y0=L[j][1],y1=L[j+1][1];
      om[j][0]=y0<y1?y0:y1;
      om[j][1]=y0<y1?L[j][0]:L[j+1][0];
      om[j][2]=j;
      om[j][3]=y0<y1?j:(j==n-1?0:j+1);
    }
    qsort(&om.front(),om.size(),sizeof(vector<int>),asc_sort_int);
    //stable_sort(om.begin(),om.end(),asc_sort_int_);
    // vreverse(om.begin(),om.end());
    vector<double> p(n); // inverses of slopes
    for (int j=0;j<n;j++){
      double dx=L[j+1][0]-L[j][0];
      double dy=L[j+1][1]-L[j][1];
      p[j]=dy==0?(dx>0?
#ifdef TICE
                  1e38:-1e38
#else
                  1e300:-1e300
#endif
                  ):dx/dy;
    }
    // initialization, lowest horizontal that is crossing the polygon
    // y at ymin-1, that way lxj is initialized in the loop
    int y=om[0][0]-1,j,ompos=0;
    vector< vector<double> > lxj; // size about 12K for n==384
    // main loop
    for (;y<ymax;){
      if (y>=ymin){ // draw pixels for this horizontal frame
	size_t lxjs=lxj.size();
	qsort(&lxj.front(),lxjs,sizeof(vector<double>),asc_sort_double);
	bool odd=false;
	vector<char> impair(lxjs);
	for (size_t k=0;k<lxjs;++k){
	  int arete=lxj[k][1]; // edge L[arete]->L[arete+1]
	  int y1=L[arete][1],y2=L[arete+1][1];
	  if (y!=y1 && y!=y2)
	    odd=!odd;
	  else {
	    int ym=giacmin(y1,y2);
	    if ( y1!=y2 && (ym==y || ym==y)){
	      odd=!odd;
	    }
	  }
	  impair[k]=odd;
	}
	for (size_t k=0;k<lxjs;++k){
	  if (impair[k]){
	    int x1=giacmax(xmin,int(lxj[k][0]+.5));
	    int x2=k==lxjs-1?xmax:giacmin(xmax,int(lxj[k+1][0]+.5));
	    for (;x1<=x2;++x1)
	      raw_set_pixel(x1,y,color);
	  }
	}
      } // end if y>=ymin
      y++;
      if (y>=ymax) break;
      // update lxj
      for (j=0;j<int(lxj.size());++j){
        int k=lxj[j][1];
        if (y<=giacmax(L[k][1],L[k+1][1]))
          lxj[j][0] += p[k];
        else {
          lxj.erase(lxj.begin()+j);
          --j;
        }
      }
      // new edges
      for (j=ompos;j<n;++j){
	ompos=j;
	if (om[j][0]>y)
	  break;
	if (om[j][0]<y)
	  continue;
	vector<double> add(2,om[j][1]);
	add[1]=om[j][2];
	lxj.push_back(add);
      }
    } // end for (;y<ymax;)
  }

  void draw_polygon(vector< vector<int> > & v1,int color){
    //chk_freeze();
    if (v1.back()!=v1.front())
      v1.push_back(v1.front());
    int n=v1.size()-1;
    for (int i=0;i<n;++i){
      int x1=v1[i][0],y1=v1[i][1],x2=v1[i+1][0],y2=v1[i+1][1];
      draw_line(x1,y1,x2,y2,color);
    }
  }
  
void draw_circle(int xc,int yc,int r,int color,bool q1,bool q2,bool q3,bool q4){
  color=convertcolor(color);
  int x=0,y=r,delta=0;
  while (x<=y){
    if (q4){
      raw_set_pixel(xc+x,yc+y,color);
      raw_set_pixel(xc+y,yc+x,color);
    }
    if (q3){
      raw_set_pixel(xc-x,yc+y,color);
      raw_set_pixel(xc-y,yc+x,color);
    }
    if (q1){
      raw_set_pixel(xc+x,yc-y,color);
      raw_set_pixel(xc+y,yc-x,color);
    }
    if (q2){
      raw_set_pixel(xc-x,yc-y,color);
      raw_set_pixel(xc-y,yc-x,color);
    }
    ++x;
    if (delta<0){
      delta += 2*y+1;
      --y;
    }
    delta += 1-2*x;
  }
}

  void draw_filled_arc(int x,int y,int rx,int ry,int color,int theta1_deg,int theta2_deg,int xmin,int xmax,int ymin,int ymax,bool segment){
    //chk_freeze();
    // approximation by a filled polygon
    // points: (x,y), (x+rx*cos(theta)/2,y+ry*sin(theta)/2) theta=theta1..theta2
    while (theta2_deg<theta1_deg)
      theta2_deg+=360;
    if (theta2_deg-theta1_deg>=360){
      theta1_deg=0;
      theta2_deg=360;
    }
    int N0=theta2_deg-theta1_deg+1;
    // reduce N if rx or ry is small
    double red=double(rx)/1024*double(ry)/768;
    if (red>1) red=1;
    if (red<0.1) red=0.1;
    int N=red*N0;
    if (N<5)
      N=N0>5?5:N0;
    if (N<2)
      N=2;
    vector< vector<int> > v(segment?N+1:N+2,vector<int>(2));
    int i=0;
    if (!segment){
      v[0][0]=x;
      v[0][1]=y;
      ++i;
    }
    double theta=theta1_deg*M_PI/180;
    double thetastep=(theta2_deg-theta1_deg)*M_PI/(180*(N-1));
    for (;i<int(v.size())-1;++i){
      v[i][0]=int(x+rx*cos(theta)+.5);
      v[i][1]=int(y-ry*sin(theta)+.5); // y is inverted
      theta += thetastep;
    }
    v.back()=v.front();
    draw_filled_polygon(v,xmin,xmax,ymin,ymax,color);
  }    

// arc of ellipse, for y/x in [t1,t2] and in quadrant 1, 2, 3, 4
// y must be replaced by -y 
void draw_arc(int xc,int yc,int rx,int ry,int color,double t1, double t2,bool q1,bool q2,bool q3,bool q4){
  color=convertcolor(color);
  int x=0,y=rx,delta=0;
  double ryx=double(ry)/rx;
  // *logptr(contextptr) << "t1,t2:" << t1 << "," << t2 << ",q1234" << q1 << "," << q2 << "," << q3 << "," << q4 << endl;
  while (x<=y){
    int xeff=x*ryx,yeff=y*ryx;
    if (q4){
      if (y>=-x*t2 && y<=-x*t1) raw_set_pixel(xc+x,yc+yeff,color);
      if (x>=-y*t2 && x<=-y*t1) raw_set_pixel(xc+y,yc+xeff,color);
    }
    if (q3){
      if (y>=x*t1 && y<=x*t2) raw_set_pixel(xc-x,yc+yeff,color);
      if (x>=y*t1 && x<=y*t2) raw_set_pixel(xc-y,yc+xeff,color);
    }
    if (q1){
      if (y>=x*t1 && y<=x*t2) raw_set_pixel(xc+x,yc-yeff,color);
      if (x>=y*t1 && x<=y*t2) raw_set_pixel(xc+y,yc-xeff,color);
    }
    if (q2){
      if (y>=-x*t2 && y<=-x*t1) raw_set_pixel(xc-x,yc-yeff,color);
      if (x>=-y*t2 && x<=-y*t1) raw_set_pixel(xc-y,yc-xeff,color);
    }
    ++x;
    if (delta<0){
      delta += 2*y+1;
      --y;
    }
    delta += 1-2*x;
  }
}
  
void draw_arc(int xc,int yc,int rx,int ry,int color,double theta1, double theta2){
  //chk_freeze();
  if (theta2-theta1>=2*M_PI){
    draw_arc(xc,yc,rx,ry,color,-LARGEDOUBLE,LARGEDOUBLE,true,true,true,true);
    return;
  }
  // at most one vertical in [theta1,theta2]
  double t1=tan(theta1);
  double t2=tan(theta2);
  int n=int(floor(theta1/M_PI+.5));
  // n%2==0 -pi/2<theta1<pi/2, n%2==1 pi/2<theta1<3*pi/2
  double theta=(n+.5)*M_PI;
  // if theta1 is almost pi/2 mod pi, t1 might be wrong because of rounding
  if (fabs(theta1-(theta-M_PI))<1e-6 && t1>0) 
    t1=-LARGEDOUBLE;
  //*logptr(contextptr) << "thetas:" << theta1 << "," << theta << "," << theta2 << ", n " << n << ", t:" << t1 << "," << t2 << endl;
  if (theta2>theta){
    if (theta2>=theta+M_PI){
      if (n%2==0){ // -pi/2<theta1<pi/2<3*pi/2<theta2
        draw_arc(xc,yc,rx,ry,color,t1,LARGEDOUBLE,true,false,false,false);
        draw_arc(xc,yc,rx,ry,color,-LARGEDOUBLE,LARGEDOUBLE,false,true,true,false);	  
        draw_arc(xc,yc,rx,ry,color,-LARGEDOUBLE,t2,false,false,false,true);
      }
      else { // -3*pi/2<theta1<-pi/2<pi/2<theta2
	draw_arc(xc,yc,rx,ry,color,t1,LARGEDOUBLE,false,false,true,false);
	draw_arc(xc,yc,rx,ry,color,-LARGEDOUBLE,LARGEDOUBLE,true,false,false,true);
	draw_arc(xc,yc,rx,ry,color,-LARGEDOUBLE,t2,false,true,false,false);
      }
      return;
    }
    if (n%2==0){ // -pi/2<theta1<pi/2<theta2<3*pi/2
      draw_arc(xc,yc,rx,ry,color,t1,LARGEDOUBLE,true,false,false,false);
      draw_arc(xc,yc,rx,ry,color,-LARGEDOUBLE,t2,false,true,false,false);
    }
    else { // -3*pi/2<theta1<-pi/2<theta2<pi/2
      draw_arc(xc,yc,rx,ry,color,t1,LARGEDOUBLE,false,false,true,false);
      draw_arc(xc,yc,rx,ry,color,-LARGEDOUBLE,t2,false,false,false,true);
    }
    return;
  }
  if (n%2==0) { // -pi/2<theta1<theta2<pi/2
    draw_arc(xc,yc,rx,ry,color,t1,t2,true,false,false,true);	
  }
  else { // pi/2<theta1<theta2<3*pi/2
    draw_arc(xc,yc,rx,ry,color,t1,t2,false,true,true,false);	
  }
}
  
void draw_filled_circle(int xc,int yc,int r,int color,bool left,bool right){
  color=convertcolor(color);    
  int x=0,y=r,delta=0;
  while (x<=y){
    for (int Y=-y;Y<=y;Y++){
      if (right)
	raw_set_pixel(xc+x,yc+Y,color);
      if (left)
	raw_set_pixel(xc-x,yc+Y,color);
    }
    for (int Y=-x;Y<=x;Y++){
      if (right)
	raw_set_pixel(xc+y,yc+Y,color);
      if (left)
	raw_set_pixel(xc-y,yc+Y,color);
    }
    ++x;
    if (delta<0){
      delta += 2*y+1;
      --y;
    }
    delta += 1-2*x;
  }
}

void draw_rectangle(int x, int y, int width, int height, unsigned short color){
  //chk_freeze();
  drawRectangle(x,y,width,height,color);
}

//Uses the Bresenham line algorithm 
void draw_line(int x1, int y1, int x2, int y2, int color,unsigned short motif) {
  if ( (absint(x1) & 0xfffff000) ||
       (absint(x2) & 0xfffff000) ||
       (absint(y1) & 0xfffff000) ||
       (absint(y2) & 0xfffff000) 
       )
    return;
  color=convertcolor(color);
  int w =(color & 0x00070000) >> 16;
  ++w;
  color &= 0xffff;
  if ( (x1<0 && x2<0) || (x1>=LCD_WIDTH_PX && x2>=LCD_WIDTH_PX) || (y1<clip_ymin && y2<clip_ymin) || (y1>=LCD_HEIGHT_PX && y2>=LCD_HEIGHT_PX))
    return;
  signed char ix; 
  signed char iy; 
  
  // if x1 == x2 or y1 == y2, then it does not matter what we set here 
  int delta_x = (x2 > x1?(ix = 1, x2 - x1):(ix = -1, x1 - x2)) << 1; 
  int delta_y = (y2 > y1?(iy = 1, y2 - y1):(iy = -1, y1 - y2)) << 1;
  unsigned char * lcdptr=0;
  int doit=motif;
  if (w==1 && motif==0xffff){
#ifdef TICE
    lcdptr=((unsigned char *) lcd_Ram)+x1+(y1<<8)+(y1<<6);
#else
    lcdptr=((unsigned char *) lcd_Ram)+x1+y1*LCD_WIDTH_PX;
#endif
    *lcdptr=color;
    if (x1==x2){ // vertical segment
      if (y1>y2){
        for (--y1;y1>=y2;--y1){
          *lcdptr=color;
          lcdptr-=LCD_WIDTH_PX;
        }
      }
      else {
        for (++y1;y1<=y2;++y1){
          *lcdptr=color;
          lcdptr+=LCD_WIDTH_PX;
        }
      }
      return;
    }
    else if (y1==y2){ // horizontal segment
      unsigned char * lcdend=lcdptr+(x2-x1);
      if (x1>x2){
        for (--lcdptr;lcdptr>=lcdend;--lcdptr)
          *lcdptr=color;
      }
      else {
        for (++lcdptr;lcdptr<=lcdend;++lcdptr)
          *lcdptr=color;
      }
      return;
    }
  }
  else {
    if (doit&1)
      raw_set_pixel(x1, y1, color);
    doit >>=1;
    if (!doit) doit=motif;
  }
  if (delta_x >= delta_y) { 
    int error = delta_y - (delta_x >> 1);        // error may go below zero
    if (lcdptr){
      //dbg_printf("Bresenham lcd delta_x>delta_y\n");
      while (x1 != x2) { 
        if (error >= 0) { 
          if (error || (ix > 0)) { 
            if (iy==1) lcdptr+=LCD_WIDTH_PX; else lcdptr-=LCD_WIDTH_PX;
            y1 += iy;
            error -= delta_x; 
          }                           // else do nothing 
        }                              // else do nothing 
        x1 += ix;
        lcdptr += ix;
        error += delta_y;
        if (x1>=0 && x1<LCD_WIDTH_PX && y1>=0 && y1<LCD_HEIGHT_PX)
          *lcdptr=color;
      }
      return;
    }
    //dbg_printf("Bresenham delta_x>delta_y\n");
    while (x1 != x2) { 
      if (error >= 0) { 
        if (error || (ix > 0)) { 
          y1 += iy;
          error -= delta_x; 
        }                           // else do nothing 
      }                              // else do nothing 
      x1 += ix; 
      error += delta_y;
      int y__=y1+(w+1)/2;
      for (int y_=y1-w/2;y_<y__;++y_){
        if (doit&1)
          raw_set_pixel(x1, y_, color);
        doit >>=1;
        if (!doit) doit=motif;
      }
    }
    return;
  }
  int error = delta_x - (delta_y >> 1);      // error may go below zero 
  if (lcdptr){
    //dbg_printf("Bresenham lcd delta_y>delta_x\n");
    while (y1 != y2) { 
      if (error >= 0) { 
        if (error || (iy > 0)) { 
          x1 += ix;
          lcdptr += ix;
          error -= delta_y; 
        }                           // else do nothing 
      }                              // else do nothing 
      y1 += iy;
      if (iy==1) lcdptr+=LCD_WIDTH_PX; else lcdptr-=LCD_WIDTH_PX;
      error += delta_x;
      if (x1>=0 && x1<LCD_WIDTH_PX && y1>=0 && y1<LCD_HEIGHT_PX)
        *lcdptr=color;
    }
    return;
  }
  //dbg_printf("Bresenham delta_y>delta_x\n");
  while (y1 != y2) { 
    if (error >= 0) { 
      if (error || (iy > 0)) { 
        x1 += ix; 
        error -= delta_y; 
      }                           // else do nothing 
    }                              // else do nothing 
    y1 += iy; 
    error += delta_x;
    int x__=x1+(w+1)/2;
    for (int x_=x1-w/2;x_<x__;++x_){
      if (doit&1)
        raw_set_pixel(x_, y1, color);
      doit >>=1;
      if (!doit) doit=motif;
    }
  }
}

#ifndef TICE
static
#endif
bool isalpha(char c){
  return (c>='A' && c<='Z') || (c>='a' && c<='z');
}

bool isalphanum(char c){
  return isalpha(c) || (c>='0' && c<='9');
}


void delete_clipboard(){}

std::string * clipboard(){
  static std::string * ptr=0;
  if (!ptr)
    ptr=new std::string;
  return ptr;
}
  
void copy_clipboard(const std::string & s,bool status){
  //dbg_printf("-> clipboard %s\n",s.c_str());
  *clipboard()=s;
}

const char * paste_clipboard(){
  //dbg_printf("-> clipboard %s\n",clipboard()->c_str());
  return clipboard()->c_str();
}


#ifndef FX
void PrintMini(int x,int y,const char * s,int mode){
  x *=3;
  y *=3;
#ifdef FXCG  
  PrintMini(&x,&y,(Char *)s,mode,0xFFFFFFFF,0,0,COLOR_BLACK, COLOR_WHITE, 1, 0);
#else
  x=os_draw_string_medium(x,y,COLOR_BLACK,mode?COLOR_SELECTED:COLOR_WHITE,s,false);
#endif
}
int print_msg12(const char * msg1,const char * msg2,int textY){
  drawRectangle(0, textY+10, LCD_WIDTH_PX, 60, SDK_WHITE);
  drawRectangle(3,textY+10,300,3, SDK_BLACK);
  drawRectangle(3,textY+10,3,60, SDK_BLACK);
  drawRectangle(300,textY+10,3,60, SDK_BLACK);
  drawRectangle(3,textY+70,300,3, SDK_BLACK);
  int textX=30;
  if (msg1){
#ifdef FXCG
    PrintMini(&textX,&textY,(Char*)msg1,0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
#else
    textX=os_draw_string_medium(textX,textY,COLOR_BLACK,COLOR_WHITE,msg1,false);
#endif
  }
  textX=10;
  textY+=25;
  if (msg2){
#ifdef FXCG
    PrintMini(&textX,&textY,(Char*)msg2,0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
#else
    textX=os_draw_string_medium(textX,textY,COLOR_BLACK,COLOR_WHITE,msg2,false);
#endif
  }
  return textX;
}
#endif

void insert(string & s,int pos,const char * add){
  if (pos>int(s.size()))
    pos=s.size();
  if (pos<0)
    pos=0;
  s=s.substr(0,pos)+add+s.substr(pos,s.size()-pos);
}

void print_alpha_shift(int keyflag){
#ifdef FX
  int x=85;
  int y=58;
#else
  int x=85*3;
  int y=58*3;
#endif
  // if (keyflag==0) Printmini(x,y,"| A<>a",MINI_REV);
  if (keyflag==1){
#ifdef TICE
    Printmini(x,y," 2nd ",0);
  #else
    Printmini(x,y,"SHIFT",0);
#endif
  }
  if (keyflag==4)
    Printmini(x,y,"ALPHA",0);
  if (keyflag==8)
    Printmini(x,y,"alpha",0);
  if (keyflag==0x84)
    Printmini(x,y,"ALOCK",0);
  if (keyflag==0x88)
    Printmini(x,y,"alock",0);
}    

  void printCentered(const char* text, int y) {
    int len = strlen(text);
    int x = LCD_WIDTH_PX/2-(len*6)/2;
#ifndef FX
    x/=3;
#endif
    Printxy(x,y,text,0);
  }

#ifndef TICE
  string printint(int i){
    if (!i)
      return string("0");
    if (i<0)
      return string("-")+printint(-i);      
    int length = (int) floor(log10((double) i));
#if defined VISUALC || defined BESTA_OS
    char * s =new char[length+2];
#else
    char s[length+2];
#endif
    s[length+1]=0;
    for (;length>-1;--length,i/=10)
      s[length]=i%10+'0';
#if defined VISUALC || defined BESTA_OS
     string res=s;
     delete [] s;
     return res;
#else
    return s;
#endif
  }
#else // TICE
  string printint(int i) {
    char c[16]; // impossible for output to be longer than this
    _ti_sprintf(c, "%d", i);
    return c;
  }
#endif

int giacmax(int a,int b){
  return a<b?b:a;
}

int giacmin(int a,int b){
  return a<b?a:b;
}

int inputline(const char * msg1,const char * msg2,std::string & s,bool numeric,int ypos){
  // s="";
  int pos=s.size(),beg=0;
  for (;;){
#ifdef FX
    int X1=print_msg12(msg1,msg2,ypos-19);
    int textX=X1,textY=ypos;
    drawRectangle(textX,textY,LCD_WIDTH_PX-textX-8,8,_WHITE);
    if (pos-beg>36)
      beg=pos-12;
    if (int(s.size())-beg<36)
      beg=giacmax(0,int(s.size())-36);
    if (beg>pos)
      beg=pos;
    textX=X1;
    Printxy(textX,textY,s.substr(beg,pos-beg).c_str(),0);
    textX+=(pos-beg)*6;
    int cursorpos=textX;
    Printxy(textX+2,textY,s.substr(pos,s.size()-pos).c_str(),0);
    //Cursor_SetPosition(cursorpos,textY+1);
    drawRectangle(cursorpos,textY+1,1,6,_BLACK); // cursor
    int keyflag = (Char)Setup_GetEntry(0x14);
    Printmini(0,58,"     |     |     |     |     |     ",MINI_REV);
    print_alpha_shift(keyflag);
    unsigned int key;
    GetKey(&key);
#else // FX
    int X1=print_msg12(msg1,msg2,ypos-25);
    int textX=X1,textY=ypos;
    drawRectangle(textX,textY+24,LCD_WIDTH_PX-textX-4,18,COLOR_WHITE);
    if (pos-beg>36)
      beg=pos-12;
    if (int(s.size())-beg<36)
      beg=giacmax(0,int(s.size())-36);
    if (beg>pos)
      beg=pos;
    textX=X1;
    int cursorpos=os_draw_string_medium(textX,textY,SDK_BLACK,SDK_WHITE,s.substr(beg,pos-beg).c_str(),false); // PrintMini(&textX,&textY,(Char *)s.substr(beg,pos-beg).c_str(),0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); int cursorpos=textX;
    os_draw_string_medium(cursorpos,textY,SDK_BLACK,SDK_WHITE,s.substr(pos,s.size()-pos).c_str(),false);// PrintMini(&textX,&textY,(Char*)s.substr(pos,s.size()-pos).c_str(),0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    drawRectangle(cursorpos,textY+16,3,18,COLOR_BLACK); // cursor
    Printmini(0,C58,"         |        |        |        |  A<>a  ",4);
    int keyflag = GetSetupSetting( (unsigned int)0x14);
    int key;
    ck_getkey(&key);
#endif
    if (key==KEY_CTRL_F3){
      key=chartab();
      if (key<0)
	continue;
    }    
    //if (key==KEY_CTRL_F5){ handle_f5(); continue;    }
    if (key==KEY_CTRL_EXE){
      reset_kbd();
      return key;
    }
    if (key>=32 && key<128){
      if (!numeric || key=='-' || key=='.' || key=='e' || key=='E' || (key>='0' && key<='9')){
	s.insert(s.begin()+pos,char(key));
	++pos;
      }
      continue;
    }
    if (key==KEY_CTRL_DEL){
      if (pos){
	s.erase(s.begin()+pos-1);
	--pos;
      }
      continue;
    }
    if (key==KEY_CTRL_AC){
      if (s=="")
	return KEY_CTRL_EXIT;
      s="";
      pos=0;
      continue;
    }
    if (key==KEY_CTRL_EXIT)
      return key;
    if (key==KEY_CTRL_RIGHT){
      if (pos<s.size())
	++pos;
      continue;
    }
    // if (key==KEY_SHIFT_RIGHT){ pos=s.size(); continue; }
    if (key==KEY_CTRL_LEFT){
      if (pos)
	--pos;
      continue;
    }
    //if (key==KEY_SHIFT_LEFT){ pos=0;continue;}
    if (const char * ans=keytostring(key,keyflag,false)){
      insert(s,pos,ans);
      pos+=strlen(ans);
      continue;
    }
  }
}

void cleanup(std::string & s){
  for (size_t i=0;i<s.size();++i){
    if (s[i]=='\n')
      s[i]=' ';
  }
}

int confirm(const char * msg1,const char * msg2,bool acexit){
  print_msg12(msg1,msg2);
#ifdef FX
  Printmini(0,C58," F1 |     |     |     |     | F6  ",MINI_REV);
#else
  Printmini(0,C58,"    F1      |            |            |           |     F5    ",4);
#endif  
  int key=0;
  while (key!=KEY_CTRL_F1 && key!=KEY_CTRL_F5){
    ck_getkey(&key);
    if (key==KEY_CTRL_EXE)
      key=KEY_CTRL_F1;
    if (key==KEY_CTRL_AC || key==KEY_CTRL_EXIT){
      if (acexit) return -1;
      key=KEY_CTRL_F5;
    }
    if (key>=KEY_CTRL_F7 && key<=KEY_CTRL_F12)
      key -= 906;
    if (key>=KEY_CTRL_F13 && key<=KEY_CTRL_F18)
      key -= 912;
    // set_xcas_status();
  }
  return key;
}  

bool confirm_overwrite(){
  return do_confirm(lang?"Vraiment effacer?":"Really clear?");
}

void invalid_varname(){
  confirm(lang?"Nom de variable incorrect":"Invalid variable name", lang?"F1 ou F5: ok":"F1 or F5: ok");
}



int run_session(int start=0){
  std::vector<std::string> v;
  for (int i=start;i<Last_Line;++i){
    if (Line[i].type==LINE_TYPE_INPUT)
      v.push_back((const char *)Line[i].str);
    free(Line[i].str);
    Line[i].str=0;
    Line[i].readonly = 0;
    Line[i].type = LINE_TYPE_INPUT;
    Line[i].start_col = 0;
    Line[i].disp_len = 0;
  }
    Line[Last_Line].str=0;
    Last_Line=start;
    if (start<Start_Line)
      Start_Line=start;
    int savestartline=Start_Line;
    Start_Line=Last_Line>LINE_DISP_MAX?Last_Line-LINE_DISP_MAX:0;
    Cursor.x=0;
    Cursor.y=start-Start_Line;
    Line[start].str=Edit_Line;
    Edit_Line[0]=0;
  if (v.empty()) return 0;
  //Console_Init();
  for (int i=0;i<v.size();++i){
    Console_Output((const Char *)v[i].c_str());
    //int j=Last_Line;
    Console_NewLine(LINE_TYPE_INPUT, 1);
    Console_Disp(1);
    sync_screen();
    // Line[j].type=LINE_TYPE_INPUT;
    run(v[i].c_str(),6); /* show logo and graph but not eqw */
    // j=Last_Line;
    Console_NewLine(LINE_TYPE_OUTPUT, 1);    
    // Line[j].type=LINE_TYPE_OUTPUT;
  }
  int cl=Current_Line;
#ifdef FX
#define c8 6
#else
#define c8 8
#endif
  Cursor.y += (Start_Line-savestartline);
  if (Cursor.y<0) Cursor.y=0;
  Start_Line=savestartline;
  if (Current_Line>cl || Cursor.y>c8){
    if (cl>c8){
      Start_Line=cl-c8;
      Cursor.y=c8;
    }
    else {
      Start_Line=0;
      Cursor.y=cl;
    }
  }
  Console_Disp(1);
  sync_screen();
  return 0;
}

void menu_setup(){
  //drawRectangle(0, 18, LCD_WIDTH_PX, LCD_HEIGHT_PX-18, SDK_WHITE);
  Menu smallmenu;
  smallmenu.numitems=5;
  MenuItem smallmenuitems[smallmenu.numitems];
  smallmenu.items=smallmenuitems;
  smallmenu.height=5;
  smallmenu.width=20;
  smallmenu.startY=3;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  smallmenu.title = (char*)"Config";
  //smallmenuitems[0].type = MENUITEM_CHECKBOX;
  smallmenuitems[0].text = (char*)(lang?"Mise a l'heure":"Set time");
  smallmenuitems[1].text = (char*)(lang?"Effacer variables":"Clear variables");
  smallmenuitems[2].text = (char *) (lang?"Raccourcis":"Shortcuts");
  smallmenuitems[3].text = (char*) (lang?"A propos":"About");
  smallmenuitems[4].text = (char*) "Quit";
  while(1) {
    int sres = doMenu(&smallmenu);
    if (sres==MENU_RETURN_EXIT)
      break;
    if (sres == MENU_RETURN_SELECTION) {
      if (smallmenu.selection==1){
	int h,m; get_time(&h,&m);
	double d=h+m/100.;
	char buf[64]="Format HH.mm ";
	sprint_double(buf+strlen(buf),d);
	if (inputdouble(buf,d) && d>=0 && d<23.59){
	  h=floor(d);
	  m=100*(d-h);
	  set_time(h,m);
	  //break;
	}
      }
      if (smallmenu.selection==2 && confirm_overwrite()){
        run("restart");
        //break;
      }
      if (smallmenu.selection==5)
	break;
      if (smallmenu.selection>=3) {
	textArea text;
	text.editable=false;
	text.clipline=-1;
	text.title = smallmenuitems[smallmenu.selection-1].text;
	add(&text,smallmenu.selection==3?shortcuts_string:apropos_string);
        text.minimini=false;
	doTextArea(&text);
	continue;
      } 
    }	
  }      
}


/*

  The following functions will be used to specify the location before deleting a string of n characters altogether. Among them, a wide character (2 bytes) will be counted as a character.
	
  For example, we have the following string str:
	
  Location  |  0  |  1  |  2  |  3  |  4  |  5  | 6 |
  Character | 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 0 |

  After the call Console_DelStr (str, 3, 2), position 1 and 2 characters will be deleted, then the characters will be in advance.
	
  Results are as follows:

  Location  |  0  |  1  |  2  |  3  | 4 |  5  | 6 |
  Character | 'a' | 'd' | 'e' | 'f' | 0 | 'f' | 0 |

  (Note: the extra positions will not be filled with '\ 0', but '\ 0' will be a copy of the original end of the string.)

*/

int Console_DelStr(Char *str, int end_pos, int n)
{
  int str_len, actual_end_pos, start_pos, actual_start_pos, del_len, i;

  str_len = strlen((const char *)str);
  if ((start_pos = end_pos - n) < 0) return CONSOLE_ARG_ERR;

  if ((actual_end_pos = Console_GetActualPos(str, end_pos)) == CONSOLE_ARG_ERR) return CONSOLE_ARG_ERR;
  if ((actual_start_pos = Console_GetActualPos(str, start_pos)) == CONSOLE_ARG_ERR) return CONSOLE_ARG_ERR;

  del_len = actual_end_pos - actual_start_pos;

  for (i = actual_start_pos; i < str_len; i++)
    {
      str[i] = str[i + del_len];
    }

  return CONSOLE_SUCCEEDED;
}

/*
  ÒÔÏÂº¯ÊýÓÃÓÚÔÚÖ¸¶¨Î»ÖÃ²åÈëÖ¸¶¨µÄ×Ö·û´®¡£
	
  £¨×¢Òâ£ºÕâÀïµÄÎ»ÖÃÖ¸µÄÊÇ´òÓ¡Ê±µÄÎ»ÖÃ£¬¶ø²»ÊÇÊµ¼ÊµÄÎ»ÖÃ¡££©

  The following functions are used to specify the location of the insertion in the specified string.
  (Note: This refers to the position of the printing position when, rather than the actual position.)
*/

int Console_InsStr(Char *dest, const Char *src, int disp_pos)
{
  int i, ins_len, str_len, actual_pos;

  ins_len = strlen((const char *)src);
  str_len = strlen((const char *)dest);

  actual_pos = Console_GetActualPos(dest, disp_pos);

  if (ins_len + str_len >= EDIT_LINE_MAX) return CONSOLE_MEM_ERR;
  if (actual_pos > str_len) return CONSOLE_ARG_ERR;

  for (i = str_len; i >= actual_pos; i--)
    {
      dest[i + ins_len] = dest[i];
    }

  for (i = 0; i < ins_len; i++)
    {
      Char c=src[i];
      if (c=='\n') c=' ';
      dest[actual_pos + i] = (c==0x0a?' ':c);
    }

  return CONSOLE_SUCCEEDED;
}

/*
  ÒÔÏÂº¯ÊýÓÃÓÚÈ·¶¨¶ÔÓ¦ÓÚ×Ö·û´®´òÓ¡Î»ÖÃµÄÕæÊµÎ»ÖÃ¡£
	
  ÀýÈç£¬ÔÚÒÔÏÂÕâÒ»°üº¬¿í×Ö·ûµÄ×Ö·û´®strÖÐ£¬´òÓ¡Ê±µÄÎ»ÖÃÈçÏÂ£º
	
  Î»ÖÃ | 00 | 01 | 02 | 03 | 04 | 05 | 06 |
  ×Ö·û | Ò» | ¶þ | Èý | ËÄ | Îå | Áù | \0 |

  ¶øÔÚÊµ¼Ê´æ´¢Ê±µÄÎ»ÖÃÈçÏÂ£º

  Î»ÖÃ |  00  |  01  |  02  |  03  |  04  |  05  |  06  |  07  |  08  |  09  |  10  |  11  |
  Öµ   | 0xD2 | 0xBB | 0xB6 | 0xFE | 0xC8 | 0xFD | 0xCB | 0xC4 | 0xCE | 0xE5 | 0xC1 | 0xF9 |

  ¿ÉÒÔ·¢ÏÖ£¬µÚ4¸ö×Ö·û¡®Îå¡¯Êµ¼ÊÉÏ´æ´¢ÓÚµÚ8µÄÎ»ÖÃ¡£
  Òò´Ë£¬µ±µ÷ÓÃConsole_GetActualPos(str, 4)Ê±£¬½«·µ»Ø8¡£


  The following function is used to determine the true position of the string corresponding to the printing position.
  For example, in the following this string str contains wide characters, the location of the print is as follows:

  Location  | 00  |  01 |   02  |  03  | 04   | 05  | 06  |
  Character | one | two | three | four | five | six | \ 0 |

  The actual storage location is as follows:

  Location | 	00  | 01   |  02  |  03  |  04  |  05  |  06  |  07  |  08  |  09  |  10  |  11  |
  Value 	 | 0xD2 | 0xBB | 0xB6 | 0xFE | 0xC8 | 0xFD | 0xCB | 0xC4 | 0xCE | 0xE5 | 0xC1 | 0xF9 |

  You can find the first four characters 'five' is actually stored in the eighth position.
  So, when you call Console_GetActualPos (str, 4), it will return 8.
*/

int Console_GetActualPos(const Char *str, int disp_pos)
{
  int actual_pos, count;

  for (actual_pos = count = 0; count < disp_pos; count++)
    {
      if (str[actual_pos] == '\0') return CONSOLE_ARG_ERR;

      if (is_wchar(str[actual_pos]))
	{
	  actual_pos += 2;
	}
      else
	{
	  actual_pos++;
	}
    }

  return actual_pos;
}

/*
  ÒÔÏÂº¯ÊýÓÃÓÚ»ñÈ¡×Ö·û´®µÄ´òÓ¡³¤¶È£¬¼´£¬1¸ö¿í×Ö·û£¨Õ¼ÓÃ2×Ö½Ú£©¼Ç×÷1×Ö·û¡£
  The following functions are used to obtain a string of print length, ie, a wide character (2 bytes) recorded as a character.
*/

int Console_GetDispLen(const Char *str)
{
  int i, len;

  for (i = len = 0; str[i]!='\0'; len++)
    {
      if (is_wchar(str[i]))
	{
	  i += 2;
	}
      else
	{
	  i++;
	}
    }

  return len;
}

/*
  The following functions are used to move the cursor.
*/
int Console_MoveCursor(int direction){
  switch (direction){
    case CURSOR_UP:
      if (Current_Line==Last_Line)
	editline_cursor=Cursor.x;
      //If you need to operate.
      if ( Cursor.y>0 || Start_Line>0) {
	  //If the current line is not read-only, then Edit_Line copy to the current line.
	  if (!Line[Current_Line].readonly){
            if ((Line[Current_Line].str = (Char *)malloc(strlen((const char *)Edit_Line) + 1)) == NULL) return CONSOLE_MEM_ERR;
            strcpy((char *)Line[Current_Line].str, (const char *)Edit_Line);
            Line[Current_Line].disp_len = Console_GetDispLen(Line[Current_Line].str);
            Line[Current_Line].type = LINE_TYPE_INPUT;
          }
          
	  //If the cursor does not move to the top of, directly move the cursor upward.
	  if (Cursor.y > 0){
            Cursor.y--;
          }
	  //Otherwise, the number of rows, if the screen's first line is not the first line, then began to show minus one.
	  else if (Start_Line > 0){
            Start_Line--;
          }
	  //End if the horizontal position after moving the cursor over the line, then move the cursor to the end of the line.
	  if (Cursor.x > Line[Current_Line].disp_len){
            Cursor.x = Line[Current_Line].disp_len;
          }
	  else if (Line[Current_Line].disp_len - Line[Current_Line].start_col > COL_DISP_MAX){
            if (Cursor.x == COL_DISP_MAX) Cursor.x = COL_DISP_MAX - 1;
          }
	  //If you move the cursor to the line after the first, and the front of the line there is a character does not appear, then move the cursor to position 1.
	  if (Cursor.x == 0 && Line[Current_Line].start_col > 0) Cursor.x = 1;
	  //If the current cursor line is not read-only, then it is a string copy to Edit_Line for editing.
	  if (!Line[Current_Line].readonly&& Line[Current_Line].str){
            strcpy((char *)Edit_Line, (const char *)Line[Current_Line].str);
            free(Line[Current_Line].str);
            Line[Current_Line].str = Edit_Line;
          }
      }
      break;
    case CURSOR_ALPHA_UP:{
      int pos1=Start_Line+Cursor.y;
      Console_MoveCursor(CURSOR_UP);
      int pos2=Start_Line+Cursor.y;
      if (pos1<Last_Line && pos2<Last_Line && pos1!=pos2){
	line curline=Line[pos1];
	Line[pos1]=Line[pos2];
	Line[pos2]=curline;
      }
      break;
    }
    case CURSOR_ALPHA_DOWN: {
      int pos1=Start_Line+Cursor.y;
      Console_MoveCursor(CURSOR_DOWN);
      int pos2=Start_Line+Cursor.y;
      if (pos1<Last_Line && pos2<Last_Line && pos1!=pos2){
	line curline=Line[pos1];
	Line[pos1]=Line[pos2];
	Line[pos2]=curline;
      }
      break;
    }
    case CURSOR_DOWN:
      if (Current_Line==Last_Line)
	editline_cursor=Cursor.x;
      //If you need to operate.
      if ((Cursor.y < LINE_DISP_MAX - 1) && (Current_Line < Last_Line) || (Start_Line + LINE_DISP_MAX - 1 < Last_Line))
	{
	  //If the current line is not read-only, then Edit_Line copy to the current line.
	  if (!Line[Current_Line].readonly)
	    {
	      if ((Line[Current_Line].str = (Char *)malloc(strlen((const char *)Edit_Line) + 1)) == NULL) return CONSOLE_MEM_ERR;
	      strcpy((char *)Line[Current_Line].str, (const char *)Edit_Line);
	      Line[Current_Line].disp_len = Console_GetDispLen(Line[Current_Line].str);
	      Line[Current_Line].type = LINE_TYPE_INPUT;
	    }

	  //Èç¹û¹â±êÎ´ÒÆµ½×îÏÂ·½,ÔòÖ±½Ó½«¹â±êÏòÏÂÒÆ¡£
	  //If the cursor does not move to the bottom, the cursor moves down directly.
	  if (Cursor.y < LINE_DISP_MAX - 1 && Current_Line < Last_Line)
	    {
	      Cursor.y++;
	    }
	  //·ñÔò£¬Èç¹ûÆÁÄ»ÉÏÄ©ÐÐ²»ÊÇ×îºóÒ»ÐÐ£¬Ôò½«¿ªÊ¼ÏÔÊ¾µÄÐÐÊý¼ÓÒ»¡£
	  //The number of rows Otherwise, if the last line is not the last line on the screen, it will begin to show a plus.
	  else if (Start_Line + LINE_DISP_MAX - 1 < Last_Line)
	    {
	      Start_Line++;
	    }

	  //Èç¹ûÒÆ¶¯ºó¹â±êË®Æ½Î»ÖÃ³¬¹ýÐÐÄ©£¬Ôò½«¹â±êÒÆÖÁÐÐÄ©¡£
	  //If you move the cursor after the end of the horizontal position over the line, then move the cursor to the end of the line.
	  if (Cursor.x > Line[Current_Line].disp_len)
	    {
	      Cursor.x = Line[Current_Line].disp_len;
	    }
	  else if (Line[Current_Line].disp_len - Line[Current_Line].start_col >= COL_DISP_MAX)
	    {
	      if (Cursor.x == COL_DISP_MAX) Cursor.x = COL_DISP_MAX - 1;
	    }

	  //Èç¹ûÒÆ¶¯ºó¹â±êÔÚÐÐÊ×£¬ÇÒ¸ÃÐÐÇ°ÃæÓÐ×Ö·ûÎ´ÏÔÊ¾£¬Ôò½«¹â±êÒÆÖÁÎ»ÖÃ1¡£
	  //If you move the cursor to the line after the first, and the front of the line there is a character does not appear, then move the cursor to position 1.
	  if (Cursor.x == 0 && Line[Current_Line].start_col > 0) Cursor.x = 1;

	  //Èç¹ûÏÖÔÚ¹â±êËùÔÚÐÐ²»ÊÇÖ»¶ÁµÄ£¬Ôò½«Æä×Ö·û´®¿½±´¸øEdit_LineÒÔ¹©±à¼­¡£
	  //If the current cursor line is not read-only, then it is a string copy to Edit_Line for editing.
	  if (!Line[Current_Line].readonly&& Line[Current_Line].str)
	    {
	      strcpy((char *)Edit_Line, (const char *)Line[Current_Line].str);
	      free(Line[Current_Line].str);
	      Line[Current_Line].str = Edit_Line;
	    }
	}
      break;
    case CURSOR_LEFT:
      if (Line[Current_Line].readonly){
	if (Line[Current_Line].start_col > 0){
	  Line[Current_Line].start_col--;
	}
	break;
      }
      else {
	if (Line[Current_Line].start_col > 0){
	  if (Cursor.x > 1)
	    Cursor.x--;
	  else
	    Line[Current_Line].start_col--;
	  break;
	}
	if (Cursor.x > 0){
	  Cursor.x--;
	  break;
	}
      }
    case CURSOR_SHIFT_RIGHT:
      if (!Line[Current_Line].readonly)
	Cursor.x=min(Line[Current_Line].disp_len,COL_DISP_MAX);
      if (Line[Current_Line].disp_len > COL_DISP_MAX)
	Line[Current_Line].start_col = Line[Current_Line].disp_len - COL_DISP_MAX;
      break;
    case CURSOR_RIGHT:
      if (Line[Current_Line].readonly){
	if (Line[Current_Line].disp_len - Line[Current_Line].start_col > COL_DISP_MAX){
	  Line[Current_Line].start_col++;
	}
	break;
      }
      else {
	if (Line[Current_Line].disp_len - Line[Current_Line].start_col > COL_DISP_MAX){
	  if (Cursor.x < COL_DISP_MAX - 1)
	    Cursor.x++;
	  else
	    Line[Current_Line].start_col++;
	  break;	  
	}
	if (Cursor.x < Line[Current_Line].disp_len - Line[Current_Line].start_col){
	  Cursor.x++;
	  break;
	}
      }
    case CURSOR_SHIFT_LEFT:
      if (!Line[Current_Line].readonly)
	Cursor.x=0;
      Line[Current_Line].start_col=0;
      break;
    default:
      return CONSOLE_ARG_ERR;
      break;
    }
  return CONSOLE_SUCCEEDED;
}

/*
  ÒÔÏÂº¯ÊýÓÃÓÚÊäÈë¡£
  ×Ö·û´®½«ÊäÈëµ½¹â±ê´¦£¬¹â±ê½«×Ô¶¯ÒÆ¶¯¡£
	
  The following function is used for input.
  String input to the cursor, the cursor will automatically move.
*/

int Console_Input(const Char *str)
{
  console_changed=1;
  int old_len,i,return_val;

  if (!Line[Current_Line].readonly)
    {
      old_len = Line[Current_Line].disp_len;
      return_val = Console_InsStr(Edit_Line, str, Current_Col);
      if (return_val != CONSOLE_SUCCEEDED) return return_val;
      if ((Line[Current_Line].disp_len = Console_GetDispLen(Edit_Line)) == CONSOLE_ARG_ERR) return CONSOLE_ARG_ERR;
      for (i = 0; i < Line[Current_Line].disp_len - old_len; i++)
	{
	  Console_MoveCursor(CURSOR_RIGHT);
	}
      return CONSOLE_SUCCEEDED;
    }
  else
    {
      return CONSOLE_ARG_ERR;
    }
}

/*
  ÒÔÏÂº¯ÊýÓÃÓÚÊä³ö×Ö·û´®µ½µ±Ç°ÐÐ¡£
  The following functions are used to output the string to the current line.
*/

int Console_Output(const Char *str)
{
  console_changed=1;
  int return_val, old_len, i;

  if (!Line[Current_Line].readonly)
    {
      old_len = Line[Current_Line].disp_len;

      return_val = Console_InsStr(Edit_Line, str, Current_Col);
      if (return_val != CONSOLE_SUCCEEDED) return return_val;
      if ((Line[Current_Line].disp_len = Console_GetDispLen(Edit_Line)) == CONSOLE_ARG_ERR) return CONSOLE_ARG_ERR;
      Line[Current_Line].type = LINE_TYPE_OUTPUT;

      for (i = 0; i < Line[Current_Line].disp_len - old_len; i++)
	{
	  Console_MoveCursor(CURSOR_RIGHT);
	}
      return CONSOLE_SUCCEEDED;
    }
  else
    {
      return CONSOLE_ARG_ERR;
    }
}

void dConsolePut(const char * S){
  if (!dconsole_mode)
    return;
  int l=strlen(S);
  char *s=(char *) malloc(l+1);
  strcpy(s,S);
  for (int i=0;i<l-1;++i){
    if (s[i]=='\n' ||
        s[i]==10)
      s[i]=' ';
  }
  Console_Output((const Char *)s);
  free(s);
  if (l && S[l-1]=='\n')
    Console_NewLine(LINE_TYPE_OUTPUT, 1);
}

#define PUTCHAR_LEN 35
static char putchar_buf[PUTCHAR_LEN+2];
static int putchar_pos=0;
void dConsolePutChar(const char ch){
  if (!dconsole_mode)
    return;
  if (putchar_pos==PUTCHAR_LEN)
    dConsolePutChar('\n');
  if (ch=='\n'){
    putchar_buf[putchar_pos]='\n';
    putchar_buf[putchar_pos+1]=0;
    putchar_pos=0;
    dConsolePut(putchar_buf);
  }
  else {
    putchar_buf[putchar_pos]=ch;
    ++putchar_pos;
  }
}

/*
  Clear the current output line
*/

void Console_Clear_EditLine()
{
  if(!Line[Current_Line].readonly) {
    Edit_Line[0] = '\0';
    Line[Current_Line].start_col = 0;
    Line[Current_Line].disp_len = 0;
    Cursor.x = 0;
  }
}

/*
  ÒÔÏÂº¯ÊýÓÃÓÚ´´½¨ÐÂÐÐ¡£
  ²ÎÊýpre_line_typeÓÃÓÚÖ¸¶¨ÉÏÒ»ÐÐµÄÀàÐÍ£¬²ÎÊýpre_line_readonlyÓÃÓÚÖ¸¶¨ÉÏÒ»ÐÐÊÇ·ñÖ»¶Á¡£
  ²ÎÊýnew_line_typeÓÃÓÚÖ¸¶¨ÏÂÒ»ÐÐµÄÀàÐÍ£¬²ÎÊýnew_line_readonlyÓÃÓÚÖ¸¶¨ÏÂÒ»ÐÐÊÇ·ñÖ»¶Á¡£

  The following functions are used to create a new line.
  Pre_line_type type parameter is used to specify the line, pre_line_readonly parameter is used to specify the line is read-only.
  New_line_type parameter is used to specify the type of the next line, new_line_readonly parameter is used to specify the next line is read-only.
*/

int Console_NewLine(int pre_line_type, int pre_line_readonly)
{
  console_changed=1;
  int i;

  if (strlen((const char *)Edit_Line)||Line[Current_Line].type==LINE_TYPE_OUTPUT)
    {
      //Èç¹ûÒÑ¾­ÊÇËùÄÜ´æ´¢µÄ×îºóÒ»ÐÐ£¬ÔòÉ¾³ýµÚÒ»ÐÐ¡£
      //If this is the last line we can store, delete the first line.
      if (Last_Line == LINE_MAX - 1)
	{
	  for (i = 0; i < Last_Line; i++)
	    {
	      Line[i].disp_len = Line[i + 1].disp_len;
	      Line[i].readonly = Line[i + 1].readonly;
	      Line[i].start_col = Line[i + 1].start_col;
	      Line[i].str = Line[i + 1].str;
	      Line[i].type = Line[i + 1].type;
	    }
	  Last_Line--;

	  if (Start_Line > 0) Start_Line--;
	}

      if (Line[Last_Line].type == LINE_TYPE_OUTPUT && strlen((const char *)Edit_Line) == 0) Console_Output((const Char *)"Done");

#ifdef TEX
      if (TeX_isTeX((char*)Edit_Line)) Line[Last_Line].tex_flag = 1;
      if (Line[Last_Line].type == LINE_TYPE_OUTPUT && Line[Last_Line].tex_flag) TeX_sizeComplex ((char*)Edit_Line, &(Line[Last_Line].tex_width), &(Line[Last_Line].tex_height), NULL);
      else
	Line[Last_Line].tex_flag = 0;
#endif
		
      //½«Edit_LineµÄÄÚÈÝ¿½±´¸ø×îºóÒ»ÐÐ¡£
      //Edit_Line copy the contents to the last line.

#ifdef POPUP_PRETTY
      if(Line[Last_Line].tex_flag) {
	if ((Line[Last_Line].str = (Char *)malloc(strlen(POPUP_PRETTY_STR) + 1)) == NULL) return CONSOLE_MEM_ERR;
	strcpy((char*)Line[Last_Line].str, (const char*)POPUP_PRETTY_STR);
	if ((Line[Last_Line].tex_str = (Char *)malloc(strlen((const char *)Edit_Line) + 1)) == NULL) return CONSOLE_MEM_ERR;
	strcpy((char *)Line[Last_Line].tex_str, (const char *)Edit_Line);
      }
      else {
	if ((Line[Last_Line].str = (Char *)malloc(strlen((const char *)Edit_Line) + 1)) == NULL) return CONSOLE_MEM_ERR;
	strcpy((char *)Line[Last_Line].str, (const char *)Edit_Line);
      }
#else
      if ((Line[Last_Line].str = (Char *)malloc(strlen((const char *)Edit_Line) + 1)) == NULL) return CONSOLE_MEM_ERR;
      strcpy((char *)Line[Last_Line].str, (const char *)Edit_Line);
#endif

      if ((Line[Last_Line].disp_len = Console_GetDispLen(Line[Last_Line].str)) == CONSOLE_ARG_ERR) return CONSOLE_ARG_ERR;
      Line[Last_Line].type = pre_line_type;
      Line[Last_Line].readonly = pre_line_readonly;
      Line[Last_Line].start_col = 0;

      Edit_Line[0] = '\0';

      Last_Line++;

      Cursor.x = 0;

      if ((Last_Line - Start_Line) == LINE_DISP_MAX)
	{
	  Start_Line++;
	}
      else
	{
	  Cursor.y++;
	}

      Line[Last_Line].str = Edit_Line;
      Line[Last_Line].readonly = 0;
      Line[Last_Line].type = LINE_TYPE_INPUT;
      Line[Last_Line].start_col = 0;
      Line[Last_Line].disp_len = 0;

      return CONSOLE_NEW_LINE_SET;
    }
  else
    {
      return CONSOLE_NO_EVENT;
    }
}

void Console_Insert_Line(){
  if (Last_Line>=LINE_MAX-1)
    return;
  for (int i=Last_Line;i>=Current_Line;--i){
    Line[i+1]=Line[i];
  }
  ++Last_Line;
  int i=Current_Line;
  line & l=Line[i];
  l.str=(Char *)malloc(2);
  strcpy((char *)l.str,"0");
  l.type=Line[i+1].type==LINE_TYPE_INPUT?LINE_TYPE_OUTPUT:LINE_TYPE_INPUT;
  l.start_col=0;
  l.readonly=1;
  l.disp_len=Console_GetDispLen(l.str);
}

/*
  The following function is used to delete a character before the cursor.
*/

int Console_Backspace()
{
  console_changed=1;
  if (Last_Line>0 && Current_Line<Last_Line){
    int i=Current_Line;
    if (Edit_Line==Line[i].str)
      Edit_Line=Line[i+1].str;
    if (Line[i].str){
      copy_clipboard((const char *)Line[i].str);
      free(Line[i].str);
    }
    for (;i<Last_Line;++i){
      Line[i]=Line[i+1];
    }
    Line[i].readonly = 0;
    Line[i].type = LINE_TYPE_INPUT;
    Line[i].start_col = 0;
    Line[i].disp_len = 0;
    Line[i].str=0;
    --Last_Line;
    if (Start_Line>0)
      --Start_Line;
    else {
      if (Cursor.y>0)
	--Cursor.y;
    }
#if 1
    if (Last_Line==0 && Current_Line==0){ // workaround
      char buf[strlen((const char*)Edit_Line)+1];
      strcpy(buf,(const char*)Edit_Line);
      Console_Init();
      Console_Clear_EditLine();
      if (buf[0])
	Console_Input((const Char *)buf);
      //std::string status(giac::print_INT_(Last_Line)+" "+(giac::print_INT_(Current_Line)+" ")+giac::print_INT_(Line[Current_Line].str)+" "+(const char*)Line[Current_Line].str);
      //DefineStatusMessage(status.c_str(),1,0,0);
      //DisplayStatusArea();
    }
#endif
    Console_Disp(1);
    return CONSOLE_SUCCEEDED;
  }
  int return_val;
  return_val = Console_DelStr(Edit_Line, Current_Col, 1);
  if (return_val != CONSOLE_SUCCEEDED) return return_val;
  Line[Current_Line].disp_len = Console_GetDispLen(Edit_Line);
  return_val=Console_MoveCursor(CURSOR_LEFT);
  Console_Disp(0);
  return return_val;
}

/*
  ÒÔÏÂº¯ÊýÓÃÓÚ´¦Àí°´¼ü¡£
  The following functions are used to deal with the key.
*/

unsigned translate_fkey(unsigned input_key){
  if (input_key==KEY_CHAR_MAT) input_key=KEY_CTRL_F10;
  if (input_key==KEY_CTRL_QUIT) input_key=KEY_CTRL_F16;
#ifndef TICE
  if (input_key==KEY_CTRL_MIXEDFRAC) input_key=KEY_CTRL_F11;
  //if (input_key==KEY_CTRL_FRACCNVRT) input_key=KEY_CTRL_F13;
  //if (input_key==KEY_CHAR_LIST) input_key=KEY_CTRL_F13;
  if (input_key==KEY_CTRL_OPTN) input_key=KEY_CTRL_F15;
  //if (input_key==KEY_CTRL_PRGM) input_key=KEY_CTRL_F18;
  if (input_key==KEY_CTRL_FD) input_key=KEY_CTRL_F12;
  if (input_key==KEY_CHAR_ANGLE) input_key=KEY_CTRL_F17;
  if (input_key==KEY_CHAR_FRAC) input_key=KEY_CTRL_F20;
#endif
  return input_key;
}

void chk_clearscreen(){
  drawRectangle(0, 18, LCD_WIDTH_PX, LCD_HEIGHT_PX-18, _WHITE);
  if (confirm(lang?"Effacer l'historique?":"Clear history?",lang?"F1: annuler,   F5: effacer":"F1: cancel,   F5: erase",true)==KEY_CTRL_F5){
    Console_Init();
    Console_Clear_EditLine();
  }    
  Console_Disp(1);
}

void reload_edptr(const char * filename,textArea *edptr){
  if (edptr){
    std::string s(merge_area(edptr->elements));
    copy_clipboard(s);
    s="\n";
    edptr->elements.clear();
    edptr->clipline=-1;
    edptr->filename=remove_path(remove_extension(filename))+".py";
    load_script(edptr->filename.c_str(),s);
    if (s.empty())
      s="\n";
    // cout << "script " << edptr->filename << endl;
    edptr->editable=true;
    edptr->changed=false;
    edptr->python=true;
    edptr->elements.clear();
    edptr->y=0;
    add(edptr,s);
    edptr->line=0;
    edptr->pos=0;
  }
}  

int Console_Eval(const char * buf){
  int start=Current_Line;
  free(Line[start].str);
  Line[start].str=(Char *)malloc(strlen(buf)+1);
  strcpy((char *)Line[start].str,buf);
  run_session(start);
  int move_line = Last_Line - start;
  for (int i = 0; i < move_line; i++)
    Console_MoveCursor(CURSOR_UP);
  return CONSOLE_SUCCEEDED;
}

bool inputdouble(const char * msg1,double & d){
  std::string s1;
  if (inputline(msg1,lang?"Nouvelle valeur?":"New value?",s1,false)==KEY_CTRL_EXIT)
    return false;
  return stringtodouble(s1,d);
}

  // back is the number of char that should be deleted before inserting
  string help_insert(const char * cmdline,int & back,bool warn){
    back=0;
    int l=strlen(cmdline);
    char buf[l+128];
    strcpy(buf,cmdline);
    bool openpar=l && buf[l-1]=='(';
    if (openpar){
      buf[l-1]=0;
      --l;
      ++back;
    }
    for (;l>0;--l){
      if (!is_alphanum(buf[l-1]) && buf[l-1]!='_')
	break;
    }
    // cmdname in buf+l
    const char * cmdname=buf+l,*cmdnameorig=cmdname;
    l=strlen(cmdname);
    int res=doCatalogMenu(buf,(char *)"Index",0,cmdname);
    if (!res)
      return "";
    return cmdname+l;
  }

#ifndef TICE
  string print_INT_(int i){
    char c[256];
    sprint_int(c,i); // my_sprintf(c,"%d",i);
    return c;
  }

  string hexa_print_INT_(int i){
    string res;
    for (i=(i&0x7fffffff);i;){
      int j=i&0xf;
      i >>= 4;
      if (j>=10)
	res =char('a'+(j-10))+res;
      else
	res =char('0'+j)+res;
    }
    return "0x"+res;
  }
#else
  string print_INT_(int i){
    char c[16]; // impossible for output to be longer than this
    _ti_sprintf(c, "%d", i);
    return c;
  }

  string hexa_print_INT_(int i){
    char c[16]; // impossible for output to be longer than this
    _ti_sprintf(c, "0x%x", i);
    return c;
  }

#endif

  int chartab(){
    // display table
    drawRectangle(0,18,LCD_WIDTH_PX,LCD_HEIGHT_PX-18,_WHITE);
    // os_draw_string(0,0,_BLACK,_WHITE,lang==1?"Selectionner caractere":"Select char");
    Printxy(0,0, (lang==1?"Selection caractere (zoom)":"Select char (zoom)"),TEXT_MODE_NORMAL);
    int dy=16;
    for (int r=0;r<6;++r){
      for (int c=0;c<16;++c){
        int currc=32+16*r+c;
        char buf[8]={(char)(currc==127?'X':currc),32,0};
        Printxy(1+14*c,dy+16*r,buf,0);
      }
    }
    static int row=0,col=0;
    for (;;){
      col &= 0xf;
      if (row<0) row=5; else if (row>5) row=0;
      int currc=32+16*row+col;
      char buf[8]={(char)(currc==127?'X':currc),32,0};
      Printxy(1+14*col,dy+16*row,buf,1); // draw char selected
      #ifndef TICE
        string s("Current ");
        s += char(currc);
        s += " ";
        s += print_INT_(currc);
        s += " ";
        s += hexa_print_INT_(currc);
        s += "  ";
        Printxy(0,16*10,s.c_str(),TEXT_MODE_NORMAL);
      #else
        char s[50]; // should be more than enough
        _ti_sprintf(s, "Current %c %d %#x  ", char(currc), currc, currc);
        Printxy(0,16*10,s,TEXT_MODE_NORMAL);
      #endif
      
      // interaction
      int key; ck_getkey(&key);
      Printxy(1+14*col,dy+16*row,buf,0); // undo draw char selected
      if (key==KEY_CTRL_EXIT){
	drawRectangle(0,18,LCD_WIDTH_PX,LCD_HEIGHT_PX-18,_WHITE);	
	return -1;
      }
      if (key==KEY_CTRL_EXE){
	drawRectangle(0,18,LCD_WIDTH_PX,LCD_HEIGHT_PX-18,_WHITE);
	return currc;
      }
      if (key==KEY_CTRL_LEFT)
	--col;
      if (key==KEY_CTRL_RIGHT)
	++col;
      if (key==KEY_CTRL_UP)
	--row;
      if (key==KEY_CTRL_DOWN)
	++row;
    }
  }

extern "C" const char * const * mp_vars();
extern "C" int gc_ramfree();
int trialpha(const void *p1,const void * p2){
  int i=strcmp(* (char * const *) p1, * (char * const *) p2);
  return i;
}

const char * trig(){
  // int w=5*18,h=8; 
  int w=5*38,h=10,x=LCD_WIDTH_PX-w,y=LCD_HEIGHT_PX-STATUS_AREA_PX-h; 
  unsigned char buf[w*h];
  unsigned char * saveptr=buf;  
  for (int j=0;j<h;++j){
    unsigned char * lcdramptr=((unsigned char *)lcd_Ram)+(j+y+STATUS_AREA_PX)*LCD_WIDTH_PX+x;
    for (int i=0;i<w;++i,++lcdramptr,++saveptr){
      *saveptr=*lcdramptr;
    }
  }
  drawRectangle(x,y+STATUS_AREA_PX,w,h,COLOR_BLACK);
  os_draw_string_small(x,y+1,COLOR_CYAN,COLOR_BLACK,"1:sin 2:cos 3:tan 4:asin 5:acos 6:atan",false);
  int k;
  for (;;){
    k=getkey(1);
    if (k==KEY_CTRL_EXIT || k==KEY_CTRL_QUIT || k==KEY_CTRL_AC)
      k=0;
    else if (k==KEY_CTRL_EXE)
      k=1;
    else
      k-='0';
    if (k>=0 && k<=6)
      break;
  }
  saveptr=buf;  
  for (int j=0;j<h;++j){
    unsigned char * lcdramptr=((unsigned char *)lcd_Ram)+(j+y+STATUS_AREA_PX)*LCD_WIDTH_PX+x;
    for (int i=0;i<w;++i,++lcdramptr,++saveptr){
      *lcdramptr=*saveptr;
    }
  }
  char * tab[]={"","sin(","cos(","tan(","asin(","acos(","atan("};
  return tab[k];
}

  const char * keytostring(int key,int keyflag,bool py){
    const int textsize=512;
    bool alph=keyflag==4||keyflag==0x84||keyflag==8||keyflag==0x88;
    static char text[textsize];
    switch (key){
    case KEY_CHAR_PLUS:
      return "+";
    case KEY_CHAR_MINUS:
      return "-";
    case KEY_CHAR_PMINUS:
      return "_";
    case KEY_CHAR_MULT:
      return "*";
#ifndef TICE
    case KEY_CHAR_FRAC:
      return py?"\\":"solve(";
#endif
    case KEY_CHAR_DIV: 
      return "/";
    case KEY_CHAR_POW:
      return "^";
    case KEY_CHAR_ROOT:
      return "sqrt(";
    case KEY_CHAR_SQUARE:
      return py?"**2":"^2";
#ifndef TICE
    case KEY_CHAR_CUBEROOT:
      return py?"**(1/3)":"^(1/3)";
    case KEY_CHAR_POWROOT:
      return py?"**(1/":"^(1/";
#endif
    case KEY_CHAR_RECIP:
      return py?"**-1":"^-1";
#ifndef TICE
    case KEY_CHAR_THETA:
      return "phase(";
    case KEY_CHAR_VALR:
      return "abs(";
    case KEY_CHAR_ANGLE:
      return "polar_complex(";
#endif
    case KEY_CTRL_XTT:
      return xthetat?"t":"x";
      //return "x"; 
    case KEY_CHAR_LN:
      return "log(";
    case KEY_CHAR_LOG:
      return "log10(";
    case KEY_CHAR_EXPN10:
      return "10**(";
    case KEY_CHAR_EXPN:
      return "exp(";
    case KEY_CHAR_EXP:
      return "e";
    case KEY_CHAR_SIN:
#ifdef FRANCAIS 
      return trig();
#else
      return "sin(";
    case KEY_CHAR_COS:
      return "cos(";
    case KEY_CHAR_TAN:
      return "tan(";
    case KEY_CHAR_ASIN:
      return "asin(";
    case KEY_CHAR_ACOS:
      return "acos(";
    case KEY_CHAR_ATAN:
      return "atan(";
#endif
#ifndef TICE
    case KEY_CTRL_MIXEDFRAC:
      return "limit(";
    case KEY_CTRL_FRACCNVRT:
      return "approx(";
      //case KEY_CTRL_FORMAT: return "purge(";
    case KEY_CTRL_FD:
      return "exact(";
#endif
    case KEY_CHAR_STORE:
      // if (keyflag==1) return "inf";
      return "=>";
    case KEY_CHAR_IMGNRY:
      return "i";
    case KEY_CHAR_PI:
      return "pi";
    case KEY_CTRL_VARS: {
      return select_var();
    }
    case KEY_CHAR_ANS:
      return "ans()";
    case KEY_CTRL_F3: {
      static string * sptr=0;
      if (!sptr)
	sptr=new string(" ");
      int c=chartab();
      if (c<0) return "";
      (*sptr)[0] = (c<32 || c==127)?0:char(c);
      return *sptr; // ":=";
    }
    case KEY_CHAR_COMMA:
      // if (keyflag==1) return "solve(";
      return ",";
    case KEY_CHAR_LPAR:
      return "(";
    case KEY_CHAR_RPAR:
      return ")";
    case KEY_CHAR_LBRCKT:
      return "[";
    case KEY_CHAR_RBRCKT:
      return "]";
    case KEY_CHAR_LBRACE:
      return "{";
    case KEY_CHAR_RBRACE:
      return "}";
    case KEY_CHAR_EQUAL:
      return "=";
    case KEY_CTRL_F2:
      return (alph)?"#":(keyflag==1?"<":">");
    case KEY_CTRL_F1:
      return (alph)?"\\":(keyflag==1?"%":"'");
    case KEY_CTRL_PASTE:
      return paste_clipboard();
    case KEY_CHAR_MAT:{
      // const char * ptr=input_matrix(false); if (ptr) return ptr;
      if (showCatalog(text,12)) return text;
      return "";
    }
    case KEY_CHAR_LIST: {
      //const char * ptr=input_matrix(true); if (ptr) return ptr;
      if (showCatalog(text,13)) return text;
      return "";
    }
    case KEY_CTRL_SOLVE:{
      // const char * ptr=input_matrix(false); if (ptr) return ptr;
      if (showCatalog(text,15)) return text;
      return "";
    }
    case KEY_CTRL_CATALOG:
      if(showCatalog(text,1)) 
	return text;
      return "";
#if 0
    case KEY_CTRL_F4:
      if(showCatalog(text,0)) 
	return text;
      return "";
    case KEY_CTRL_QUIT: 
      if(showCatalog(text,11))
	return text;
      return "";
    case KEY_CTRL_SETUP:
      if(showCatalog(text,7))
	return text;
      return "";
#endif
    }
    return 0;
  }

  bool console_help_insert(bool warn=true){
    if (!Edit_Line)
      return false;
    char buf[strlen((char *)Edit_Line)+1];
    strcpy(buf,(char *)Edit_Line);
    buf[Line[Current_Line].start_col+Cursor.x]=0;
    int back;
    string s=help_insert(buf,back,warn);
    if (s.empty())
      return false;
    for (int i=0;i<back;++i)
      Console_Backspace();
    Console_Input((const Char *)s.c_str());
    Console_Disp(0);
    return true;
  }

string adjust(const char * s,int L=12){
  int l=strlen(s);
  string res(s);
  if (l>L)
    res=res.substr(0,L);
  else {
    for (int i=0;i<L-l;++i)
      res += ' ';
  }
  return res;
}


void set_xcas_status(){
  statuslinemsg(session_filename);
  statusline(0);
}

// app=0 for console, 1 for editor, 2 for eqw, 3 spreadsheet
void get_current_console_menu(string & menu,string & shiftmenu,string & alphamenu,int &menucolorbg,int app){
  shiftmenu = adjust(menu_f6);
  shiftmenu += "|";
  shiftmenu += adjust(menu_f7);
  shiftmenu += "|";
  shiftmenu += app==2?"   zoom   ":adjust(menu_f8);
  shiftmenu += "|";
  shiftmenu += app==2?"   evalf  ":adjust(menu_f9);
  shiftmenu += "|";
  shiftmenu += adjust(menu_f10);
  //shiftmenu += "|";
  //shiftmenu += app==2?"evalf ":(app==3?" prog ":adjust(menu_f12));
  alphamenu = adjust(menu_f11);
  alphamenu += "|";
  alphamenu += adjust(menu_f12);
  alphamenu += "|";
  alphamenu += app==2?"   zoom   ":adjust(menu_f13);
  alphamenu += "|";
  alphamenu += app==2?"  regroup  ":adjust(menu_f14);
  alphamenu += "|";
  alphamenu += adjust(menu_f15);
  //alphamenu += "|";
  //alphamenu += app==2?"evalf ":adjust(menu_f18);
  if (0 && app==3){
    menu=(lang?" outil | stat | edit | cmds | A<>a | menu":" tools | stat | edit | cmds | A<>a | menu");
#ifndef FX
    menucolorbg=COLOR_ORANGE;
#endif
    return;
  }
  if (app==2){
    menu += adjust(menu_f1);
    menu += "| ";
    menu += adjust(menu_f2);
    menu += "|  edit sel  |";
    menu += "   eval   |  ";
    menu += "|  copy sel  ";
    menucolorbg=34800;
    return;
  }
  if (0 && app==5){
    menu=" point | lines | disp | cmds | A<>a | file ";
    shiftmenu="triangl|polyg|geo3d|solids|gdiff|measur";
    alphamenu="tests|analyt|cursor|transf|plots|conic";
    menucolorbg=COLOR_CYAN;
    return;
  }
  if (app==1){
    menu=lang==1?" if/else... | def/for... |  edition  |   char/io   |  Fichier    ":" if/else... | def/for... |     edit    |  char/io  |    File    ";
  }
  else {
    menu += adjust(menu_f1);
    menu += "| ";
    menu += adjust(menu_f2);
    menu += "| ";
    menu += adjust(menu_f3);
    menu += "|  chartab "; //     menu += adjust(menu_f3);
    menu += lang?"|  Fichier  ":"|    File     ";
  }
  //drawRectangle(0,174,LCD_WIDTH_PX,24,COLOR_BLACK);
  int python_color=65520;
  if (app==1){
    python_color=65512;
  }
  menucolorbg=python_color;
}

void console_disp_status(int keyflag){
  Console_FMenu_Init();
  string menu(" "),shiftmenu=menu,alphamenu; int menucolorbg=12345;
  get_current_console_menu(menu,shiftmenu,alphamenu,menucolorbg,0);
  //dbg_printf("keyflag=%i menu=%s %s %s\n",keyflag,menu.c_str(),shiftmenu.c_str(),alphamenu.c_str());
  if (keyflag==1) menu=shiftmenu;
  if (keyflag & 0xc) menu=alphamenu;
  Printmini(0,C58,menu.c_str(),MINI_REV);
  // status, clock, 
  set_xcas_status();
  Bdisp_PutDisp_DD();
}  

void text_disp_menu(int keyflag){
  Console_FMenu_Init();
  string menu(" "),shiftmenu=menu,alphamenu; int menucolorbg=12345;
  get_current_console_menu(menu,shiftmenu,alphamenu,menucolorbg,1);
  if (keyflag==1) menu=shiftmenu;
  if (keyflag & 0xc) menu=alphamenu;
  Printmini(0,C58,menu.c_str(),MINI_REV);
  Bdisp_PutDisp_DD();
}

#ifdef WITH_PERIODIC
string run_periodic_table(){
  const char * name,*symbol;
  char protons[32],nucleons[32],mass[32],electroneg[32];
  string s;
  int res=periodic_table(name,symbol,protons,nucleons,mass,electroneg);
  if (!res)
    return s;
  if (res & 1)
    s += name;
  if (res & 2){
    if (res & 1)
      s+=',';
    s+=symbol;
  }
  if (res & 4){
    if (res&3)
      s+=',';
    s+=protons;
  }
  if (res & 8){
    if (res&7)
      s+=',';
    s+=nucleons;
  }
  if (res & 16){
    if (res&15)
      s+=',';
    s+=mass+2;
  }
  if (res & 32){
    if (res&31)
      s+=',';
    s+=electroneg+4;
  }
  return s;
}
#endif

int Console_GetKey(){
  unsigned int key, i, move_line, move_col;
  Char tmp_str[3];
  Char *tmp;
  int oldkeyflag=-1;
  for (;;){
    int keyflag = (Char)Setup_GetEntry(0x14);
    bool alph=keyflag==4||keyflag==0x84||keyflag==8||keyflag==0x88;
    if (oldkeyflag!=keyflag)
      console_disp_status(keyflag);
    oldkeyflag=keyflag;
    ck_getkey((int *)&key);
    if (key==KEY_CTRL_QUIT
#ifndef WITH_PLOT
        || key==KEY_CTRL_F1
#endif
        )
      return -2;
    if (key==KEY_CTRL_SHIFT || key==KEY_CTRL_ALPHA)
      continue;
    //if (1){ char buf1[32],buf2[32]; sprint_double(buf1,key),sprint_double(buf2,KEY_CTRL_F7); confirm(buf1,buf2); }
#ifdef TICE
    if (key==KEY_CHAR_MAT) key=KEY_CTRL_F10;
    if (key==KEY_CTRL_STATS) {
#ifdef WITH_SHEET
      sheet();
      return 0;
#else
      key=KEY_CTRL_F16;
#endif
    }
#else
    if (key!=KEY_CHAR_FRAC && key!=KEY_CTRL_MIXEDFRAC)
      key=translate_fkey(key);
#endif
    if (key==KEY_CHAR_LIST) 
      return Console_FMenu(KEY_CTRL_F12);
    if (key>=KEY_LIST1 && key<=KEY_LIST9){
      tmp_str[0] = 'Y';
      tmp_str[1] = key-KEY_LIST1+'1';
      tmp_str[2] = '\0';
      Console_Input(tmp_str);
      Console_Disp(0);
      continue;
    }
    if (
        // key >= '.' && key <= '9'
        key>=' ' && key<=126
        ){
      tmp_str[0] = key;
      tmp_str[1] = '\0';
      Console_Input(tmp_str);
      Console_Disp(0);
      continue;
    }
    //if (key==KEY_CTRL_F5){      handle_f5();      Console_Disp();      continue;    }
    if (key==KEY_CTRL_PASTE)
      return Console_Input((const Char*) paste_clipboard());
    if (key==KEY_CTRL_VARS){
      const char * ptr=keytostring(key,0,0);
      if (ptr)
	return Console_Input((const Char *)ptr);
    }
#ifndef TICE
    if (key==KEY_CTRL_OPTN){
      char buf[256];
      if (doCatalogMenu(buf,"OPTIONS",14)) // should be doCatalogMenu(buf,"OPTION",1)
	return Console_Input((const Char *)buf);
      Console_Disp(1);  oldkeyflag=-1;
      continue;      
    }
#endif
#if 0 // CATALOG is not returned by GetKey, we will use OPTN instead
    if (key==KEY_CTRL_CATALOG){
      char buf[256];
      if (doCatalogMenu(buf,"CATALOG",0))
	return Console_Input((const Char *)buf);
    }
#endif
    if (key == KEY_CTRL_F4){
      int key=chartab();
      if (key>0){
	char buf[]={(char)key,0};
	return Console_Input((const Char *)buf);
      }
      return CONSOLE_SUCCEEDED;
    }
#ifdef WITH_EQW
    if (key==KEY_EQW_TEMPLATE && Current_Line==Last_Line){
      char buf[max(GEN_PRINT_BUFSIZE,strlen(Edit_Line)+1)];
      strcpy(buf,(const char *)Edit_Line);
      if (buf[0]==0){
        buf[0]='0';
        buf[1]=0;
      }
      if (xcas::eqws(buf,false /* eval */)){
        Console_Clear_EditLine();
        return Console_Input((const Char *)buf);
      }
      Console_Disp(1);
      continue;
    }
#endif
    if ( (key==KEY_CTRL_RIGHT || key==KEY_CTRL_LEFT || key==KEY_EQW_TEMPLATE) && Current_Line<Last_Line){
      int l=Current_Line;
      bool graph=strcmp((const char *)Line[l].str,"Graphic object")==0;
      if (graph && l>0) --l;
      char buf[max(GEN_PRINT_BUFSIZE,strlen((const char *)Line[l].str)+1)];
      strcpy(buf,(const char *)Line[l].str);
#ifdef WITH_EQW
      if ( (alph || key==KEY_CTRL_RIGHT) ?textedit(buf):xcas::eqws(buf,graph /* eval */))
#else
      if (alph || textedit(buf))
#endif
      {
	if (Current_Line==Last_Line){
	  Console_Clear_EditLine();
	  return Console_Input((const Char *)buf);
	}
	else {
#if 1
	  if (Line[l].type==LINE_TYPE_INPUT && l<Last_Line-1 && Line[l+1].type==LINE_TYPE_OUTPUT)
	    return Console_Eval(buf);
	  else {
	    free(Line[l].str);
	    Line[l].str=(Char*)malloc(strlen(buf)+1);
	    Line[l].disp_len = Console_GetDispLen(Line[l].str);
	    strcpy((char *)Line[l].str,buf);
	  }
#else
	  int x=editline_cursor;
 	  move_line = Last_Line - Current_Line;
	  for (i = 0; i <=move_line; i++) Console_MoveCursor(CURSOR_DOWN);
	  Cursor.x=x;
	  return Console_Input((const Char *)buf);
#endif
	}	  
      }
      Console_Disp(1);
      continue;
    }
    if (key==KEY_CTRL_SYMB){
      char buf[512];
      if (!showCatalog(buf,0,0))
	buf[0]=0;
      return Console_Input((const Char*)buf);
    }
    if (key==KEY_CTRL_F5){
#if 1
      Menu smallmenu;
#ifdef WITH_PERIODIC
#ifdef WITH_SHEET
      smallmenu.numitems=18;
#else
      smallmenu.numitems=17;
#endif
#else
      smallmenu.numitems=16;
#endif
      MenuItem smallmenuitems[smallmenu.numitems];
      
      smallmenu.items=smallmenuitems;
      smallmenu.height=12;
      smallmenu.scrollbar=1;
      smallmenu.scrollout=1;
      //smallmenu.title = "KhiCAS";
      smallmenuitems[0].text = (char *) (lang?"Enregistrer session":"Save session ");
      smallmenuitems[1].text = (char *) (lang?"Enregistrer sous":"Save session as");
      smallmenuitems[2].text = (char*) (lang?"Charger session":"Load session");
      smallmenuitems[3].text = (char*)(lang?"Nouvelle session":"New session");
      smallmenuitems[4].text = (char*)(lang?"Executer session":"Run session");
      smallmenuitems[5].text = (char*)(lang?"Editeur script":"Script editor");
      smallmenuitems[6].text = (char*)(lang?"Ouvrir script":"Open script");
      smallmenuitems[7].text = (char*)(lang?"Exec script (->)":"Run script (->)");
      smallmenuitems[8].text = (char*)(lang?"Effacer historique":"Clear history");
      smallmenuitems[9].text = (char*)(lang?"Effacer script":"Clear script");
#ifdef WITH_EQW
      smallmenuitems[10].text = (char*)(lang?"Editer expression [X,T...]":"Expression edit"); 
      smallmenuitems[11].text = (char*)(lang?"Editer matrice [matr]":"Matrix edit");
#else
      smallmenuitems[10].text = (char*)"Expression Y0-Y9 [X,T...]"; 
      smallmenuitems[11].text = (char*)"Matrices [A]-[I] [matr]";
#endif
      smallmenuitems[12].text = (char*)"Config [mode]";
      smallmenuitems[13].text = (char *) (lang?"Raccourcis":"Shortcuts");
      smallmenuitems[14].text = (char*) (lang?"A propos":"About");
#ifdef WITH_PERIODIC
      smallmenuitems[15].text = (char*) (lang?"Tableau periodique":"Periodic table");
#ifdef WITH_SHEET
      smallmenuitems[16].text = (char*) (lang?"Tableur":"Spreadsheet");
#endif
#endif
      smallmenuitems[smallmenu.numitems-1].text = (char*) (lang?"Quitter":"Quit");
      // smallmenuitems[2].text = (char*)(isRecording ? "Stop Recording" : "Record Script");
      while(1) {
        int sres = doMenu(&smallmenu);
        if(sres == MENU_RETURN_SELECTION) {
          const char * ptr=0;
          if (smallmenu.selection==1){
            if (strcmp(session_filename,"session")==0)
              smallmenu.selection=2;
            else {
              save(session_filename);
              break;
            }
          }
          if (smallmenu.selection==2){
            char buf[270];
            if (get_filename(buf,".xw")){
              save(buf);
              string fname(remove_path(remove_extension(buf)));
              strcpy(session_filename,fname.c_str());
              if (edptr)
                edptr->filename="\\\\fls0\\"+fname+".py";
            }
            break;
          }
          if (smallmenu.selection==3){
            char filename[MAX_FILENAME_SIZE+1];
            if (fileBrowser(filename, (char*)"*.xw", (char *)"Sessions")){
              if (console_changed==0 || strcmp(session_filename,"session")==0 || confirm(lang?"Session courante perdue?":"Current session will be lost",lang?"F1: annul, F5: ok":"F1: cancel, F5: ok")==KEY_CTRL_F5){
                restore_session(filename);
                strcpy(session_filename,remove_path(remove_extension(filename)).c_str());
                // reload_edptr(session_filename,edptr);
              }     
            }
            break;
          }
          if (0 && smallmenu.selection==3) {
            // FIXME: make a menu catalog?
            char buf[512];
            if (doCatalogMenu(buf,"CATALOG",0))
              return Console_Input((const Char *)buf);
             oldkeyflag=-1;
             break;
          }
          if (smallmenu.selection==4) {
            char filename[MAX_FILENAME_SIZE+1];
            drawRectangle(0, 16, LCD_WIDTH_PX, LCD_HEIGHT_PX-24, _WHITE);
            if (get_filename(filename,".xw")){
              if (console_changed==0 || strcmp(session_filename,"session")==0 || confirm(lang?"Session courante perdue?":"Current session will be lost",lang?"F1: annul, F5: ok":"F1: cancel, F5: ok")==KEY_CTRL_F5){
                Console_Init();
                Console_Clear_EditLine();
                std::string s(remove_path(remove_extension(filename)));
                strcpy(session_filename,s.c_str());
                reload_edptr(session_filename,edptr);
              }
            }  
            break;
          }
          if (smallmenu.selection==5) {
            run_session();
            break;
          }
          if (smallmenu.selection==6) {
            if (!edptr || merge_area(edptr->elements).size()<2)
              edit_script((remove_extension(session_filename)+".py").c_str());
            else
              doTextArea(edptr);
            break;
          }
          if (smallmenu.selection==7) {
            char filename[MAX_FILENAME_SIZE+1];
            drawRectangle(0, 16, LCD_WIDTH_PX, LCD_HEIGHT_PX-24, _WHITE);
            if (fileBrowser(filename, (char*)"*.py", (char *)"Scripts"))
              edit_script(filename);
            break;
          }
          if (smallmenu.selection==8) {
            select_script_and_run();
            break;
          }
          if(smallmenu.selection == 9) {
            chk_restart();
            Console_Init();
            Console_Clear_EditLine();
            break;
          }
          if (smallmenu.selection==10){
            erase_script();
            break;
          }
          if (smallmenu.selection==12){
#ifdef WITH_EQW
            const char * ptr=input_matrix(false);
            if (ptr)
              return Console_Input(ptr);
            else
              break;
#else
            print_msg12(lang?"Importer matrice [A]-[I]:":"Import matrix [A]-[I]:",lang?"Taper une touche entre A et I":"Type key A to I");
            lock_alpha();
            int key;
            ck_getkey(&key);
            reset_kbd();
            if (key>='a' && key<='i') {
              string mats=get_timatrix(key-'a');
              if (mats.size()){
                mats += "=>";
                mats += char(key);
                return Console_Input(mats.c_str());
              }
            }
            continue;
#endif
          }
          if (smallmenu.selection == 11){
#ifdef WITH_EQW
            char buf[GEN_PRINT_BUFSIZE+2];
            buf[0]='0'; buf[1]=0;
            if (xcas::eqws(buf,false/* eval */))
              return Console_Input(buf);
            else
              break;
#else
            print_msg12(lang?"Importer expression Y0-Y9:":"Import expression Y0-Y9:",lang?"Taper une touche entre 0 et 9":"Type key 0 to 9");
            reset_kbd();
            int key;
            ck_getkey(&key);
            if (key>='0' && key<='9') {
              char buf[3]={0x5e,0,0};
              buf[1]=(key=='0')?0x19:(0x10+(key-'1'));
              string val=get_tivar(buf);
              if (val.size()){
                val += "=>y";
                val += char(key);
                return Console_Input(val.c_str());
              }
            }
            continue;
#endif
          }
          if (smallmenu.selection == 13){
            menu_setup();
            continue;
          }
#ifdef WITH_PERIODIC
          if (smallmenu.selection == 16)
            return Console_Input(run_periodic_table());
#ifdef WITH_SHEET
          if (smallmenu.selection == 17){
            sheet();
            return 0;
          }
#endif
#endif
          if (smallmenu.selection == smallmenu.numitems){
            return -2;
          }
          if(smallmenu.selection >= 14) {
            textArea text;
            text.editable=false;
            text.clipline=-1;
            text.title = smallmenuitems[smallmenu.selection-1].text;
            add(&text,smallmenu.selection==14?shortcuts_string:apropos_string);
            text.minimini=false;
            doTextArea(&text);
            continue;
          } 
        }
        break;
      } // end while(1)
      Console_Disp(1);
      return CONSOLE_SUCCEEDED;
#else
      char filename[MAX_FILENAME_SIZE+1];
      //drawRectangle(0, 18, LCD_WIDTH_PX, LCD_HEIGHT_PX-18, _WHITE);
      if (get_filename(filename))
	edit_script(filename);
      //edit_script(0);
      return CONSOLE_SUCCEEDED;
#endif
    }
    if ( (key >= KEY_CTRL_F1 && key <= KEY_CTRL_F6) ||
	 (key >= KEY_CTRL_F7 && key <= KEY_CTRL_F20) 
	 ){
      return Console_FMenu(key);
    }
    if (
        // (key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z')
        0 
        ){
      //if (Case == LOWER_CASE) key += 'a' - 'A';
      tmp_str[0] = key;
      tmp_str[1] = 0;
      Console_Input(tmp_str);
      Console_Disp(0);
      continue;
    }

    if (key == KEY_SHIFT_ANS){
      select_script_and_run();
      Console_Disp(1);
      continue;
    }
    if (key==KEY_CTRL_PAGEUP){
      for (int i=0;i<8;++i){
        if (Console_MoveCursor(CURSOR_UP)==CONSOLE_MEM_ERR)
          return CONSOLE_MEM_ERR;
      }
      Console_Disp(1);
      continue;
    }
    if (key==KEY_CTRL_PAGEDOWN){
      for (int i=0;i<8;++i){
        if (Console_MoveCursor(CURSOR_DOWN)==CONSOLE_MEM_ERR)
          return CONSOLE_MEM_ERR;
      }
      Console_Disp(1);
      continue;
    }
    if (key == KEY_CTRL_UP){
      int prevcursor=Cursor.y,prevstart=Start_Line;
      // redraw current line without selection console_displine();
      if (Console_MoveCursor(alph?CURSOR_ALPHA_UP:CURSOR_UP)==CONSOLE_MEM_ERR)
        return CONSOLE_MEM_ERR;
      if (Start_Line!=prevstart){
        Console_Disp(1); // improve: scroll area and redraw line?
      }
      else {
        console_displine(prevcursor,2);
        console_displine(Cursor.y,2);
      }
      continue;
    }
    if ( (key == KEY_CTRL_DOWN || key=='\t' || key==KEY_CTRL_F10
	  || key==KEY_CHAR_FRAC || key==KEY_CTRL_MIXEDFRAC
	  ) && Current_Line==Last_Line && !Line[Current_Line].readonly && Current_Col>0){
      // find cmdname
      console_help_insert(false);
      Console_Disp(1); oldkeyflag=-1;
      continue;
      // keytooltip=Console_tooltip(contextptr);
    }
    if (key == KEY_CTRL_DOWN){
      int prevcursor=Cursor.y,prevstart=Start_Line;
      if (Console_MoveCursor(alph?CURSOR_ALPHA_DOWN:CURSOR_DOWN)==CONSOLE_MEM_ERR)
        return CONSOLE_MEM_ERR;
      if (Start_Line!=prevstart){
        Console_Disp(1); // improve: scroll area and redraw line?
      }
      else {
        console_displine(prevcursor,2);
        console_displine(Cursor.y,2);
      }
      continue;
    }
    if (key == KEY_CTRL_LEFT){
      Console_MoveCursor(CURSOR_LEFT);
      Console_Disp(0);
      continue;
    }
    if (key == KEY_CTRL_RIGHT){
      Console_MoveCursor(CURSOR_RIGHT);
      Console_Disp(0);
      continue;
    }
    if (key == KEY_SHIFT_LEFT)	return Console_MoveCursor(CURSOR_SHIFT_LEFT);
    if (key == KEY_SHIFT_RIGHT)	return Console_MoveCursor(CURSOR_SHIFT_RIGHT);
    if (key==KEY_CTRL_PRGM){
      if (!edptr)
        edit_script((remove_extension(session_filename)+".py").c_str());
      else
        doTextArea(edptr);
      Console_Disp(1);
    }
    if (key == KEY_CTRL_EXIT){
      if (Last_Line==Current_Line){
        if (strlen(Edit_Line))
          Console_Clear_EditLine();
        else if (do_confirm(lang?"F1: Effacer historique, F5: annuler":"F1: Clear history, F5: Cancel")){
          Console_Init();
          Console_Clear_EditLine();
          Console_Disp(1);
        }
      }
      else {
	move_line = Last_Line - Current_Line;
	for (i = 0; i <= move_line; i++) Console_MoveCursor(CURSOR_DOWN);
      }
      return CONSOLE_SUCCEEDED;
    }
    if (key == KEY_CTRL_AC){
      if (Line[Current_Line].readonly){
        move_line = Last_Line - Current_Line;
        for (i = 0; i <= move_line; i++) Console_MoveCursor(CURSOR_DOWN);
        return CONSOLE_SUCCEEDED;
      }
      if (Edit_Line[0]=='\0'){
        //return Console_Input((const Char *)"restart");
        chk_clearscreen();
        continue;
      }
      Edit_Line[0] = '\0';
      Line[Current_Line].start_col = 0;
      Line[Current_Line].type = LINE_TYPE_INPUT;
      Line[Current_Line].disp_len = 0;
      Cursor.x = 0;
      return CONSOLE_SUCCEEDED;
    }
    
    if (key == KEY_CTRL_INS) {
      if (Current_Line<Last_Line){
	Console_Insert_Line();
	Console_Insert_Line();
      }
      else {
	int c=chartab();
	string s=" ";
	if (c>32 && c<127) s[0]=char(c);
	Console_Input((const Char *)s.c_str());
      }
      Console_Disp(1);
      continue;
    }
    
    if (key == KEY_CTRL_SETUP) {
      menu_setup();
      Console_Disp(1);
      continue;
    }
    
    if (key == KEY_CTRL_EXE){
      if (Current_Line == Last_Line)
        return Console_NewLine(LINE_TYPE_INPUT, 1);
      tmp = Line[Current_Line].str;
      
#if 1
      int x=editline_cursor;
      move_line = Last_Line - Current_Line;
      for (i = 0; i <= move_line; i++) Console_MoveCursor(CURSOR_DOWN);
      Cursor.x=x;
      if (Cursor.x>COL_DISP_MAX)
        Line[Last_Line].start_col=Cursor.x-COL_DISP_MAX;
#else
      move_line = Last_Line - Current_Line;
      for (i = 0; i <= move_line; i++) Console_MoveCursor(CURSOR_DOWN);
      move_col = Line[Current_Line].disp_len - Current_Col;
      for (i = 0; i <= move_col; i++) Console_MoveCursor(CURSOR_RIGHT);
#endif
      return Console_Input(tmp);
    }
    
    if (key == KEY_CTRL_DEL){
      int ret=Console_Backspace();
      //if (ret!=CONSOLE_SUCCEEDED) return ret;
      continue;
    }
    if (key == KEY_CHAR_CR && (Current_Line!=Last_Line || Cursor.x==0)){
      run_session(0);
      return 0;
    }
    if (key == KEY_CTRL_CLIP){
      copy_clipboard((const char *)Line[Current_Line].str);
    }
    if (const char * ptr=keytostring(key,keyflag,false)){
      if (ptr){
	Console_Input((const Char *)ptr);
        if (key==KEY_CTRL_CATALOG || key==KEY_CTRL_SOLVE)
          Console_Disp(1);
        else
          Console_Disp(0);
        continue;
      }
    }
    return CONSOLE_NO_EVENT;
  }
}

Char* original_cfg=0;

int Console_FMenu(int key){
  const char * s=console_menu(key,original_cfg,0),*ptr=0;
  if (!s){
    //cout << "console " << unsigned(s) << endl;
    return CONSOLE_NO_EVENT;
  }
  if (strcmp("matrix(",s)==0 && (ptr=input_matrix(false)) )
    s=ptr;
  if (strcmp("makelist(",s)==0 && (ptr=input_matrix(true)) )
    s=ptr;
  return Console_Input((const Char *)s);
}

const char * console_menu(int key,int active_app){
  return console_menu(key,original_cfg,active_app);
}

const char * console_menu(int key,Char* cfg_,int active_app){
  Char * cfg=cfg_;
  if (key>=KEY_CTRL_F7 && key<=KEY_CTRL_F20){ key += 6+KEY_CTRL_F1-KEY_CTRL_F7; }
  // if (1){ char buf1[32],buf2[32]; sprint_double(buf1,key),sprint_double(buf2,KEY_CTRL_F1); confirm(buf1,buf2); }
  int i, matched = 0;
  const char * ret=0;
  const int maxentry_size=64;
  static char console_buf[maxentry_size];
  char temp[maxentry_size],menu1[maxentry_size],menu2[maxentry_size],menu3[maxentry_size],menu4[maxentry_size],menu5[maxentry_size],menu6[maxentry_size],menu7[maxentry_size],menu8[maxentry_size],menu9[maxentry_size];
  char * tabmenu[]={menu1,menu2,menu3,menu4,menu5,menu6,menu7,menu8,menu9};
  struct FMenu entry = {0,tabmenu,0};
  // char* cfg = (char *)memory_load((char *)"\\\\fls0\\FMENU.cfg");

  while (*cfg) {
    //Get each line
    for(i=0; i<maxentry_size-1 && *cfg && *cfg!='\r' && *cfg!='\n'; i++, cfg++) {
      temp[i] = *cfg;
    }
    temp[i]=0;
    //If starting by 'F' followed by the right number, start filling the structure.
    if (temp[0] == 'F' && temp[1]==(key-KEY_CTRL_F1)+'1'){
      matched = 1;
      continue;
    }
    if (temp[0] == 'F' && temp[1]!=(key-KEY_CTRL_F1)+'0'){
      matched = 0;
      continue;
    }
    //Fill the structure
    if (matched && temp[0] && entry.count<9) {
      strcpy(tabmenu[int(entry.count)], temp);
      entry.count++;
    }
    cfg++;
  }
  if(entry.count > 0) {
    ret = Console_Draw_FMenu(key, &entry,cfg,active_app);
    // cout << "console0 " << (unsigned) ret << endl;
    if (!ret) return ret;
    if (strcmp(ret,"char table")==0){
      int key=chartab();
      if (key<0)
        return 0;
      char buf[]={(char)key,0};
      strcpy(console_buf,buf);
    }
#ifdef WITH_PERIODIC
    else if (!strcmp("periodic_table",ret)){
      const char * name,*symbol;
      char protons[32],nucleons[32],mass[32],electroneg[32];
      char * ptr=console_buf;
      int res=periodic_table(name,symbol,protons,nucleons,mass,electroneg);
      if (!res)
        return 0;
      if (res & 1)
        ptr=strcpy(ptr,name)+strlen(ptr);
      if (res & 2){
        if (res & 1)
          ptr=strcpy(ptr,",")+strlen(ptr);
        ptr=strcpy(ptr,symbol)+strlen(ptr);
      }
      if (res & 4){
        if (res&3)
          ptr=strcpy(ptr,",")+strlen(ptr);
        ptr=strcpy(ptr,protons)+strlen(ptr);
      }
      if (res & 8){
        if (res&7)
          ptr=strcpy(ptr,",")+strlen(ptr);
        ptr=strcpy(ptr,nucleons)+strlen(ptr);
      }
      if (res & 16){
        if (res&15)
          ptr=strcpy(ptr,",")+strlen(ptr);
        ptr=strcpy(ptr,mass+2)+strlen(ptr);
      }
      if (res & 32){
        if (res&31)
          ptr=strcpy(ptr,",")+strlen(ptr);
        ptr=strcpy(ptr,electroneg+4)+strlen(ptr);
      }
    }
#endif
    else
      strcpy(console_buf,ret);
    return console_buf;
  }
  return 0;
}

char *Console_Make_Entry(const Char* str)
{
  char* entry = NULL;
  entry = (char*)calloc((strlen((const char *)str)+1), sizeof(Char*));
  if(entry) memcpy(entry, (const char *)str, strlen((const char *)str)+1);

  return entry;
}


//Draws and runs the asked for menu.
const char * Console_Draw_FMenu(int key, struct FMenu* menu,Char * cfg,int active_app)
{
  int i, nb_entries = 0, selector = 0, position_number, position_x, ret, longest = 0;
  unsigned int input_key;
  char quick[] = "*: ";
  int quick_len = 2;
  char **entries;
  DISPBOX box,box3;
  position_number = (key - KEY_CTRL_F1) % 5;
  
  if (position_number<0 || position_number>5)
    position_number=4;
    
  entries  = menu->str;
  nb_entries = menu->count;
  //dbg_printf("fmenu key=%i position_number=%i menu=%s\n",key,position_number,menu->str);
    
  for(i=0; i<nb_entries; i++)
    if(strlen(entries[i]) > longest) longest = strlen(entries[i]);

  // screen resolution Graph90 384x(216-24), Graph35 128x64
  // factor 3x3
  position_x = 21*position_number;
  if(position_x + longest*4 + quick_len*4 > 115) position_x = 115 - longest*4 - quick_len*4;
  if (position_x<=2)
    position_x=2;
    
  box.left = position_x;
  box.right = position_x + longest*3 + quick_len*3  + 6;
  box.bottom = 69;
  box.top = 69-nb_entries*7;
  box3.left=3*box.left;
  box3.right=3*position_x + longest*8 + quick_len*9  + 12;//3*box.right;
  box3.bottom=3*box.bottom+STATUS_AREA_PX;
  box3.top=3*box.top+STATUS_AREA_PX;
  
  drawRectangle(box3.left,box3.top,box3.right-box3.left,box3.bottom-box3.top,COLOR_WHITE);
  draw_line(box3.left, box3.top, box3.right, box3.top,COLOR_BLACK);
  draw_line(box3.left, box3.bottom, box3.left, box3.top,COLOR_BLACK);
  draw_line(box3.right, box3.bottom, box3.right, box3.top,COLOR_BLACK);
#ifdef CURSOR    
  Cursor_SetFlashOff();
#endif
  for (;;){
    for(i=0; i<nb_entries; i++) {
      quick[0] = '0'+(i+1);
      PrintMini(3+position_x, box.bottom-7*(i+1)+2, quick, 0);
      PrintMini(3+position_x+quick_len*4, box.bottom-7*(i+1)+2, entries[i], 0);
    }
    PrintMini(3+position_x+quick_len*4,box.bottom-7*(selector+1)+2, entries[selector], 4);
    ck_getkey((int *)&input_key);
    if (input_key == KEY_CTRL_EXIT || input_key==KEY_CTRL_AC) return 0;
    if (input_key == KEY_CTRL_UP && selector < nb_entries-1) selector++;	
    if (input_key == KEY_CTRL_DOWN && selector > 0) selector--;
      
    if (input_key == KEY_CTRL_EXE) return entries[selector];
      
    if (input_key >= KEY_CHAR_1 && input_key < KEY_CHAR_1 + nb_entries) return entries[input_key-KEY_CHAR_1];
      
    input_key=translate_fkey(input_key);
      
    if ( active_app==0 &&
	 ((input_key >= KEY_CTRL_F1 && input_key <= KEY_CTRL_F6) ||
	  (input_key >= KEY_CTRL_F7 && input_key <= KEY_CTRL_F12) )
	 ){
      Console_Disp(1);
      key=input_key;
      return console_menu(key,cfg,active_app);
    }
  } // end while input_key!=EXE/EXIT

  return 0; // never reached
}


int Console_Init()
{
  console_changed=1;
  int i;
  if (!Line){
    Line=new line[LINE_MAX];
    for (i = 0; i < LINE_MAX; i++){
      Line[i].str=0;
    }
  }
	
  Start_Line = 0;
  Last_Line = 0;

  for (i = 0; i < LINE_MAX; i++)
    {
      if(Line[i].str){
	free(Line[i].str);
	Line[i].str=0;
      }
      Line[i].readonly = 0;
      Line[i].type = LINE_TYPE_INPUT;
      Line[i].start_col = 0;
      Line[i].disp_len = 0;
    }

  if ((Edit_Line = (Char *)malloc(EDIT_LINE_MAX + 1)) == NULL) return CONSOLE_MEM_ERR;
  Edit_Line[0]=0;
  Line[0].str = Edit_Line;

  Cursor.x = 0;
  Cursor.y = 0;

  Case = LOWER_CASE;

  /*for(i = 0; i < 6; i++) {
    FMenu_entries[i].name = NULL;
    FMenu_entries[i].count = 0;
    }*/

  Console_FMenu_Init();

  return CONSOLE_SUCCEEDED;
}

#ifdef WITH_PLOT

#ifdef FRANCAIS
  const char conf_standard[] = "F1 algebre\nsimplify(\nfactor(\npartfrac(\ntcollect(\ntexpand(\nsum(\noo\nproduct(\nF2  calcul\n'\ndiff(\nintegrate(\nlimit(\ntabvar(\nseries(\nsolve(\ndesolve(\n=\nF3   graphes  \nplot(\nplotseq(\nplotlist(\nplotparam(\nplotpolar(\nplotfield(\nhistogram(\nbarplot(\nF5 menu\nreserved\nF6  arithm. \nirem(\n mod \nifactor(\ngcd(\nisprime(\nnextprime(\npowmod(\niegcd(\nF7   reels\nexact(\napprox(\nfloor(\nceil(\nround(\nsign(\nmax(\nmin(\nF8 complexes\nabs(\narg(\nre(\nim(\nconj(\ncsolve(\ncfactor(\ncpartfrac(\nF9   polynomes\nproot(\npcoeff(\nquo(\nrem(\ngcd(\negcd(\nresultant(\nF:   alg. lin.\nmatrix(\ndet(\nmatpow(\nranm(\nrref(\ntran(\negvl(\negv(\nF;  misc\n_\nperiodic_table\n!\nrand()\nrandint(\nbinomial(\n and \n or \nF>   draw\ndraw_pixel(\ndraw_line(\ndraw_rectangle(\ndraw_polygon(\ndraw_circle(\ndraw_string(\nget_pixel(\nclearscreen();\nF?   colors\nred\ngreen\nblue\nmagenta\ncyan\nyellow\nwhite\nfilled\nF<   lists\nmakelist(\nrange(\nseq(\nlen(\nappend(\nranv(\nsort(\napply(\n\nF=   program\n:\n&\n#\nhex(\nbin(\nf(x):=\ndebug(\npython(\nF@ mat\n[A]\n[B]\n[C]\n[D]\n[E]\n[F]\n[G]\n[H]";
#else
  const char conf_standard[] = "F1 algebra\nsimplify(\nfactor(\npartfrac(\ntcollect(\ntexpand(\nsum(\noo\nproduct(\nF2 calculus\n'\ndiff(\nintegrate(\nlimit(\nseries(\nsolve(\ndesolve(\n=\nF3  plot  \nplot(\nplotseq(\nplotlist(\nplotparam(\nplotpolar(\nplotfield(\nhistogram(\nbarplot(\nF5 menu\nreserved\nF6  arithm. \n mod \nirem(\nifactor(\ngcd(\nisprime(\nnextprime(\npowmod(\niegcd(\nF7   reals\nexact(\napprox(\nfloor(\nceil(\nround(\nsign(\nmax(\nmin(\nF8 complexes\nabs(\narg(\nre(\nim(\nconj(\ncsolve(\ncfactor(\ncpartfrac(\nF9   polynomials\nproot(\npcoeff(\nquo(\nrem(\ngcd(\negcd(\nresultant(\nF:   lin. alg.\nmatrix(\ndet(\nmatpow(\nranm(\nrref(\ntran(\negvl(\negv(\nF;  misc\n_\nperiodic_table\n!\nrand()\nrandint(\nbinomial(\n and \n or \nF>   draw\ndraw_pixel(\ndraw_line(\ndraw_rectangle(\ndraw_polygon(\ndraw_circle(\ndraw_string(\nget_pixel(\nclearscreen();\nF?   colors\nred\ngreen\nblue\nmagenta\ncyan\nyellow\nwhite\nfilled\nF<   lists\nmakelist(\nrange(\nseq(\nlen(\nappend(\nranv(\nsort(\napply(\n\nF=   program\n:\n&\n#\nhex(\nbin(\nf(x):=\ndebug(\npython(\nF@ mat\n[A]\n[B]\n[C]\n[D]\n[E]\n[F]\n[G]\n[H]";
#endif

#else // WITH_PLOT

#ifdef FRANCAIS
  const char conf_standard[] = "F1 save,quit\nreserved\nF2 algebre\nsimplify(\nfactor(\npartfrac(\ntcollect(\ntexpand(\nsum(\noo\nproduct(\nF4 calcul\n'\ndiff(\nintegrate(\nlimit(\nseries(\nsolve(\ndesolve(\n=\nF5 menu\nreserved\nF6  arithm. \nirem(\n mod \nifactor(\ngcd(\nisprime(\nnextprime(\npowmod(\niegcd(\nF7   reels\nexact(\napprox(\nfloor(\nceil(\nround(\nsign(\nmax(\nmin(\nF8 complexes\nabs(\narg(\nre(\nim(\nconj(\ncsolve(\ncfactor(\ncpartfrac(\nF9   polynomes\nproot(\npcoeff(\nquo(\nrem(\ngcd(\negcd(\nresultant(\nF:   alg. lin.\nmatrix(\ndet(\nmatpow(\nranm(\nrref(\ntran(\negvl(\negv(\nF;  misc\n_\nperiodic_table\n!\nrand()\nrandint(\nbinomial(\n and \n or \nF>   draw\ndraw_pixel(\ndraw_line(\ndraw_rectangle(\ndraw_polygon(\ndraw_circle(\ndraw_string(\nget_pixel(\nclearscreen();\nF?   colors\nred\ngreen\nblue\nmagenta\ncyan\nyellow\nwhite\nfilled\nF<   lists\nmakelist(\nrange(\nseq(\nlen(\nappend(\nranv(\nsort(\napply(\n\nF=   program\n:\n&\n#\nhex(\nbin(\nf(x):=\ndebug(\npython(\nF@ mat\n[A]\n[B]\n[C]\n[D]\n[E]\n[F]\n[G]\n[H]";
#else
  const char conf_standard[] = "F1 save,quit\nreserved\nF2 algebra\nsimplify(\nfactor(\npartfrac(\ntcollect(\ntexpand(\nsum(\noo\nproduct(\nF4 calculus\n'\ndiff(\nintegrate(\nlimit(\nseries(\nsolve(\ndesolve(\n=\nF5 menu\nreserved\nF6  arithm. \n mod \nirem(\nifactor(\ngcd(\nisprime(\nnextprime(\npowmod(\niegcd(\nF7   reals\nexact(\napprox(\nfloor(\nceil(\nround(\nsign(\nmax(\nmin(\nF8 complexes\nabs(\narg(\nre(\nim(\nconj(\ncsolve(\ncfactor(\ncpartfrac(\nF9   polynomials\nproot(\npcoeff(\nquo(\nrem(\ngcd(\negcd(\nresultant(\nF:   lin. alg.\nmatrix(\ndet(\nmatpow(\nranm(\nrref(\ntran(\negvl(\negv(\nF;  misc\n_\nperiodic_table\n!\nrand()\nrandint(\nbinomial(\n and \n or \nF>   draw\ndraw_pixel(\ndraw_line(\ndraw_rectangle(\ndraw_polygon(\ndraw_circle(\ndraw_string(\nget_pixel(\nclearscreen();\nF?   colors\nred\ngreen\nblue\nmagenta\ncyan\nyellow\nwhite\nfilled\nF<   lists\nmakelist(\nrange(\nseq(\nlen(\nappend(\nranv(\nsort(\napply(\n\nF=   program\n:\n&\n#\nhex(\nbin(\nf(x):=\ndebug(\npython(\nF@ mat\n[A]\n[B]\n[C]\n[D]\n[E]\n[F]\n[G]\n[H]";
#endif

#endif

// const char conf_standard[] = "F1 arit\ngcd(\nlcm(\niegcd(\npowmod(\nisprime(\nnextprime(\nifactor(\nfrom arit import *\nF2 math\nfloor(\nceil(\nround(\nmin(\nmax(\nabs(\ndef f(x): return \nfrom math import *\nF3 char\nchar table\n:\n;\n_\n<\n>\n%\nimport \nF4 menu\nreserved\nF5 A<>a\nreserved\nF6 none\nF7 arit\ngcd(\nlcm(\niegcd(\npowmod(\nisprime(\nnextprime(\nifactor(\nfrom arit import *\nF8 math\nfloor(\nceil(\nround(\nmin(\nmax(\nabs(\ndef f(x): return \nfrom math import *\nF9 cmath\n.real\n.imag\nphase(\nfrom cmath import *;i=1j\nrandint(\nrandom()\nchoice(\nfrom random import *\nF: plot\naxis(\nplot(\ntext(\narrow(\nscatter(\nboxplot(\nshow()\nfrom matplotl import *\nF; draw\nclear_screen();\ndraw_pixel(\ndraw_line(\ndraw_rectangle(\ndraw_arc(\ndraw_circle(\ndraw_string(\nfrom graphic import *\nF< logo\nforward(\nbackward(\nleft(\nright(\npencolor(\ncircle(\nreset()\nfrom turtle import *\nF= numpy\narray(\nreshape(\narange(\nlinspace(\nsolve(\neig(\ninv(\nfrom numpy import *;i=1j\nF> linalg\nmatrix(\nadd(\nsub(\nmul(\ninv(\nrref(\ntranspose(\nfrom linalg import *\nF? char\nchar table\n:\n;\n_\n<\n>\n%\nimport \nF@ fill\ndraw_filled_rectangle\ndraw_filled_circle(\ndraw_filled_polygon(\ndraw_filled_arc\nfrom graphic import *\nFA color\nred\nblue\nmagenta\ncyan\ngreen\nyellow\nwhite\nblack\nFB prog\nprint(\ninput(\nhex(\nbin(\ngetkey()\nsleep()\nmonotonic()\nfrom ion import *\nFC list\nlist(\nrange(\nlen(\nappend(\nzip(\nsorted(\nmap(\nreversed(\n";


// Loads the FMenus' data into memory, from a cfg file
void update_fmenu(const Char * cfg){
  int i, number=0;
  Char temp[64] = {'\0'};
  while (*cfg){
    //Get each line
    for (i=0 ; i+1<sizeof(temp)/sizeof(char) && (*cfg && *cfg!='\r' && *cfg!='\n'); i++,cfg++){
      temp[i] = *cfg;
    }
    temp[i]=0;
    //If starting by 'F', adjust the number and eventually set the name of the menu
    if(temp[0] == 'F' && temp[1]>='1' && temp[1]<=('0'+18)) {
      number = temp[1]-'0' - 1;
      if(temp[3]) {
	strncpy(FMenu_entries_name[number], (char*)temp+3,maxfmenusize);
	//FMenu_entries[number].name[4] = '\0';
      }
    }

    memset(temp, '\0', 20);
    cfg++;
  }
  //free(original_cfg);
}

#ifndef FX

#ifndef CURSOR
#ifdef TICE
int print_x=0,print_y=0,vfontsize=15,hfontsize=8;
#else
int print_x=0,print_y=0,vfontsize=18,hfontsize=12;
#endif 
#endif

void locate(int x,int y){
#ifdef CURSOR
  return locate_OS(x,y);
#else 
  print_x=(x-1)*hfontsize;
  print_y=(y-1)*vfontsize;
#endif
}

void Cursor_SetPosition(int x,int y){
  return locate(x+1,y+1);
}

void print(int &X,int&Y,const char * buf,int color,bool revert,bool fake,bool minimini){
#if defined FX || defined FXCG
  if (minimini) 
     PrintMiniMini( &X, &Y, (Char *)buf, revert?4:0, color, fake?1:0 );
  else 
    PrintMini(&X, &Y, (Char *)buf, revert?4:0, 0xFFFFFFFF, 0, 0, color, COLOR_WHITE, fake?0:1, 0);
#else
  if(minimini) 
    X=os_draw_string_small(X,Y,color,revert?COLOR_SELECTED:COLOR_WHITE,buf,fake);
  else 
    X=os_draw_string_medium(X,Y+1,color,revert?COLOR_SELECTED:COLOR_WHITE,buf,fake);
#endif
}

void PrintRev(const Char * s,int color=TEXT_COLOR_BLACK){
#ifdef CURSOR
  Print_OS((Char *)s,TEXT_MODE_INVERT,0);
#else
  print(print_x,print_y,(const char *)s,color,true/* revert*/,false,false);
#endif  
}

  bool tooltip(int x,int y,int pos,const char * editline){
    char cmdline[strlen(editline)+1];
    strcpy(cmdline,editline);
    cmdline[pos]=0;
    int l=strlen(cmdline);
    char buf[l+1];
    strcpy(buf,cmdline);
    bool openpar=l && buf[l-1]=='(';
    if (openpar){
      buf[l-1]=0;
      --l;
    }
    for (;l>0;--l){
      if (!isalphanum(buf[l-1]) && buf[l-1]!='_')
	break;
    }
    // cmdname in buf+l
    const char * cmdname=buf+l,*cmdnameorig=cmdname;
    int l1=strlen(cmdname);
    if (l1<2)
      return false;
    const char * howto=0,*syntax=0,*related=0,*examples=0;
    if (l1>0
#if 1 //def BW
        && 0
#else
        && has_static_help(cmdname,lang | 0x100,howto,syntax,related,examples)
#endif
        && examples){
      // display tooltip
      if (x<0)
	x=os_draw_string(0,y,_BLACK,1234,editline,true); // fake print -> x position // replaced cmdline by editline so that tooltip is at end
      x+=2;
      y+=4;
      drawRectangle(x,y,6,10,65529);
      draw_line(x,y,x+6,y,SDK_BLACK);
      draw_line(x,y,x+3,y+3,SDK_BLACK);
      draw_line(x+6,y,x+3,y+3,SDK_BLACK);
      y-=4;
      x+=7;
      int bg=65529; // background
      x=os_draw_string_small(x,y,_BLACK,bg,": ",false);
      if (howto && strlen(howto)){
#ifdef NSPIRE_NEWLIB
	y-=2;
#endif
	os_draw_string_small(x,y,_BLACK,bg,
#ifdef NUMWORKS
			     remove_accents(howto).c_str(),
#else
			     howto,
#endif
			     false);
#ifdef NSPIRE_NEWLIB
	y+=12;
#else
	y+=11;
#endif
      }
      string toolt;
      if (related && strlen(related)){
	toolt += cmdname;
	toolt += '(';
	if (syntax && strlen(syntax))
	  toolt += syntax;
	else
	  toolt += "arg";
	toolt += ')';
	toolt += ' ';
	if (related)
	  toolt += related;
      }
      else
	toolt+=examples;
      os_draw_string_small(x,y,_BLACK,bg,toolt.c_str(),false);
      return true;
    }
    return false;
  }

  int print_color(int print_x,int print_y,const char *s,int color,bool invert,bool minimini){
    int python=4;
    const char * src=s;
    // char * singleword=(char *) malloc(256); if (!singleword) return print_x;
    char singleword[256];
    bool linecomment=false;
    int couleur=color;
    while (*src && print_y<LCD_WIDTH_PX){
      singleword[0]=0;
      const char * oldsrc=src;
      if ( (python && *src=='#') ||
	   (!python && *src=='/' && *(src+1)=='/')){
	linecomment=true;
	couleur=4;
      }
      //dbg_printf("print_color loop1 %s\n",src);
      if (linecomment){
	src = (char*)toksplit((Char*)src, ' ', (Char*)singleword, minimini?50:35); //break into words; next word
        //dbg_printf("print_color comment %s single=%s\n",src,singleword);
      }
      else { // skip string (only with delimiters " ")
	if (*src=='"'){
	  for (++src;*src;++src){
	    if (*src=='"' && *(src-1)!='\\')
	      break;
	  }
	  if (*src=='"')
	    ++src;
	  int i=src-oldsrc;
	  strncpy(singleword,oldsrc,i);
	  singleword[i]=0;
          //dbg_printf("print_color loop1a %s single=%s\n",src,singleword);
	}
	else {
	  size_t i=0;
	  for (;*src==' ';++src){ // skip initial whitespaces
	    ++i;
	  }
	  if (i==0){
	    if (isalpha(*src)){ // skip keyword
	      for (;isalphanum(*src) || *src=='_';++src){
		++i;
	      }
	    }
	    // go to next space or alphabetic char
	    for (;*src;++i,++src){
	      if (*src==' ' || (i && *src>=' ' && *src<='/') || (python && *src=='#') || (!python && *src=='/' && *(src+1)=='/')|| *src=='"' || isalpha(*src))
		break;
	    }
	  }
	  strncpy(singleword,oldsrc,i);
	  singleword[i]=0;
          //dbg_printf("print_color loop2a %i %s single=%s\n",i,src,singleword);
	  if (i==0){
            //dbg_printf("print_color loop2b %i %s\n",i,src);
            print(print_x,print_y,src,color,invert,/*fake*/false,minimini);
            //dbg_printf("print_color loop2c %i %s\n",i,src);
	    return print_x; // FIXME KEY_CTRL_F2;
	  }
	} // end normal case
      } // end else linecomment case
      couleur=linecomment?5:find_color(singleword);
      //dbg_printf("print_color loop3 singleworld=%s couleur=%i\n",couleur);
      if (couleur==1) couleur=COLOR_BLUE;
      if (couleur==2) couleur=49432; //was COLOR_YELLOWDARK;
      if (couleur==3) couleur=51712;//33024;
      if (couleur==4) couleur=COLOR_MAGENTA;
      if (couleur==5) couleur=COLOR_GREEN;
      if (linecomment || singleword[0]=='"'){
        //dbg_printf("print_color loop4 couleur=%i\n",couleur);
	print(print_x,print_y,singleword,couleur,invert,/*fake*/false,minimini);
      }
      else { // print two parts, commandname in color and remain in black
	char * ptr=singleword;
	if (isalpha(*ptr)){
	  while (isalphanum(*ptr) || *ptr=='_')
	    ++ptr;
	}
	char ch=*ptr;
	*ptr=0;
        //dbg_printf("print_color loop5 couleur=%i\n",couleur);
        print(print_x,print_y,singleword,couleur,invert,/*fake*/false,minimini);
	*ptr=ch;
        //dbg_printf("print_color loop6 couleur=%i\n",couleur);
	print(print_x,print_y,ptr,COLOR_BLACK,invert,/*fake*/false,minimini);
      }
      // ?add a space removed from token
      if( linecomment?*src:*src==' ' ){
	if (*src==' ')
	  ++src;
        //dbg_printf("print_color loop7\n");
	print(print_x,print_y," ",COLOR_BLACK,invert,false,minimini);
      }
    }
    //dbg_printf("print_color return\n");
    return print_x;
  }    

  void print_color(const char *s,int color,bool invert,bool minimini){
    print_x=print_color(print_x,print_y,s,color,invert,minimini);
  }

void Print(const Char * s,int color,bool colorsyntax){
#ifdef CURSOR
  Print_OS((Char *)s,TEXT_MODE_NORMAL,0);
#else
  if (//1 ||
      !colorsyntax ||
      (strlen((const char *)s)==1 && (s[0]=='>' || s[0]=='<')))
      print(print_x,print_y,(const char *)s,color,false,false,false);
    else
      print_color((const char *)s,color,false,false);
#endif
}


void console_displine(int i,int redraw_mode){
  int print_y=i*vfontsize;
  bool minimini=false;
  //dbg_printf("ConsoleDisp loop i=%i\n",i);
  line & curline=Line[i+Start_Line];
  bool colorsyntax=curline.type == LINE_TYPE_INPUT;
  if (i == Cursor.y){
    //dbg_printf("ConsoleDisp loop i=Cursor.y %i\n",i);
    // cursor line
    if ((redraw_mode & 1)==0)
      drawRectangle(0,i*vfontsize+STATUS_AREA_PX,LCD_WIDTH_PX,vfontsize,_WHITE);
    if (curline.type == LINE_TYPE_INPUT || (curline.type == LINE_TYPE_OUTPUT && curline.disp_len >= COL_DISP_MAX)){
      locate(1, i + 1);
      if (curline.readonly){
        PrintRev(curline.str + curline.start_col);
      }
      else 
        Print(curline.str+curline.start_col+(Cursor.x>COL_DISP_MAX-1?1:0),TEXT_COLOR_BLACK,colorsyntax);
    }
    else {
      locate(COL_DISP_MAX - curline.disp_len + 1, i + 1);
      if (curline.readonly){
#ifdef CURSOR
        Cursor_SetFlashOff();
#endif
        PrintRev(curline.str);
      }
      else 
        Print(curline.str,TEXT_COLOR_BLACK,colorsyntax);
    }
    //dbg_printf("ConsoleDisp loop1 i=Cursor.y %i\n",i);
      
    if (curline.disp_len - curline.start_col > COL_DISP_MAX-1){
      // draw arrow indicating there is more
      print_x=LCD_WIDTH_PX-hfontsize;
      if (curline.readonly){
        if(curline.disp_len - curline.start_col != COL_DISP_MAX) {
          PrintRev((Char *)">",COLOR_MAGENTA);
        }
      } // if cur.readonly
      else if (Cursor.x < COL_DISP_MAX-1){
        Print((Char *)">",COLOR_MAGENTA,colorsyntax);
      }
    }
    //dbg_printf("ConsoleDisp loop2 i=Cursor.y %i\n",i);
      
    if (curline.start_col > 0){
      locate(1, i + 1);	
      if (curline.readonly){
#ifdef CURSOR
        Cursor_SetFlashOff();
#endif		  
        PrintRev((Char *)"<",COLOR_MAGENTA);
      }
      else {
        Print((Char *)"<",COLOR_MAGENTA,colorsyntax);
      }
    }
    //dbg_printf("ConsoleDisp loop3 i=Cursor.y %i\n",i);
      
    if (!curline.readonly){
      int fakestart=curline.start_col+(Cursor.x > COL_DISP_MAX-1?1:0);
      int fakex,fakey=Cursor.y*vfontsize;
      string fakes;
      // parenthese match
      const char * str=(const char *) curline.str;
      int pos=Cursor.x+fakestart,pos2;
      int l=strlen(str);
      char ch=0;
      if (pos<l)
        ch=str[pos];
      int matchdirection=0,paren=0,crochet=0,accolade=0;
      if (ch=='(' || ch=='[' || ch=='{')
        matchdirection=1;
      if (ch=='}' || ch==']' || ch==')')
        matchdirection=-1;
      if (!matchdirection && pos){
        --pos;
        ch=str[pos];
        if (ch=='(' || ch=='[' || ch=='{')
          matchdirection=1;
        if (ch=='}' || ch==']' || ch==')')
          matchdirection=-1;
      }
      if (matchdirection){
        char buf[2]={0,0};
        bool ok=true;
        for (pos2=pos;ok && (pos2>=0 && pos2<l);pos2+=matchdirection){
          ch=str[pos2];
          if (ch=='(') ++paren;
          if (ch==')') --paren;
          if (ch=='[') ++crochet;
          if (ch==']') --crochet;
          if (ch=='{') ++accolade;
          if (ch=='}') --accolade;
          if (matchdirection>0 && (paren<0 || crochet<0 || accolade<0) )
            ok=false;
          if (matchdirection<0 && (paren>0 || crochet>0 || accolade>0) )
            ok=false;
          if (paren==0 && crochet==0 && accolade==0)
            break;
        }
        ok = paren==0 && crochet==0 && accolade==0;
        if (pos>=fakestart){
          fakex=0;
          buf[0]=str[pos];
          fakes=string((const char *)curline.str).substr(fakestart,pos-fakestart);
          print(fakex,fakey,fakes.c_str(),TEXT_COLOR_BLACK,false,true/* fake*/,minimini); // fake print
          print(fakex,fakey,buf,ok?COLOR_GREEN:COLOR_RED,true/* revert*/,false,minimini);
        }
        if (ok){
          fakex=0;
          if (pos2>fakestart){
            fakes=string((const char *)curline.str).substr(fakestart,pos2-fakestart);
            print(fakex,fakey,fakes.c_str(),TEXT_COLOR_BLACK,false,true/* fake*/,false); // fake print
            buf[0]=str[pos2];
            print(fakex,fakey,buf,COLOR_GREEN,true/* revert*/,false,minimini);
          }
        } // end if ok
      } // end if matchdirection
      //dbg_printf("ConsoleDisp loop4 i=Cursor.y %i\n",i);
      //locate(Cursor.x+1,Cursor.y+1);
      //DefineStatusMessage((giac::print_DOUBLE_(Cursor.y,6)+","+giac::print_DOUBLE_(print_y,6)).c_str(),1,0,0);
      //DisplayStatusArea();
      fakes=string((const char *)curline.str).substr(fakestart,Cursor.x);
      fakex=0;fakey=Cursor.y*vfontsize;
      print(fakex,fakey,fakes.c_str(),TEXT_COLOR_BLACK,false,true/* fake*/,false);
      drawRectangle(fakex,STATUS_AREA_PX+fakey,2,vfontsize,COLOR_BLACK);
      //drawRectangle(Cursor.x*hfontsize,24+Cursor.y*vfontsize,2,vfontsize,COLOR_BLACK);
    }
  } // end cursor line
  else {
    //dbg_printf("ConsoleDisp loop i!=Cursor.y %i\n",i);
    if (redraw_mode==0)
      return;
    else
      if ((redraw_mode & 1)==0)
        drawRectangle(0,i*vfontsize+STATUS_AREA_PX,LCD_WIDTH_PX,vfontsize,_WHITE);
    bool bigoutput = curline.type==LINE_TYPE_OUTPUT && curline.disp_len>=COL_DISP_MAX-3;
    locate(bigoutput?3:1,i+1);
    if (curline.type==LINE_TYPE_INPUT || bigoutput){
      //dbg_printf("ConsoleDisp loop1 i!=Cursor.y %i\n",i);
      Print(curline.str + curline.start_col,TEXT_COLOR_BLACK,colorsyntax);
    }
    else {
#ifdef CURSOR
      locate(COL_DISP_MAX - Line[i + Start_Line].disp_len + 1, i + 1);
#else
      //dbg_printf("ConsoleDisp loopa i!=Cursor.y %i\n",i);
      print(print_x,print_y,(const char *)curline.str,TEXT_COLOR_BLACK,false,true/*fake*/,false);
      //dbg_printf("ConsoleDisp loopb i!=Cursor.y %i\n",i);
      print_x=LCD_WIDTH_PX-print_x;
#endif
      //dbg_printf("ConsoleDisp loopc i!=Cursor.y %i\n",i);
      Print(curline.str,TEXT_COLOR_BLACK,colorsyntax);
    }
    //dbg_printf("ConsoleDisp loop2 i!=Cursor.y %i\n",i);
    if (curline.disp_len - curline.start_col > COL_DISP_MAX){
#ifdef CURSOR
      locate(COL_DISP_MAX, i + 1);
#else
      print_x=LCD_WIDTH_PX-hfontsize;
#endif
      Print((Char *)">",COLOR_BLUE,colorsyntax);
    }
    //dbg_printf("ConsoleDisp loop3 i!=Cursor.y %i\n",i);
    if (curline.start_col > 0){
#ifdef CURSOR
      locate(1, i + 1);
#else
      print_x=0;
#endif
      Print((Char *)">",COLOR_BLUE,colorsyntax);
    }      
    //dbg_printf("ConsoleDisp loop4 i!=Cursor.y %i\n",i);
  } // end non cursor line
}  

// redraw_mode bit0=1 means redraw all
int Console_Disp(int redraw_mode){
  if (redraw_mode & 1)
    os_fill_rect(0,STATUS_AREA_PX,LCD_WIDTH_PX,LCD_HEIGHT_PX-STATUS_AREA_PX,SDK_WHITE); // Bdisp_AllClr_VRAM();
  //dbg_printf("ConsoleDisp\n");
  //Reading each "line" that will be printed
  for (int i = 0; (i < LINE_DISP_MAX) && (i + Start_Line <= Last_Line); i++){
    console_displine(i,redraw_mode);
  } // end loop on all lines

  return CONSOLE_SUCCEEDED;
}

#endif

void Console_FMenu_Init()
{
  int i, number=0, key, handle;
  Char* tmp_realloc = NULL;
  Char temp[20] = {'\0'};
#if 0
  if (!original_cfg){
#if 1
    string s;
    if (load_script("MENU.py",s)>0){
      original_cfg=(Char *)malloc(s.size()+1);
      strcpy((char *)original_cfg,s.c_str());
    }
#else
    original_cfg = (Char *)memory_load((char *)"\\\\fls0\\MENU.py");
#endif
    // Does the file exists ?
    // Todo : check the error codes...
    if(!original_cfg) {
#if 1
      save_script((const char *)"MENU.py",conf_standard);
#else
      memory_createfile((char *)"\\\\fls0\\MENU.py", strlen((char*)conf_standard)+1);
      handle = memory_openfile((char *)"\\\\fls0\\MENU.py", _OPENMODE_READWRITE);
      memory_writefile(handle, conf_standard, strlen((char*)conf_standard)+1);
      memory_closefile(handle);
#endif
      original_cfg = (Char *)conf_standard;
    }
  }
#else
  original_cfg = (Char *)conf_standard;
#endif
  Char* cfg=original_cfg;
  update_fmenu(cfg);
}


void dConsoleRedraw(){
  Console_Disp(1);
}

/*
  Draw a popup at the center of the screen containing the str expression drawn in pretty print.
*/
#if defined POPUP_PRETTY && defined TEX
void Console_Draw_TeX_Popup(Char* str, int width, int height)
{
  DISPBOX popup;
  DISPBOX temp;
  Char arrows[4*3] = {0xE6, 0x9A, '\0', 0xE6, 0x9B, '\0', 0xE6, 0x9C, '\0', 0xE6, 0x9D, '\0'};
  int margin = 2, border = 1;
  int scroll_lateral = 0, scroll_lateral_flag = 0, scroll_vertical = 0, scroll_vertical_flag = 0;
  int key;

  if(width > 115) {
    popup.left = 5;
    popup.right = 122;

    scroll_lateral_flag = 1;
  }
  else {
    popup.left = 64 - width/2 - margin - border;
    popup.right = 128 - popup.left;
  }

  if(height > 50) {
    popup.top = 5;
    popup.bottom = 57;

    scroll_vertical_flag = 1;
  }
  else {
    popup.top = 32 - height/2 - margin - border;
    popup.bottom = 64 - popup.top;
  }

  /*temp.left = 0; temp.top = 0; temp.right = 128; temp.bottom = 64;
    Bdisp_ReadArea_VRAM (&temp, vram_copy);*/
	
  while(key != KEY_CTRL_EXIT) {
    Bdisp_AreaClr_VRAM(&popup);
    Bdisp_AreaReverseVRAM(popup.left, popup.top, popup.right, popup.bottom);

    Bdisp_AreaReverseVRAM(popup.left + border, popup.top + border, popup.right - border, popup.bottom - border);

    TeX_drawComplex((char*)str, popup.left+border+margin + scroll_lateral, popup.top+border+margin + scroll_vertical); 

    if(scroll_lateral_flag ||scroll_vertical_flag) {
      temp.left = 0; temp.top = 0; temp.right = popup.left-1; temp.bottom = popup.bottom;
      Bdisp_AreaClr_VRAM(&temp);
      temp.left = 0; temp.top = popup.bottom+1; temp.right = 127; temp.bottom = 63;
      Bdisp_AreaClr_VRAM(&temp);
      temp.left = popup.left-1; temp.top = 0; temp.right = 127; temp.bottom = popup.top-1;
      Bdisp_AreaClr_VRAM(&temp);
      temp.left = popup.right+1; temp.top = popup.top-1; temp.right = 127; temp.bottom = 63;
      Bdisp_AreaClr_VRAM(&temp);

      if(scroll_lateral < 0) Printmini(1, 30, arrows, 0);
      if(scroll_lateral > -(width - 115)) Printmini(123, 30, arrows + 3, 0);
      if(scroll_vertical < 0) Printmini(61, 0, arrows + 6, 0);
      if(scroll_vertical > -(height - 47)) Printmini(61, 58, arrows + 9, 0);

      Bdisp_DrawLineVRAM(popup.left, popup.top, popup.left, popup.bottom);
      Bdisp_DrawLineVRAM(popup.left, popup.top, popup.right, popup.top);
      Bdisp_DrawLineVRAM(popup.left, popup.bottom, popup.right, popup.bottom);
      Bdisp_DrawLineVRAM(popup.right, popup.top, popup.right, popup.bottom);
    }

    ck_getkey(&key);

    if(scroll_lateral_flag) {
      if(key == KEY_CTRL_LEFT && scroll_lateral < 0) scroll_lateral += 5;
      if(key == KEY_CTRL_RIGHT && scroll_lateral > -(width - 115)) scroll_lateral -= 5;

      if(scroll_lateral > 0) scroll_lateral = 0;
    } 
    if(scroll_vertical_flag) {
      if(key == KEY_CTRL_UP && scroll_vertical < 0) scroll_vertical += 3;
      if(key == KEY_CTRL_DOWN && scroll_vertical > -(height - 47)) scroll_vertical -= 3;

      if(scroll_vertical > 0) scroll_vertical = 0;
    }
  }
}
#endif

/*
  ÒÔÏÂº¯ÊýÓÃÓÚÊäÈëÐÐ£¬³É¹¦ºó½«·µ»Ø¸ÃÐÐµÄ×Ö·û´®¡£
*/

const Char *Console_GetLine()
{
  int return_val;
	
  do
    {
      return_val = Console_GetKey();
      Console_Disp(1);
      if (return_val == CONSOLE_MEM_ERR) return NULL;
      if (return_val == -2) return "kill";
    } while (return_val != CONSOLE_NEW_LINE_SET);

  return Line[Current_Line - 1].str;
}

/*
  Simple accessor to the Edit_Line buffer.
*/
Char* Console_GetEditLine()
{
  return Edit_Line;
}
