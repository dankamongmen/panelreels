#include <stdlib.h>
#include <locale.h>
#include <outcurses.h>

#define FADE_MILLISECONDS 1000

static int
panelreel_demo(WINDOW* w, struct panelreel* pr){
  // Press a for a new panel above the current, c for a new one below the current,
  // and b for a new block at arbitrary placement. q quits.
  int pair = COLOR_CYAN;
  wattr_set(w, A_NORMAL, 0, &pair);
  int key;
  mvwprintw(w, 1, 1, "a, b, c create tablets, DEL kills tablet, q quits.");
  clrtoeol();
  do{
    pair = COLOR_RED;
    wattr_set(w, A_NORMAL, 0, &pair);
    mvwprintw(w, 2, 2, "%d tablets", panelreel_tabletcount(pr));
    pair = COLOR_BLUE;
    wattr_set(w, A_NORMAL, 0, &pair);
    key = mvwgetch(w, 3, 2);
    clrtoeol();
    switch(key){
      case 'a': add_tablet(pr, NULL, NULL, NULL); break;
      case 'b': add_tablet(pr, NULL, NULL, NULL); break;
      case 'c': add_tablet(pr, NULL, NULL, NULL); break;
      case KEY_DC: del_active_tablet(pr); break;
      case 'q': break;
      default: wprintw(w, "Unknown key: %c (%d)\n", key, key);
    }
  }while(key != 'q');
  return 0;
}

// Much of this text comes from http://kermitproject.org/utf8.html
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
    L"Σίβνλλα τί ϴέλεις; respondebat illa: άπο ϴανεΐν ϴέλω",
    L"﻿काचं शक्नोम्यत्तुम् । नोपहिनस्ति माम्",
    L"kācaṃ śaknomyattum; nopahinasti mām",
    L"ὕαλον ϕαγεῖν δύναμαι· τοῦτο οὔ με βλάπτει",
    L"Μπορώ να φάω σπασμένα γυαλιά χωρίς να πάθω τίποτα",
    L"Μπορῶ νὰ φάω σπασμένα γυαλιὰ χωρὶς νὰ πάθω τίποτα",
    L"Vitrum edere possum; mihi non nocet",
    L"Je puis mangier del voirre. Ne me nuit",
    L"Je peux manger du verre, ça ne me fait pas mal",
    L"Pòdi manjar de veire, me nafrariá pas",
    L"J'peux manger d'la vitre, ça m'fa pas mal",
    L"Dji pou magnî do vêre, çoula m' freut nén må",
    L"Ch'peux mingi du verre, cha m'foé mie n'ma",
    L"Mwen kap manje vè, li pa blese'm",
    L"Kristala jan dezaket, ez dit minik ematen",
    L"Puc menjar vidre, que no em fa mal",
    L"Puedo comer vidrio, no me hace daño",
    L"Puedo minchar beire, no me'n fa mal",
    L"Eu podo xantar cristais e non cortarme",
    L"Posso comer vidro, não me faz mal",
    L"Posso comer vidro, não me machuca",
    L"M' podê cumê vidru, ca ta maguâ-m'",
    L"Ami por kome glas anto e no ta hasimi daño",
    L"Posso mangiare il vetro e non mi fa male",
    L"Sôn bôn de magnà el véder, el me fa minga mal",
    L"Me posso magna' er vetro, e nun me fa male",
    L"M' pozz magna' o'vetr, e nun m' fa mal",
    L"Mi posso magnare el vetro, no'l me fa mae",
    L"Pòsso mangiâ o veddro e o no me fà mâ",
    L"Puotsu mangiari u vitru, nun mi fa mali",
    L"Jau sai mangiar vaider, senza che quai fa donn a mai",
    L"Pot să mănânc sticlă și ea nu mă rănește",
    L"Mi povas manĝi vitron, ĝi ne damaĝas min",
    L"Mý a yl dybry gwéder hag éf ny wra ow ankenya",
    L"Dw i'n gallu bwyta gwydr, 'dyw e ddim yn gwneud dolur i mi",
    L"Foddym gee glonney agh cha jean eh gortaghey mee",
    L"᚛᚛ᚉᚑᚅᚔᚉᚉᚔᚋ ᚔᚈᚔ ᚍᚂᚐᚅᚑ ᚅᚔᚋᚌᚓᚅᚐ",
    L"Con·iccim ithi nglano. Ním·géna",
    L"Is féidir liom gloinne a ithe. Ní dhéanann sí dochar ar bith dom",
    L"Ithim-sa gloine agus ní miste damh é",
    L"S urrainn dhomh gloinne ithe; cha ghoirtich i mi",
    L"ᛁᚳ᛫ᛗᚨᚷ᛫ᚷᛚᚨᛋ᛫ᛖᚩᛏᚪᚾ᛫ᚩᚾᛞ᛫ᚻᛁᛏ᛫ᚾᛖ᛫ᚻᛖᚪᚱᛗᛁᚪᚧ᛫ᛗᛖ",
    L"Ic mæg glæs eotan ond hit ne hearmiað me",
    L"Ich canne glas eten and hit hirtiþ me nouȝt",
    L"I can eat glass and it doesn't hurt me",
    L"[aɪ kæn iːt glɑːs ænd ɪt dɐz nɒt hɜːt miː] (Received Pronunciation",
    L"⠊⠀⠉⠁⠝⠀⠑⠁⠞⠀⠛⠇⠁⠎⠎⠀⠁⠝⠙⠀⠊⠞⠀⠙⠕⠑⠎⠝⠞⠀⠓⠥⠗⠞⠀⠍",
    L"Mi kian niam glas han i neba hot mi",
    L"Ah can eat gless, it disnae hurt us",
    L"𐌼𐌰𐌲 𐌲𐌻𐌴𐍃 𐌹̈𐍄𐌰𐌽, 𐌽𐌹 𐌼𐌹𐍃 𐍅𐌿 𐌽𐌳𐌰𐌽 𐌱𐍂𐌹𐌲𐌲𐌹𐌸",
    L"ᛖᚴ ᚷᛖᛏ ᛖᛏᛁ ᚧ ᚷᛚᛖᚱ ᛘᚾ ᚦᛖᛋᛋ ᚨᚧ ᚡᛖ ᚱᚧᚨ ᛋᚨ",
    L"Ek get etið gler án þess að verða sár",
    L"Eg kan eta glas utan å skada meg",
    L"Jeg kan spise glass uten å skade meg",
    L"Eg kann eta glas, skaðaleysur",
    L"Ég get etið gler án þess að meiða mig",
    L"Jag kan äta glas utan att skada mig",
    L"Jeg kan spise glas, det gør ikke ondt på mig",
    L"Æ ka æe glass uhen at det go mæ naue",
    L"Ik kin glês ite, it docht me net sear",
    L"Ik kan glas eten, het doet mĳ geen kwaad",
    L"Iech ken glaas èèse, mer 't deet miech jing pieng",
    L"Ek kan glas eet, maar dit doen my nie skade nie",
    L"Ech kan Glas iessen, daat deet mir nët wei",
    L"Ich kann Glas essen, ohne mir zu schaden",
    L"Ich kann Glas verkasematuckeln, ohne dattet mich wat jucken tut",
    L"Isch kann Jlaas kimmeln, uuhne datt mich datt weh dääd",
    L"Ich koann Gloos assn und doas dudd merr ni wii",
    L"Iech konn glaasch voschbachteln ohne dass es mir ebbs daun doun dud",
    L"'sch kann Glos essn, ohne dass'sch mer wehtue",
    L"Isch konn Glass fresse ohne dasses mer ebbes ausmache dud",
    L"I kå Glas frässa, ond des macht mr nix",
    L"I ka glas eassa, ohne dass mar weh tuat",
    L"I koh Glos esa, und es duard ma ned wei",
    L"I kaun Gloos essen, es tuat ma ned weh",
    L"Ich chan Glaas ässe, das schadt mir nöd",
    L"Ech cha Glâs ässe, das schadt mer ned",
    L"Meg tudom enni az üveget, nem lesz tőle bajom",
    L"Voin syödä lasia, se ei vahingoita minua",
    L"Sáhtán borrat lása, dat ii leat bávččas",
    L"Мон ярсан суликадо, ды зыян эйстэнзэ а ули",
    L"Mie voin syvvä lasie ta minla ei ole kipie",
    L"Minä voin syvvä st'oklua dai minule ei ole kibie",
    L"Ma võin klaasi süüa, see ei tee mulle midagi",
    L"Es varu ēst stiklu, tas man nekaitē",
    L"Aš galiu valgyti stiklą ir jis manęs nežeidži",
    L"Mohu jíst sklo, neublíží mi",
    L"Môžem jesť sklo. Nezraní ma",
    L"Mogę jeść szkło i mi nie szkodzi",
    L"Lahko jem steklo, ne da bi mi škodovalo",
    L"Ja mogu jesti staklo, i to mi ne šteti",
    L"Ја могу јести стакло, и то ми не штети",
    L"Можам да јадам стакло, а не ме штета",
    L"Я могу есть стекло, оно мне не вредит",
    L"Я магу есці шкло, яно мне не шкодзіць",
    L"Ja mahu jeści škło, jano mne ne škodzić",
    L"Я можу їсти скло, і воно мені не зашкодить",
    L"Мога да ям стъкло, то не ми вреди",
    L"მინას ვჭამ და არა მტკივა",
    L"Կրնամ ապակի ուտել և ինծի անհանգիստ չըներ",
    L"Unë mund të ha qelq dhe nuk më gjen gjë",
    L"Cam yiyebilirim, bana zararı dokunmaz",
    L"جام ييه بلورم بڭا ضررى طوقونم",
    L"Алам да бар, пыяла, әмма бу ранит мине",
    L"Men shisha yeyishim mumkin, ammo u menga zarar keltirmaydi",
    L"Мен шиша ейишим мумкин, аммо у менга зарар келтирмайди",
    L"আমি কাঁচ খেতে পারি, তাতে আমার কোনো ক্ষতি হয় না",
    L"मी काच खाऊ शकतो, मला ते दुखत नाही",
    L"ನನಗೆ ಹಾನಿ ಆಗದೆ, ನಾನು ಗಜನ್ನು ತಿನಬಹು",
    L"मैं काँच खा सकता हूँ और मुझे उससे कोई चोट नहीं पहुंचती",
    L"എനിക്ക് ഗ്ലാസ് തിന്നാം. അതെന്നെ വേദനിപ്പിക്കില്ല",
    L"நான் கண்ணாடி சாப்பிடுவேன், அதனால் எனக்கு ஒரு கேடும் வராது",
    L"నేను గాజు తినగలను మరియు అలా చేసినా నాకు ఏమి ఇబ్బంది లే",
    L"මට වීදුරු කෑමට හැකියි. එයින් මට කිසි හානියක් සිදු නොවේ",
    L"میں کانچ کھا سکتا ہوں اور مجھے تکلیف نہیں ہوتی",
    L"زه شيشه خوړلې شم، هغه ما نه خوږو",
    L".من می توانم بدونِ احساس درد شيشه بخور",
    L"أنا قادر على أكل الزجاج و هذا لا يؤلمني",
    L"Nista' niekol il-ħġieġ u ma jagħmilli xejn",
    L"אני יכול לאכול זכוכית וזה לא מזיק לי",
    L"איך קען עסן גלאָז און עס טוט מיר נישט װײ",
    L"Metumi awe tumpan, ɜnyɜ me hwee",
    L"Inā iya taunar gilāshi kuma in gamā lāfiyā",
    L"إِنا إِىَ تَونَر غِلَاشِ كُمَ إِن غَمَا لَافِىَ",
    L"Mo lè je̩ dígí, kò ní pa mí lára",
    L"Nakokí kolíya biténi bya milungi, ekosála ngáí mabé tɛ́",
    L"Naweza kula bilauri na sikunyui",
    L"Saya boleh makan kaca dan ia tidak mencederakan saya",
    L"Kaya kong kumain nang bubog at hindi ako masaktan",
    L"Siña yo' chumocho krestat, ti ha na'lalamen yo'",
    L"Au rawa ni kana iloilo, ia au sega ni vakacacani kina",
    L"Aku isa mangan beling tanpa lara",
    L"က္ယ္ဝန္‌တော္‌၊က္ယ္ဝန္‌မ မ္ယက္‌စားနုိင္‌သည္‌။ ၎က္ရောင္‌့ ထိခုိက္‌မ္ဟု မရ္ဟိပာ။ (9",
    L"ကျွန်တော် ကျွန်မ မှန်စားနိုင်တယ်။ ၎င်းကြောင့် ထိခိုက်မှုမရှိပါ။ (9",
    L"Tôi có thể ăn thủy tinh mà không hại gì",
    L"些 𣎏 世 咹 水 晶 𦓡 空 𣎏 害",
    L"ខ្ញុំអាចញុំកញ្ចក់បាន ដោយគ្មានបញ្ហា",
    L"ຂອ້ຍກິນແກ້ວໄດ້ໂດຍທີ່ມັນບໍ່ໄດ້ເຮັດໃຫ້ຂອ້ຍເຈັບ",
    L"ฉันกินกระจกได้ แต่มันไม่ทำให้ฉันเจ็",
    L"Би шил идэй чадна, надад хортой би",
    L"ᠪᠢ ᠰᠢᠯᠢ ᠢᠳᠡᠶᠦ ᠴᠢᠳᠠᠨᠠ ᠂ ᠨᠠᠳᠤᠷ ᠬᠣᠤᠷᠠᠳᠠᠢ ᠪᠢᠰ",
    L"﻿म काँच खान सक्छू र मलाई केहि नी हुन्‍न्",
    L"ཤེལ་སྒོ་ཟ་ནས་ང་ན་གི་མ་རེད",
    L"我能吞下玻璃而不伤身体",
    L"我能吞下玻璃而不傷身體",
    L"Góa ē-tàng chia̍h po-lê, mā bē tio̍h-siong",
    L"私はガラスを食べられます。それは私を傷つけません",
    L"나는 유리를 먹을 수 있어요. 그래도 아프지 않아",
    L"Mi save kakae glas, hemi no save katem mi",
    L"Hiki iaʻu ke ʻai i ke aniani; ʻaʻole nō lā au e ʻeha",
    L"E koʻana e kai i te karahi, mea ʻā, ʻaʻe hauhau",
    L"ᐊᓕᒍᖅ ᓂᕆᔭᕌᖓᒃᑯ ᓱᕋᙱᑦᑐᓐᓇᖅᑐ",
    L"Naika məkmək kakshət labutay, pi weyk ukuk munk-sik nay",
    L"Tsésǫʼ yishą́ągo bííníshghah dóó doo shił neezgai da",
    L"mi kakne le nu citka le blaci .iku'i le se go'i na xrani m",
    L"Ljœr ye caudran créneþ ý jor cẃran",
    NULL
  };
  const wchar_t** s;
  int count = COLORS;
  outcurses_rgb* palette;
  int key;

  palette = malloc(sizeof(*palette) * count);
  retrieve_palette(count, palette, NULL, true);
  int pair = COLOR_WHITE + (COLORS * COLOR_WHITE);
  wattr_set(w, A_NORMAL, 0, &pair);
  mvwaddwstr(w, 0, 0, L"wide chars, multiple colors…");
  int cpair = 16;
  // FIXME show 6x6x6 color structure
  for(s = strs ; *s ; ++s){
    waddch(w, ' ');
    size_t idx;
    for(idx = 0 ; idx < wcslen(*s) ; ++idx){
      pair = cpair++;
      // wattr_set(w, A_NORMAL, 0, &pair);
      cchar_t wch;
      setcchar(&wch, &(*s)[idx], A_NORMAL, 0, &pair);
      wadd_wch(w, &wch);
    }
  }
  fadein(w, count, palette, FADE_MILLISECONDS);
  free(palette);
  do{
    key = wgetch(w);
  }while(key == ERR);
  wclear(w);
  return 0;
}

static void
print_intro(WINDOW *w){
  int key, pair;

  pair = COLOR_GREEN;
  wattr_set(w, A_NORMAL, 0, &pair);
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
    .borderpair = (COLORS * (COLOR_MAGENTA + 1)) + 1,
    .borderattr = A_NORMAL,
  };
  int r, g, b, pair;
  int f, bg;
  pair = popts.borderpair;
  extended_pair_content(pair, &f, &bg);
  extended_color_content(bg, &r, &g, &b);
  struct panelreel* pr = create_panelreel(w, &popts);
  if(pr == NULL){
    fprintf(stderr, "Error creating panelreel\n");
    return -1;
  }
  panelreel_demo(w, pr);
  fadeout(w, FADE_MILLISECONDS);
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
