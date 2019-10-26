#include <stdlib.h>
#include <locale.h>
#include <outcurses.h>

static int
panelreel_demo(WINDOW* w, struct panelreel* pr){
  // Press a for a new panel above the current, c for a new one below the current,
  // and b for a new block at arbitrary placement. q quits.
  wattron(w, COLOR_PAIR(COLOR_CYAN));
  int key;
  mvwprintw(w, 1, 1, "a, b, c create tablets, DEL kills tablet, q quits.");
  clrtoeol();
  do{
    wattron(w, COLOR_PAIR(COLOR_RED));
    mvwprintw(w, 2, 2, "%d tablets", panelreel_tabletcount(pr));
    wattron(w, COLOR_PAIR(COLOR_BLUE));
    struct tablet* t;
    key = mvwgetch(w, 3, 2);
    clrtoeol();
    switch(key){
      case 'a': t = add_tablet(pr, NULL, NULL, NULL); break;
      case 'b': t = add_tablet(pr, NULL, NULL, NULL); break;
      case 'c': t = add_tablet(pr, NULL, NULL, NULL); break;
      case KEY_DC: del_active_tablet(pr); break;
      case 'q': break;
      default: wprintw(w, "Unknown key: %c (%d)\n", key, key);
    }
  }while(key != 'q');
  return 0;
}

static int
widecolor_demo(WINDOW* w){
  static const wchar_t* strs[] = {
    L"Война и мир",
    L"Бра́тья Карама́зовы",
    L"Час сэканд-хэнд",
    L"ஸீரோ டிகிரி",
    L"Tonio Kröger",
    L"بين القصرين",
    L"قصر الشوق",
    L"السكرية",
    L"三体",
    L"血的神话: 公元1967年湖南道县文革大屠杀纪实",
    L"三国演义",
    L"紅樓夢",
    L"Hónglóumèng",
    L"红楼梦",
    L"महाभारतम्",
    L"Mahābhāratam",
    L" रामायणम्",
    L"Rāmāyaṇam",
    L"القرآن",
    L"תּוֹרָה",
    L"תָּנָ״ךְ",
    L"Osudy dobrého vojáka Švejka za světové války",
    NULL
  };
  const wchar_t** s;
  int key;

  wattron(w, COLOR_PAIR(COLOR_WHITE));
  mvwaddwstr(w, 0, 0, L"wide chars, multiple colors…");
  int cpair = 16;
  // FIXME change color with each char, and show 6x6x6 structure
  for(s = strs ; *s ; ++s){
    waddch(w, ' ');
    wattron(w, COLOR_PAIR(cpair++));
    waddwstr(w, *s);
  }
  do{
    key = wgetch(w);
  }while(key == ERR);
  wclear(w);
  return 0;
}

static void
print_intro(WINDOW *w){
  int key;

  attron(COLOR_PAIR(COLOR_GREEN));
  mvwprintw(w, 1, 1, "About to run the Outcurses demo. Press any key to continue...\n");
  do{
    key = wgetch(w);
  }while(key == ERR);
}

static int
demo(WINDOW* w){
  print_intro(w);
  widecolor_demo(w);
  panelreel_options popts = {
    .infinitescroll = true,
    .circular = true,
    .headerlines = 4,
    .leftcolumns = 4,
  };
  struct panelreel* pr = create_panelreel(w, &popts);
  if(pr == NULL){
    fprintf(stderr, "Error creating panelreel\n");
    return -1;
  }
  panelreel_demo(w, pr);
  fade(w, 1000);
  if(destroy_panelreel(pr)){
    fprintf(stderr, "Error destroying panelreel\n");
    return -1;
  }
  return 0;
}

int main(void){
  int ret = EXIT_FAILURE;
  WINDOW* w;

  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Coudln't set locale based on user preferences\n");
    return EXIT_FAILURE;
  }
  if((w = init_outcurses(true)) == NULL){
    fprintf(stderr, "Error initializing outcurses\n");
    return EXIT_FAILURE;
  }
  if(demo(w) == 0){
    ret = EXIT_SUCCESS;
  }
  if(stop_outcurses(true)){
    fprintf(stderr, "Error initializing outcurses\n");
    return EXIT_FAILURE;
  }
	return ret;
}
