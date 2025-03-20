//#define DBG 0
#define WITH_LCDMALLOC 1
#define KHICAS_STACK 1

#include <string>
#include <stdlib.h>
#include <giac/giacPCH.h>
#include <giac/input_parser.h>
//#include "gmp_replacements.h"
#include "calc.h"
#ifdef TICE
#include <sys/lcd.h>
#include <ti/vars.h>
#include <ti/info.h>
#endif

#include "console.h"
//#include "menu_config.h"
#include "menuGUI.h"
#include "textGUI.h"
#include "file.h"
#include "main.h"
#include <giac/kdisplay.h>
#if defined TICE && !defined std
#define std ustl
#endif
using namespace std;
#ifdef NSPIRE_NEWLIB
#include "os.h"
#endif

#define EXPR_BUF_SIZE 256
#define GIAC_HISTORY_SIZE 2
#define GIAC_HISTORY_MAX_TAILLE 32

bool freeze=false,freezeturtle=false;
size_t pythonjs_stack_size=20*1024,pythonjs_heap_size=max_heap_size*3*1024/4;
int xcas_python_eval=0;
char * pythonjs_static_heap=0;
char * python_heap=0;
giac::context * contextptr=0;
extern "C" int mp_token(const char * line);
extern "C" {
#ifdef FXCG
#include <fxcg/rtc.h>
#endif
  extern int execution_in_progress_py;
  extern volatile int ctrl_c_py;
  //void set_abort_py();
  //void clear_abort_py();
}

#ifdef WITH_SHEET
void sheet(){
  xcas::sheet(contextptr);
}
#endif

void python_free(){
  if (!python_heap) return;
  mp_deinit();
  if (!pythonjs_static_heap){
    if ( ((size_t) python_heap)>1)
      free(python_heap);
  }
  python_heap=0;
}

int python_init(int stack_size,int heap_size){
  python_free();
  // python_heap=micropy_init(8192,16384);
  //string s(("Heap "+print_INT_(heap_size/1024)+"K MicroPython not available"));
  //Console_Output((const Char *)s.c_str());
  //os_draw_string_medium(0,180,COLOR_BLACK,COLOR_WHITE,s.c_str(),false);
  //dConsolePut(s.c_str());
  //Console_NewLine(LINE_TYPE_OUTPUT,1);
  Console_Disp(1);
  python_heap=micropy_init(stack_size,heap_size);
  //confirm("heap","init");
  if (!python_heap)
    return 0;
  return 1;
}

int micropy_ck_eval(const char *line){
  freeze=false; freezeturtle=false;
  if (python_heap && line[0]==0)
    return 1;
  if (!python_heap){
    python_init(pythonjs_stack_size,pythonjs_heap_size);
  }
  if (!python_heap){
    return RAND_MAX;
  }
  ctrl_c_py=0;
  execution_in_progress_py = 1;
  int res= micropy_eval(line);
  execution_in_progress_py = 0;
  if (ctrl_c_py & 1){
    confirm(lang?"Interrompu":"Interrupted","F1/F5: ok",""); // insure ON has been removed from keyboard buffer
  }  
  //while (1) { int key; ck_getkey(&key); if (key==KEY_CTRL_EXIT) break; }
  return res;
}

#if defined WITH_EQW && !defined FAKE_GIAC

const char * input_matrix(const giac::gen &g,giac::gen & ge,const giac::context *){
  if (ge.type==giac::_VECT)
    ge.subtype=0;
  static string input_matrix_s=g.print(contextptr)+'='+ge.print(contextptr);
  return input_matrix_s.c_str();
}    
  
const char * input_matrix(bool list){
  static ustl::string * sptr=0;
  if (!sptr)
    sptr=new ustl::string;
  *sptr="";
  giac::gen v(giac::_VARS(0,contextptr));
  giac::vecteur w;
  if (v.type==giac::_VECT){
    for (size_t i=0;i<v._VECTptr->size();++i){
      giac::gen & tmp = (*v._VECTptr)[i];
      if (tmp.type==giac::_IDNT){
	giac::gen tmpe(giac::eval(tmp,1,contextptr));
	if (list){
	  if (tmpe.type==giac::_VECT && !giac::ckmatrix(tmpe))
	    w.push_back(tmp);
	}
	else {
	  if (ckmatrix(tmpe))
	    w.push_back(tmp);
	}
      }
    }
  }
  ustl::string msg;
  if (w.empty())
    msg=lang?"Creer nouveau":"Create new";
  else
    msg=((lang?"Creer nouveau ou editer ":"Create new or edit ")+(w.size()==1?w.front():giac::gen(w,giac::_SEQ__VECT)).print(contextptr));
  lock_alpha();
  if (inputline(msg.c_str(),(lang?"Nom de variable:":"Variable name:"),*sptr,false) && !sptr->empty() && isalpha((*sptr)[0])){
    reset_kbd();
    giac::gen g(*sptr,contextptr);
    giac::gen ge(eval(g,1,contextptr));
    if (g.type==giac::_IDNT){
      if (ge.type==giac::_VECT){
	ge=xcas::eqw(ge,true);
	ge=giac::eval(ge,1,contextptr);
        freeze=giac::ctrl_c=giac::kbd_interrupted=giac::interrupted=false;
	return input_matrix(g,ge,contextptr);
      }
      if (ge==g || confirm_overwrite()){
	*sptr="";
	if (inputline((lang?"Nombre de lignes":"Line number"),"",*sptr,true)){
	  int l=strtol(sptr->c_str(),0,10);
	  if (l>0 && l<256){
	    int c;
	    if (list)
	      c=0;
	    else {
	      ustl::string tmp(*sptr+(lang?" lignes.":" lines."));
	      *sptr="";
	      inputline(tmp.c_str(),lang?"Colonnes:":"Columns:",*sptr,true);
	      c=strtol(sptr->c_str(),0,10);
	    }
	    if (c==0){
	      ge=giac::vecteur(l);
	    }
	    else {
	      if (c>0 && l*c<256)
		ge=giac::_matrix(giac::makesequence(l,c),contextptr);
	    }
	    ge=xcas::eqw(ge,true);
	    ge=giac::eval(ge,1,contextptr);
            freeze=giac::ctrl_c=giac::kbd_interrupted=giac::interrupted=false;
            reset_kbd();
	    if (ge.type==giac::_VECT)
	      return input_matrix(g,ge,contextptr);
	    return "";
	  } // l<256
	}
      } // ge==g || overwrite confirmed
    } // g.type==_IDNT
    else {
      invalid_varname();
    }	
  } // isalpha
  reset_kbd();
  return 0;
}
#else
const char * input_matrix(bool list){
  return 0;
}
#endif

int select_item(const char ** ptr,const char * title,bool askfor1){
  int nitems=0;
  for (const char ** p=ptr;*p;++p)
    ++nitems;
  if (nitems==0 || nitems>=256)
    return -1;
  if (!askfor1 && nitems==1)
    return 0;
  MenuItem smallmenuitems[nitems];
  for (int i=0;i<nitems;++i){
    smallmenuitems[i].text=(char *) ptr[i];
  }
  Menu smallmenu;
  smallmenu.numitems=nitems; 
  smallmenu.items=smallmenuitems;
  smallmenu.height=nitems>=11?12:nitems+1;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  smallmenu.title = (char*) title;
  //MsgBoxPush(5);
  int sres = doMenu(&smallmenu);
  //MsgBoxPop();
  if (sres!=MENU_RETURN_SELECTION && sres!=KEY_CTRL_EXE)
    return -1;
  return smallmenu.selection-1;
}

int fileBrowser(char * filename, const char * ext, const char * title){
  const int N=32;
  const char * filenames[N]={0};
  //dbg_printf("fileBrowser ext=%s title=%s\n",ext,title);
  int res=os_file_browser(filenames,N,ext,2);
  if (res<=0 || res>=N)
    return 0;
  res=select_item(filenames,title,/* ask for even if 1 file avail*/ true);
  //dbg_printf("fileBrowser %i\n",res);
  if (res<0)
    return 0;
  strcpy(filename,filenames[res]);
  //dbg_printf("fileBrowser %s\n",filename);
  return 1;
}

int process_freeze(){
  if (freezeturtle){
    displaylogo();
    freezeturtle=false;
    return 1;
  }
  if (freeze){
    freeze=false;
    int key;
    ck_getkey(&key);
    return 1;
  }
  return 0;
}    


// extern U ** mem;
// extern unsigned int **free_stack;

// #define SYMBOLSSTATEFILE (char*)"\\\\fls0\\lastvar.py"
  
static char varbuf[128];
const char * select_var(){
#ifdef FAKE_GIAC
  int freemem=(int)malloc(0xffffff);
  dbg_printf("malloc free=%i\n",freemem);
  return "";
#else
  giac::gen g(giac::_VARS(0,contextptr));
  if (g.type!=giac::_VECT)
    return "";
  giac::vecteur & v=*g._VECTptr;
  MenuItem smallmenuitems[v.size()+4];
  vector<ustl::string> vs(v.size()+1);
  int i,total=0;
  const char typ[]="idzDcpiveSfEsFRmuMwgPF";
  for (i=0;i<v.size();++i){
    vs[i]=v[i].print(contextptr);
    if (v[i].type==giac::_IDNT){
      giac::gen w;
      v[i]._IDNTptr->in_eval(0,v[i],w,contextptr,true);
      //vector<int> vi(9); tailles(w,vi); total += vi[8]; vs[i] += " ~"; vs[i] += giac::print_INT_(vi[8]);
      vs[i] += ',';
      vs[i] += typ[w.type];
    }
    smallmenuitems[i].text=(char *) vs[i].c_str();
  }
  // total += sizeof(giac::context)+contextptr->tabptr->capacity()*(sizeof(const char *)+sizeof(giac::gen)+8)+bytesize(giac::history_in(contextptr))+bytesize(giac::history_out(contextptr));
#ifndef TICE
  vs[i]="purge(~"+giac::print_INT_(total)+')';
#else
  char* vs_str = new char [sizeof("purge(~-8388608)")];
  sprintf(vs_str, "purge(~%d)", total);
  vs[i]=vs_str;
#endif
  smallmenuitems[i].text=(char *)vs[i].c_str();
  smallmenuitems[i+1].text=(char *)"assume(";
  smallmenuitems[i+2].text=(char *)"restart ";
  smallmenuitems[i+3].text=(char *)"VARS()";
  Menu smallmenu;
  smallmenu.numitems=v.size()+4; 
  smallmenu.items=smallmenuitems;
  smallmenu.height=12;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  int freemem=(int)malloc(0xffffff);
#ifndef TICE
  string title=("Variables "+giac::print_INT_(freemem));
  smallmenu.title = (char*) title.c_str();
#else
  char* title = new char [sizeof("Variables -8388608")];
  sprintf(title, "Variables %d", freemem);
  smallmenu.title = (char*) title;
#endif
  //MsgBoxPush(5);
  int sres = doMenu(&smallmenu);
  //MsgBoxPop();
  if (smallmenu.selection && smallmenu.selection<=v.size() && (sres==MENU_RETURN_SELECTION || sres==KEY_CTRL_DEL)){
    g=v[smallmenu.selection-1];
    if (sres==KEY_CTRL_DEL)
      g=giac::symbolic(giac::at_purge,g);
    strcpy(varbuf,g.print(contextptr).c_str());
    return varbuf;
  }
  if (sres==MENU_RETURN_SELECTION){
    if (smallmenu.selection==1+v.size())
      return "purge(";
    if (smallmenu.selection==2+v.size())
      return "assume(";
    if (smallmenu.selection==3+v.size())
      return "restart";
    if (smallmenu.selection==4+v.size())
      return "VARS()";
  }
  return "";  
#endif
}  


//const char * keywords[]={"do","faire","for","if","return","while"}; // added to lexer_tab_int.h

  const char * python_keywords[] = {   // List of known giac keywords...
    "False",
    "None",
    "True",
    "and",
    "break",
    "continue",
    "def",
    "default",
    "elif",
    "else",
    "except",
    "for",
    "from",
    "global",
    "if",
    "import",
    "not",
    "or",
    "return",
    "throw",
    "try",
    "while",
    "xor",
    "yield",
  };
  const char * const python_builtins[]={
    "NoneType",
    "__call__",
    "__class__",
    "__delitem__",
    "__dir__", 
    "__enter__",
    "__exit__",
    "__getattr__",
    "__getitem__",
    "__hash__",
    "__init__",
    "__int__",
    "__iter__",
    "__len__",
    "__main__",
    "__module__",
    "__name__",
    "__new__",
    "__next__",
    "__qualname__",
    "__repr__",
    "__setitem__",
    "__str__",
    "abs",
    "all",
    "any",
    "append",
    "args",
    "bool",
    "builtins",
    "bytearray",
    "bytecode",
    "bytes",
    "callable",
    "chr",
    "classmethod",
    "complex",
    "dict",
    "dir",
    "divmod",
    "eval",
    "exec",
    "float",
    "format",
    "getattr",
    "globals",
    "hasattr",
    "hash",
    "hex",
    "id",
    "index",
    "input",
    "int",
    "iter",
    "len",
    "list",
    "locals",
    "map",
    "max",
    "min",
    "next",
    "object",
    "oct",
    "pow",
    "print",
    "range",
    "repr",
    "reversed",
    "round",
    "self",
    "set",
    "setattr",
    "sorted",
    "split",
    "str",
    "sum",
    "super",
    "tuple",
    "type",
    "zip",
  };

  int dichotomic_search(const char * const * tab,unsigned tab_size,const char * s){
    int beg=0,end=tab_size,cur,test;
    // string index is always >= begin and < end
    for (;;){
      cur=(beg+end)/2;
      test=strcmp(s,tab[cur]);
      if (!test)
	return cur;
      if (cur==beg)
	return -1;
      if (test>0)
	beg=cur;
      else
	end=cur;
    }
    return -1;
  }

  bool is_python_keyword(const char * s){
    return dichotomic_search(python_keywords,sizeof(python_keywords)/sizeof(char*),s)!=-1;
  }
  
  bool is_python_builtin(const char * s){
    return dichotomic_search(python_builtins,sizeof(python_builtins)/sizeof(char*),s)!=-1;
  }

void do_eval(giac::gen & g){
#ifdef FAKE_GIAC
  statuslinemsg(!lang?"cancel: stop calcul.":"annul: stoppe calcul",COLOR_RED);
  os_wait_1ms(1000);
#else
  freeze=giac::ctrl_c=giac::kbd_interrupted=giac::interrupted=false;
#ifndef WITH_QUAD
  if (taille(g,64)<64)
    dbg_printf("Eval %s\n",g.print(contextptr).c_str());
#endif
  giac::set_abort();
  statuslinemsg(!lang?"cancel: stop calcul.":"annul: stoppe calcul",COLOR_RED);
  g=giac::eval(g,giac::eval_level(contextptr),contextptr);
  if (giac::interrupted){
    print_msg12(lang?"Interrompu":"Interrupted",0);
    getkey(0);
    freeze=false;
  }
  giac::clear_abort();
  giac::ctrl_c=giac::kbd_interrupted=giac::interrupted=false;
  if (freeze){
    statuslinemsg(lang?"Ecran fige.":"Screen freezed",COLOR_RED);
    getkey(0);
    freeze=false;
  }
#endif
}

// called from editor, return 
int check_parse(const std::vector<textElement> & v,int python){
#ifdef FAKE_GIAC
  return 0;
#else
  //dbg_printf("check_parse\n");
  if (v.empty())
    return 0;
  char status[256];
  for (int i=0;i<sizeof(status);++i)
    status[i]=0;
  std::string s=merge_area(v);
  giac::python_compat(python,contextptr);
  if (python) s="@@"+s; // force Python translation
  giac::gen g(s,contextptr);
  int lineerr=giac::first_error_line(contextptr);
  if (lineerr){
    ustl::string tok=giac::error_token_name(contextptr);
    int pos=-1;
    if (lineerr>=1 && lineerr<=v.size()){
      pos=v[lineerr-1].s.find(tok);
      const ustl::string & err=v[lineerr-1].s;
      if (pos>=err.size())
	pos=-1;
      if (python){
	// find 1st token, check if it's def/if/elseif/for/while
	size_t i=0,j=0;
	for (;i<err.size();++i){
	  if (err[i]!=' ')
	    break;
	}
	ustl::string firsterr;
	for (j=i;j<err.size();++j){
	  if (!isalpha(err[j]))
	    break;
	  firsterr += err[j];
	}
	// if there is no : at end set pos=-2
	if (firsterr=="for" || firsterr=="def" || firsterr=="if" || firsterr=="elseif" || firsterr=="while"){
	  for (i=err.size()-1;i>0;--i){
	    if (err[i]!=' ')
	      break;
	  }
	  if (err[i]!=':')
	    pos=-2;
	}
      }
    }
    else {
      lineerr=v.size();
      tok=lang?"la fin":"end";
      pos=0;
    }
    string S((lang?"Erreur ligne ":"Error line ")+giac::print_INT_(lineerr));
    if (pos>=0)
      do_confirm((S+(lang?" a ":" at ")+tok).c_str());
    else {
      if (pos==-2)
        S += lang?". ; manquant ?":", : missing?";
      do_confirm(S.c_str());
    }
  }
  else {
#if 1
    do_eval(g);
    statuslinemsg(lang?"Syntaxe OK.":"Parse OK.",COLOR_CYAN);
    os_wait_1ms(700);
#else
    print_msg12(lang?"Syntaxe OK.":"Parse OK.",lang?"Taper une touche pour evaluer.":"Type any key to eval.");
    int key=getkey(1);
    if (key!=KEY_CTRL_EXIT)
      do_eval(g);
#endif
  }
  return lineerr;
#endif
}

int find_color(const char * s){
  if (!s) return 0;
  char ch=s[0];
  if (ch=='"')
    return 38052;
  if (!isalpha(s[0]))
    return 0;
  char buf[256];
  const char * ptr=s;
  for (int i=0;i<255 && (isalphanum(*ptr) || *ptr=='_'); ++i){
    ++ptr;
  }
  strncpy(buf,s,ptr-s);
  buf[ptr-s]=0;
  if (strcmp(buf,"def")==0 || strcmp(buf,"import")==0 || is_python_keyword(buf))
    return _BLUE;
#ifdef FAKE_GIAC
  if (is_python_builtin(buf))
    return _CYAN;
  if (strcmp(buf,"sin")==0)
    return 24844;
  return 0;
#else  
  giac::gen g;
  int token=giac::find_or_make_symbol(buf,g,0,false,contextptr);
  //dbg_printf("find_color %s %i %s\n",buf,token,g.print(contextptr).c_str());
  if (token==T_UNARY_OP || token==T_UNARY_OP_38 || token==T_LOGO)
    return 24844; // 38052;
  if (token==T_NUMBER)
    return _GREEN;
  if (token!=T_SYMBOL)
    return _BLACK;
  return _MAGENTA;
#endif
}

std::string get_searchitem(std::string & replace){
  replace="";
  std::string search;
  lock_alpha();
  int res=inputline(lang?"EXIT ou chaine vide: annulation":"EXIT or empty string: cancel",lang?"Chercher:":"Search:",search,false);
  if (search.empty() || res==KEY_CTRL_EXIT)
    return "";
  replace="";
  std::string tmp=(lang?"EXIT: recherche seule de ":"EXIT: search only ")+search;
  lock_alpha();
  res=inputline(tmp.c_str(),lang?"Remplacer par:":"Replace by:",replace,false);
  if (res==KEY_CTRL_EXIT)
    replace="";
  return search;
}


int select_script_and_run() {
  char filename[MAX_FILENAME_SIZE+1];
  if (fileBrowser(filename, (char*)"*.py", (char *)"Run script")) 
    return run_script(filename);
  return 0;
}

void erase_script(){
  char filename[MAX_FILENAME_SIZE+1];
  int res=fileBrowser(filename, (char*)"*.py", (char *)"Scripts");
  if (res && do_confirm(lang?"Vraiment effacer":"Really erase?")){
    erase_file(filename);
  }
}

string extract_name(const char * s){
  int l=strlen(s),i,j;
  for (i=l-1;i>=0;--i){
    if (s[i]=='.')
      break;
  }
  if (i<=0)
    return "f";
  for (j=i-1;j>=0;--j){
    if (s[j]=='\\')
      break;
  }
  if (j<0)
    return "f";
  return string(s+j+1).substr(0,i-j-1);
}

void edit_script(const char * fname){
  // clear_abort();
  char fname_[MAX_FILENAME_SIZE+1];
  char * filename=0;
  int res=1;
  if (fname)
    filename=(char *)fname; // safe, it will not be modified
  else {
    res=fileBrowser(fname_, (char*)"*.py", (char *)"Scripts");
    filename=fname_;
  }
  if (res) {
    string s;
    load_script(filename,s);
    if (s.empty()){
      int k=KEY_CTRL_F5; // confirm("Program","F1: Tortue, F5: Python",true);
      if (k==-1)
        return;
      if (k==KEY_CTRL_F1)
        s="\nefface;\n ";
      else{
        s=extract_name(filename);
        if (s=="session")
          s="f";
        s="def "+s+"(x):\n  \n  return x";
      }
    }
    // split s at newlines
    if (edptr==0)
      edptr=new textArea;
    if (!edptr) return;
    edptr->elements.clear();
    edptr->clipline=-1;
#if defined FX || defined FXCG
    edptr->filename="\\\\fls0\\"+remove_path(remove_extension(filename))+".py";
#else
    edptr->filename=remove_path(remove_extension(filename));
#endif
    //cout << "script " << edptr->filename << endl;
    edptr->editable=true;
    edptr->changed=false;
    edptr->python=true;
    edptr->longlinescut=false;
    edptr->elements.clear();
    edptr->y=0;
    add(edptr,s);
    s.clear();
    edptr->line=0;
    //edptr->line=edptr->elements.size()-1;
    edptr->pos=0;
    //dbg_printf("dotextarea\n");
    int res=doTextArea(edptr);
  }
}

string khicas_state(){
#ifdef FAKE_GIAC
  return "";
#else
  giac::gen g(giac::_VARS(-1,contextptr)); 
  int b=xcas_python_eval==1?4:python_compat(contextptr);
  python_compat(0,contextptr);
  char buf[2048]="";
  //dbg_printf("VARS=%s\n",g.print(contextptr).c_str());
  if (g.type==giac::_VECT){
    for (int i=0;i<g._VECTptr->size();++i){
      string s((*g._VECTptr)[i].print(contextptr));
      //dbg_printf("VAR[%i] %s\n",i,s.c_str());
      if (strlen(buf)+s.size()+128<sizeof(buf)){
	strcat(buf,s.c_str());
	strcat(buf,":;");
      }
    }
  }
  python_compat(b,contextptr);
  if (strlen(buf)+128<sizeof(buf)){
    strcat(buf,"python_compat(");
    strcat(buf,giac::print_INT_(b).c_str());
    strcat(buf,");angle_radian(");
    strcat(buf,angle_radian(contextptr)?"1":"0");
    strcat(buf,");with_sqrt(");
    strcat(buf,withsqrt(contextptr)?"1":"0");
    strcat(buf,");");
  }
  //dbg_printf("khicas_state %s\n",buf);
  return buf;
#endif
}

void save_khicas_symbols_smem(const char * filename) {
  // save variables in xcas mode,
  // because at load time the parser will be in xcas mode
  string s(khicas_state());
  save_script(filename,s);
}

string remove_path(const string & st){
  int s=int(st.size()),i;
  for (i=s-1;i>=0;--i){
    if (st[i]=='\\')
      break;
  }
  return st.substr(i+1,s-i-1);
}


string remove_extension(const string & st){
  int s=int(st.size()),i;
  for (i=s-1;i>=0;--i){
    if (st[i]=='.')
      break;
  }
  return st.substr(0,i);
}

void save(const char * fname){
  //clear_abort();
  string filename(remove_path(remove_extension(fname)));
  save_console_state_smem((filename+".xw").c_str()); 
  if (edptr)
    check_leave(edptr);
}

void save_session(){
  if (strcmp(session_filename,"session") && console_changed){
    std::string tmp(session_filename);
    tmp += lang?" a ete modifie!":" was modified!";
    if (confirm(tmp.c_str(),lang?"F1: sauvegarder, F5: tant pis":"F1: save, F5: discard changes")==KEY_CTRL_F1){
      save(session_filename);
      console_changed=0;
    }    
  }
  save("session");
  // this is only called on exit, no need to reinstall the check_execution_abort timer.
  if (edptr && edptr->changed && edptr->filename!="session.py"){
    if (!check_leave(edptr)){
      save_script("lastprg.py",merge_area(edptr->elements));
    }
  }
}

int restore_session(const char * fname){
  clear_screen(); // Bdisp_AllClr_VRAM();
  drawRectangle(0,0,LCD_WIDTH_PX,16,COLOR_BLACK);
#ifdef WITH_DESOLVE
  os_draw_string_medium_(0,0,lang?"KhiCAS pour TI83 [allegee avec desolve]":"KhiCAS for TI84 [with desolve]");
#else
  os_draw_string_medium_(0,0,lang?"KhiCAS pour TI83 [allegee sans desolve]":"KhiCAS for TI84 [without desolve]");
#endif
  os_draw_string_medium_(0,C18,"(c) B. Parisse et al, license GPL 2");
  os_draw_string_medium_(0,2*C18,"www-fourier.univ-grenoble-alpes.fr/~parisse");
  // os_draw_string_medium_(0,3*C18,"");
  os_draw_string_medium_(0,4*C18,lang?"Utiliser les 5 touches sous l'ecran pour":"Press one of the 5 keys below the screen");
  os_draw_string_medium_(0,5*C18,lang?"ouvrir un menu, cf. la legende au-dessus":"will open a fast menu according to the legend");
  os_draw_string_medium_(0,6*C18,lang?"Touche sto: sauvegarde la session":"Press sto to save the session");
  string filename(remove_path(remove_extension(fname)));
  if (load_console_state_smem((filename+".xw").c_str()))
    return 1;
  else {
    statuslinemsg("");
    os_draw_string_medium_(0,9*C18,lang?"Tapez une touche.":"Press any key");
    int key; ck_getkey(&key);
    return 0;
  }
}

#ifdef TEX
#include "TeX.h"
int tex_flag=1;
extern "C"{
  int* get_tex_flag_address(){
    return &tex_flag;
  }
}

void TeX_init(void){
  Txt_Init(FONT_SYSTEM);
}

void TeX_quit(void){
  Txt_Quit();
}
#endif



bool textedit(char * s){
  if (!s)
    return false;
  int ss=strlen(s);
  if (ss==0){
    *s=' ';
    s[1]=0;
    ss=1;
  }
  textArea ta;
  ta.elements.clear();
  ta.editable=true;
  ta.clipline=-1;
  ta.changed=false;
  ta.filename="temp.py";
  ta.y=0;
  ta.allowEXE=true;
  bool str=s[0]=='"' && s[ss-1]=='"';
  if (str){
    s[ss-1]=0;
    add(&ta,s+1);
  }
  else
    add(&ta,s);
  ta.line=0;
  ta.pos=ta.elements[ta.line].s.size();
  int res=doTextArea(&ta);
  if (res==TEXTAREA_RETURN_EXIT)
    return false;
  string S(merge_area(ta.elements));
  if (str)
    S='"'+S+'"';
  int Ssize=S.size();
  if (Ssize<GEN_PRINT_BUFSIZE){
    strcpy(s,S.c_str());
    for (--Ssize;Ssize>=0;--Ssize){
      if ((unsigned char)s[Ssize]==0x9c || s[Ssize]=='\n')
	s[Ssize]=0;
      if (s[Ssize]!=' ')
	break;
    }
    return true;
  }
  return false;
}

bool stringtodouble(const string & s1,double & d){
  char * ptr=0;
  d=strtod(s1.c_str(),&ptr);
  return ptr!=0;
}

// keep only 6 digits 
void ti_sprint_float(char * ch,float d){
  int i=d;
  if (i==d){
    sprintf(ch,"%i.0",i);
    return ;
  }
  ch[0]=0;
  bool pos=d>=0;
  if (!pos)
    d=-d;
  float m=frexp(d,&i);
  // d=m*2^i
  // 2^i is near 1000^j
  bool negexp=i<0;
  if (negexp)
    i=-i;
  int j=i/10;
  float pow1000=1;
  int exp10=0;
  for (int k=j;k>0;k--){
    pow1000 *= 1000;
    exp10+=3;
  }
  if (negexp){
    d=d*pow1000;
    exp10=-exp10;
  }
  else 
    d=d/pow1000;
  while (d<1e5){
    d*=10;
    exp10--;
  }
  i=d+.5;
  if (!pos){
    ch[0]='-';
    ++ch;
  }
  sprintf(ch,"%ie%i",i,exp10);
}

void ti_sprint_double(char * ch,double d){
  int i=d;
  if (i==d){
    sprintf(ch,"%i.0",i);
    return;
  }
  real_t tmp_real = os_FloatToReal(d);
  os_RealToStr(ch, &tmp_real, 11, 1, -1);
  int s=strlen(ch);
  for (int i=0;i<s;++i){
    if (ch[i]==0x1b)
      ch[i]='e';
    if (ch[i]==0x1a)
      ch[i]='-';
  }
#ifndef WITH_QUAD
  dbg_printf("%x %x %x %x %x %x %x %x",ch[0],ch[1],ch[2],ch[3],ch[4],ch[5],ch[6],ch[7]);
#endif
}

void do_run(const char * s){
  int S=strlen(s);
  char * buf=(char *)malloc(max(S+1,256));
  if (!buf){
    do_confirm("Memory full");
    return;
  }
  buf[S]=0;
  for (int i=0;i<S;++i){
    char c=s[i];
    if (c==0x1e || c==char(0x9c))
      buf[i]='\n';
    else {
      if (c==0x0d)
        buf[i]=' ';
      else
        buf[i]=c;
    }
  }
  S=strlen(buf);
  if (S==3 && buf[0]=='[' && buf[2]==']' && buf[1]>='A' && buf[1]<='I'){
    string mats=get_timatrix(buf[1]-'A');
    if (mats.size()){
      mats += "=>";
      mats += char((buf[1]-'A')+'a');
      free(buf);
      do_run(mats.c_str());
      return;
    }
  }
  int yn=0x11;
  if (S>=5){ // detect "...=>Yk"
    //dbg_printf("detect %s %x %x %x %x\n",buf+S-4,buf[S-4],buf[S-3],buf[S-2],buf[S-1]);
    char c=buf[S-1];
    if (c>='0' && c<='9'){
      char y=buf[S-2];
      if ((y=='y' || y=='Y') && buf[S-3]=='>' && buf[S-4]=='='){
        //buf[S-4]=0;
        yn = (c=='0')?0x19:(c-'1'+0x10);
      }
    }
  }
#ifdef FAKE_GIAC
  if (S==2 && (s[0]=='Y' || s[0]=='y') && (s[1]>='0' && s[1]<='9')){
    char buf[3]={0x5e,0,0};
    buf[1]=(s[1]=='0')?0x19:(0x10+(s[1]-'1'));
    string val=get_tivar(buf);
    if (val.size())
      Console_Output(val.c_str());
    else
      Console_Output(s);      
  }
  else if (s[0]>='0' && s[0]<='9'){
    double d=atol(s);
    dbg_printf("atof %f\n",d);
    char ch[128];
    ti_sprint_double(ch,atan(d));
    Console_Output(ch);          
  }
  else {
    Console_Output(buf);
    vector<unsigned char> v;
    tokenize(buf,v);
    if (v.size()>2){
      char buf[3]={0x5e,0,0};
      buf[1]=yn;
      int res=os_CreateEquation(buf,(equ_t *)&v.front());
      //dbg_printf("create Y%i res=%i\n",yn,res);
    }
  }
  Console_NewLine(LINE_TYPE_OUTPUT,1);
#else
  giac::ctrl_c=giac::kbd_interrupted=giac::interrupted=false;
  if (1 && xcas_python_eval==0){
    if (!contextptr)
      contextptr=new giac::context;
    giac::gen g(buf,contextptr);
    g=equaltosto(g,contextptr);
    g=add_autosimplify(g,contextptr);
    do_eval(g);
#ifdef WITH_EQW
    giac::gen gs;
    int do_logo_graph_eqw=7;
    xcas::check_do_graph(g,gs,do_logo_graph_eqw,contextptr);
#endif
#ifdef WITH_PLOT
    if (xcas::ispnt(g))
      Console_Output("Graphic object");
    else
#endif
    if (taille(g,256)>=256)
      Console_Output("Done");
    else {
      const char * s=g.print(contextptr).c_str();
      Console_Output(s);
      vector<unsigned char> v;
      tokenize(s,v);
      //dbg_printf("Y2 %s v.size()=%i\n",s,v.size());
      if (v.size()>2){
        char buf[3]={0x5e,0,0};
        buf[1]=yn;
        int res=os_CreateEquation(buf,(equ_t *)&v.front());
        // dbg_printf("create Y2 res=%i\n",res);
      }
    }
    Console_NewLine(LINE_TYPE_OUTPUT,1);
  }
  else {
    execution_in_progress_py = 1;
    // set_abort_py();
    int res=micropy_ck_eval(buf);
    execution_in_progress_py = 0;
  }
  free(buf);
  // clear_abort_py();
  if (get_free_memory()<8*1024){
    // FIXME? clear turtle, display msg
  } 
  //Console_Output("Done"); return ;
#endif
}

void run(const char * s,int do_logo_graph_eqw){
  if (strlen(s)>=2 && (s[0]=='#' ||
		       (s[0]=='/' && (s[1]=='/' || s[1]=='*'))
		       ))
    return;
  do_run(s);
  process_freeze();
  //return ge; 
}

// returns 1 if script was run, 0 otherwise
int run_script(const char* filename) {
  string s;
  if (load_script(filename,s)<=0)
    return 0;
  do_run(s.c_str());
  return 1;
}

void load_khicas_vars(const char * BUF){
#ifdef FAKE_GIAC
  statuslinemsg("Loading vars",COLOR_RED);
#else
  if (!contextptr)
    contextptr=new giac::context;
  dconsole_mode=0;
  xcas_mode(contextptr)=python_compat(contextptr)=0;
  bool bi=try_parse_i(contextptr);
  try_parse_i(false,contextptr);
  giac::gen g(BUF,contextptr);
  try_parse_i(bi,contextptr);
  statuslinemsg("Evaluating vars",COLOR_RED);
  //       dbg_printf("load_khicas_vars BUF=%s g=%s\n",BUF,g.print().c_str());
  if (g.type==giac::_VECT){
    const giac::vecteur & v =*g._VECTptr;
    for (int i=0;i<v.size();++i){
      const giac::gen & vi=v[i];
      //dbg_printf("load_khicas_vars i=%i v[i]=%s\n",i,vi.print().c_str());
      if (vi.is_symb_of_sommet(giac::at_nodisp) && vi._SYMBptr->feuille.is_symb_of_sommet(giac::at_sto)){
        const giac::gen &f=vi._SYMBptr->feuille._SYMBptr->feuille;
        if (f.type==giac::_VECT && f._VECTptr->size()==2){
          //dbg_printf("load_khicas_vars i=%i\n",i,vi.print().c_str());
          giac::sto(f._VECTptr->front(),f._VECTptr->back(),contextptr);
        }
      }
      else
        giac::eval(vi,1,contextptr);
    }
  }
  else
    g=giac::eval(g,1,contextptr);
  giac::angle_radian(giac::angle_radian(contextptr),giac::context0);
#endif
}  


void stop(const char * s)
{
  python_free();
#if KHICAS_STACK
  asm("assume	adl = 1\n\t"
    "ld  sp, ($D19888)\n\t"
    : /* output */
    : /* input */
    : /* clobbered registers */
  );
#endif
  lcd_Control = 0b100100101101; // TI-OS default
  exit(1);
}

#ifdef KMALLOC
kmalloc_arena_t static_ram = { 0 },ram3M={0};
#ifdef FXCG
#ifdef GINT_MALLOC
// #define VAR_HEAP
#ifdef VAR_HEAP
__attribute__((aligned(4))) char malloc_heap[240*1024];
#else
/* Unused part of user stack; provided by linker script */
extern char sextra, eextra;
#endif
#endif // KMALLOC

int get_free_memory(){
  kmalloc_gint_stats_t * s;
  s=kmalloc_get_gint_stats(&ram3M);
  if (!s) return 0;
  int res=s->free_memory;
  return res;
}
#else // FXCG
#ifdef FX
int get_free_memory(){
  kmalloc_gint_stats_t * s;
  s=kmalloc_get_gint_stats(&static_ram);
  int res=s?s->free_memory:0;
  return res;
}
#else // FX
int get_free_memory(){
  return freeslotmem();
}
#endif // FX
#endif // FXCG
#endif // KMALLOC

vector<logo_turtle> & turtle_stack();

#ifdef FAKE_GIAC
namespace giac {
  unsigned ustl_vecteur_prolog=0;
}
#endif

extern "C" unsigned char __heapbot[];
extern "C" unsigned char __heaptop[];

void runExternalProgramAndExit(const char* prgmName)
{
  python_free();
#if KHICAS_STACK
  asm("assume	adl = 1\n\t"
    "ld  sp, ($D19888)\n\t"
    : /* output */
    : /* input */
    : /* clobbered registers */
  );
#endif
  lcd_Control = 0b100100101101; // TI-OS default
  os_RunPrgm(prgmName, nullptr, 0, nullptr);
  exit(1);
}

int main1(){
  ustl::vector<giac::gen> ustlv;
  unsigned * ustlptr=(unsigned *)&ustlv;
  giac::ustl_vecteur_prolog=*ustlptr;
  //dbg_printf("ustlv len=%i %x %x %x %x\n",sizeof(ustlv),ustlptr[0],ustlptr[1],ustlptr[2],ustlptr[3]);
#ifdef FAKE_GIAC
#if defined WITH_LCDMALLOC
  // check kmalloc
  void * ptr=malloc(65536);
  int freemem=(int)malloc(0xffffff);
  dbg_printf("malloc 65536 ptr=%x free=%i\n",ptr,freemem);
  free(ptr);
  dbg_printf("after free 65536 ptr=%x free=%i\n",ptr,freemem);
#else
  void * ptr=malloc(32768);
  int freemem=(int)malloc(0xffffff);
  dbg_printf("malloc 32768 ptr=%x free=%i\n",ptr,freemem);
#endif
#else  // FAKE_GIAC
  //dbg_printf("ref_eqwdata=%i ref_complex=%i ref_symbolic=%i ref_vecteur=%i\n",sizeof(giac::ref_eqwdata),sizeof(giac::ref_complex),sizeof(giac::ref_symbolic),sizeof(giac::ref_vecteur));
  //dbg_printf("archive index plus=%i sin=%i\n",giac::archive_function_index(giac::at_plus),giac::archive_function_index(giac::at_sin));
  //dbg_printf("allocfast size refv=%i refs=%i refc=%i\n",sizeof(giac::ref_vecteur),sizeof(giac::ref_symbolic),sizeof(giac::ref_complex));
  if (!contextptr)
    contextptr = new giac::context;
  giac::epsilon(contextptr)=1e-5;
#endif // FAKE_GIAC

#ifdef KMALLOC

  /* À appeler une seule fois au début de l'exécution */
  kmalloc_init();
  
  /* Ajouter une arène sur la RAM inutilisée */
  static_ram.name = "_uram";
  static_ram.is_default = 1; // 0 for system malloc first
#ifdef VAR_HEAP
  static_ram.start = malloc_heap;
  static_ram.end = malloc_heap+sizeof(malloc_heap);
#else
  static_ram.start = (void *)  0x88053800; // tab24+6144
  static_ram.end =  (void *) 0x8807f000;
#endif
  kmalloc_init_arena(&static_ram, true);
  kmalloc_add_arena(&static_ram);
#endif // KMALLOC

  //do_confirm("SDK init"); return 0;
  // main starts after malloc init
  int i = 0, j = 0;
  
  SetQuitHandler(save_session); // automatically save session when exiting
  // FIXME turtle
  turtle(); 
  turtle_stack(); // required to init turtle
  Console_Init();
  const system_info_t * sptr=os_GetSystemInfo();
  //if (sptr) dbg_printf("%i %i %i %i\n",sptr->hardwareType,sptr->osMajorVersion,sptr->osMinorVersion, sptr->osRevisionVersion);
  // please do not remove this check, it's here for TI
  if (sptr && sptr->osMajorVersion==5 &&
      (sptr->osMinorVersion>8 ||
       (sptr->osMinorVersion==8 && sptr->osRevisionVersion>=2)
       )
      ){
    if (sptr->hardwareType==0){
      do_confirm("TI84 incompatible OS version");
      return 1;
    }
    else
      do_confirm("! OS incompatible avec mode examen !");
  }
  else if (sptr && sptr->osMajorVersion==5 && sptr->osMinorVersion==8 && sptr->hardwareType==1)
    confirm("!!! Downgradez l'OS avec CERMASTR","pour utiliser KhiCAS en mode examen");
  // do_confirm("console init");
  restore_session("session");
  //dbg_printf("main1\n");
#ifdef FXCG
  if (prizmoremu && pythonjs_heap_size>=370*1024)
    pythonjs_heap_size=370*1024;
#endif
  //dbg_printf("plus %x %x\n",(size_t)giac::at_plus->ptr(),(size_t) giac::_plus);
  //pythonjs_static_heap=(char *)malloc(pythonjs_heap_size);
  python_init(pythonjs_stack_size,pythonjs_heap_size);
  //dbg_printf("main2\n");
  //do_confirm("after init");
  //load_config();
  //{int K; ck_getkey(&K); sdk_end(); return 0;}
  Console_Disp(1);
  init_locale();
  // { statuslinemsg("after console init"); int key; GetKey(&key); }
  //do_confirm("ready");
  //Bkey_SetAllFlags(0x80); // disable catalog syscall 0x0EA1 on the Prizm
  // initialize failed ?
  // if (!(line && free_stack && mem && stack && symtab && binding && arglist && logbuf))		return 0;
  while (1) { 
    //dbg_printf("main1 loop\n");
    const Char *expr;
    if ( (expr=Console_GetLine())==NULL )
      stop("memory error");
    else if (strcmp(expr, "asm") == 0) {
      runExternalProgramAndExit("ASMHOOK");
    }
    else if (strcmp(expr,"kill")==0
        // && confirm("Quitter?",lang?"F1: confirmer,  F5: annuler":"F1: confirm,  F5: cancel")==KEY_CTRL_F1
        ){
      save("session"); // FIXME TICE
      if (strcmp(session_filename,"session")) // save without confirmation
        save(session_filename);
      break;
    }
    else if (strcmp(expr,"restart")==0){
      if (confirm(lang?"Effacer variables?":"Clear variables?",lang?"F1: annul,  F5: confirmer":"F1: cancel,  F5: confirm")!=KEY_CTRL_F5){
        Console_Output(" cancelled");
        Console_NewLine(LINE_TYPE_OUTPUT,1);
        //ck_getkey(&key);
        Console_Disp(1);
        continue;
      }
    }
    // should save in another file
    else if (strcmp(expr,"=>")==0 || strcmp(expr,"=>\n")==0){
      save_session();
      Console_Output((Char*)"Session saved");
    }
    else {
      save_console_state_smem("session.xw"); 
      run(expr);
    }
    //print_mem_info();
    Console_NewLine(LINE_TYPE_OUTPUT,1);
    //ck_getkey(&key);
    Console_Disp(1);
  }
  return 1;
}

unsigned stack_ptr=0;

int main(){
  unsigned appstart=(0x3b0000-3);
  appstart -= *(unsigned *) appstart;
  unsigned pcmain=(unsigned) main;
#ifndef WITH_QUAD
  dbg_printf("appstart=%x pcmain=%x\n",appstart,pcmain);
#endif
  if (pcmain<appstart || pcmain>=0x3b0000)
    return 1;
#if KHICAS_STACK
#if 0
  unsigned localsp=0;
  asm("assume	adl = 1\n\t"
      "ld  ($D19888), sp\n\t"
      "ld  %0, ($D19888)\n\t"
      :"=r"(localsp)        /* output */
    );
  stack_ptr=localsp;
  //dbg_printf("start sp %x\n",stack_ptr);
#else
  asm("assume	adl = 1\n\t"
      "ld  ($D19888), sp\n\t"
      "ld  sp, $D2a800\n\t"
      :        /* output */
      :  /* input */
      : /* clobbered registers */
    );
#endif
#endif
  sdk_init();
  mp_stack_ctrl_init();
#ifdef FAKE_GIAC
  dbg_printf("heap bot=%x top=%x\n",__heapbot,__heaptop);
#else 
#ifdef WITH_LCDMALLOC
  giac::tab11=(giac::char11*) malloc(giac::ALLOC11*sizeof(giac::char11));
  giac::tab16=(giac::char16*) malloc(giac::ALLOC16*sizeof(giac::char16));
  giac::tab15=(giac::char15*) malloc(giac::ALLOC15*sizeof(giac::char15));
  giac::tab36=(giac::char36*) malloc(giac::ALLOC36*sizeof(giac::char36));
#else
#ifdef WITH_LCDMALLOC  // should never be reached
  giac::tab11=(giac::char11*) (((unsigned char *) __heapbot);
#else
  giac::tab11=(giac::char11*) (((unsigned char *) lcd_Ram)+LCD_WIDTH_PX*LCD_HEIGHT_PX);
#endif
  giac::tab16=(giac::char16*)(giac::tab11+giac::ALLOC11);
  giac::tab15=(giac::char15*)(giac::tab16+giac::ALLOC16);
  giac::tab36=(giac::char36*)(giac::tab15+giac::ALLOC15);
#endif // WITH_LCDMALLOC
#endif // FAKE_GIAC
  main1();
  python_free();
  sdk_end();
#if KHICAS_STACK
  asm("assume	adl = 1\n\t"
      "ld  sp, ($D19888)\n\t"
      :        /* output */
      :  /* input */
      : /* clobbered registers */
    );
#endif
  return 0;
}

void console_output(const char * s,int l){
  // confirm("console_output",s);
  char buf[l+1];
  strncpy(buf,s,l);
  buf[l]=0;
  dConsolePut(buf);
}

const char * console_input(const char * msg1,const char * msg2,bool numeric,int ypos){
  string str;
  if (!inputline(msg1,msg2,str,numeric,ypos))
    return 0;
  char * ptr=strdup(str.c_str());
  return ptr;
}

int do_confirm(const char * s){
  return confirm(s,(lang?"F1: oui,    F5:annuler":"F1: yes,     F5: cancel"))==KEY_CTRL_F1;
}

int kbd_filter(int key){
  if (key==KEY_CTRL_LEFT) return 0;
  if (key==KEY_CTRL_RIGHT) return 3;
  if (key==KEY_CTRL_UP) return 1;
  if (key==KEY_CTRL_DOWN) return 2;
  if (key==KEY_CTRL_EXE) return 4;
  if (key==KEY_CTRL_EXIT) return 5;
  return key;
}  

int kbd_convert(int r,int c){
  if (r==1 && c==1)
    return KEY_CTRL_AC;
  if (r==2){
    if (c==7) return KEY_CHAR_0;
    if (c==6) return KEY_CHAR_DP;
    if (c==5) return KEY_CHAR_EXPN10;
    if (c==4) return KEY_CHAR_PMINUS;
    if (c==3) return 4; // KEY_CTRL_EXE;
  }
  if (r==3){
    if (c==7) return KEY_CHAR_1;
    if (c==6) return KEY_CHAR_2;
    if (c==5) return KEY_CHAR_3;
    if (c==4) return KEY_CHAR_PLUS;
    if (c==3) return KEY_CHAR_MINUS;
  }
  if (r==4){
    if (c==7) return KEY_CHAR_4;
    if (c==6) return KEY_CHAR_5;
    if (c==5) return KEY_CHAR_6;
    if (c==4) return KEY_CHAR_MULT;
    if (c==3) return KEY_CHAR_DIV;
  }
  if (r==5){
    if (c==7) return KEY_CHAR_FRAC;
    if (c==6) return KEY_CHAR_H;
    if (c==5) return KEY_CHAR_LPAR;
    if (c==4) return KEY_CHAR_RPAR;
    if (c==3) return KEY_CHAR_COMMA;
    if (c==2) return KEY_CHAR_STORE;
  }
  if (r==6){
    if (c==7) return KEY_CTRL_XTT;
    if (c==6) return KEY_CHAR_LOG;
    if (c==5) return KEY_CHAR_LN;
    if (c==4) return KEY_CHAR_SIN;
    if (c==3) return KEY_CHAR_COS;
    if (c==2) return KEY_CHAR_TAN;
  }
  if (r==8){
    if (c==7) return KEY_CTRL_ALPHA;
    if (c==6) return KEY_CHAR_SQUARE;
    if (c==5) return KEY_CHAR_POW;
    if (c==4) return 5; // KEY_CTRL_EXIT;
    if (c==3) return 2; // KEY_CTRL_DOWN;
    if (c==2) return 3; // KEY_CTRL_RIGHT;
  }
  if (r==9){
    if (c==7) return KEY_CTRL_SHIFT;
    if (c==6) return KEY_CTRL_OPTN;
    if (c==5) return KEY_CTRL_VARS;
    if (c==4) return KEY_CTRL_MENU;
    if (c==3) return 0; // KEY_CTRL_LEFT;
    if (c==2) return 1; // KEY_CTRL_UP;
  }
  if (r==10){
    if (c==7) return KEY_CTRL_F1;
    if (c==6) return KEY_CTRL_F2;
    if (c==5) return KEY_CTRL_F3;
    if (c==4) return KEY_CTRL_F4;
    if (c==3) return KEY_CTRL_F5;
    if (c==2) return KEY_CTRL_F6;
  }
  return 0;
}

