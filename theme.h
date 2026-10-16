#ifndef THEME_H
#define THEME_H

class QApplication;
class QStyle;

namespace Theme {

void apply(QApplication &app);
bool dark();
QStyle *style();

}

#endif // THEME_H
