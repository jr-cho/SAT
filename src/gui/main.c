#include "../../include/gui/model.h"
#include "../../include/gui/controller.h"
#include "../../include/gui/ui.h"
#include <string.h>

int main(int argc, char *argv[])
{
    GUIModel m;
    model_init(&m);

    if (argc >= 2)
        strncpy(m.target, argv[1], sizeof(m.target) - 1);

    ui_init();
    while (ui_frame(&m));
    ui_shutdown();
    model_destroy(&m);
    return 0;
}
