#include <QApplication>
#include "viewer_window.hpp"
#include <imglib/ppm.hpp>
#include <imglib/filters.hpp>

int main(int argc, char** argv)
{   
    QApplication app(argc, argv);
    ViewerWindow window;
    window.show();
    return app.exec();
}