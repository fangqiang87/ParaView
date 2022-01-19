#include <QApplication>
#include <vtkPVPluginLoader.h>

#include "myMainWindow.h"
#include "pqPVApplicationCore.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  pqPVApplicationCore appCore(argc, argv);

  // Create the window which initialize all paraview behaviors
  myMainWindow window;
  window.setWindowTitle("Demo App");

  // Load a configuration XML in order to have access to filters and readers
  appCore.loadConfiguration(qApp->applicationDirPath() + "/../ParaViewFilters.xml");

  window.show();
  return app.exec();
}
