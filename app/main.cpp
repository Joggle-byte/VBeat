#include "include/app.hpp"


App* app;

int main() {
    app = new App();

    return app->main();
}