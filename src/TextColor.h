// TextColor.h

#ifndef TEXTCOLOR_H_
#define TEXTCOLOR_H_

#include <ostream>
using namespace std;

#define TEXT_YELLOW "[yellow]"
#define TEXT_RED "[red]"
#define TEXT_GREEN "[green]"
#define TEXT_BLUE "[blue]"
#define TEXT_CYAN "[cyan]"
#define TEXT_MAGENTA "[magenta]"
#define TEXT_WHITE "[white]"
#define TEXT_NORMAL ""

#define _RED(msg)  do { \
    cerr << TEXT_RED << msg << TEXT_NORMAL; \
} while (0);

#define _YELLOW(msg)  do { \
    cerr << TEXT_YELLOW << msg << TEXT_NORMAL; \
} while (0);


#endif  // TEXTCOLOR_H_
