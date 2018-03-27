#include <qapplication.h>
#include <GLWidget.h>
#include <Engine.h>

int main(int argc, char* argv[])
{

#ifdef _WINDOWS
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif 
#endif
  QApplication app(argc, argv);

  GLWidget gl_widget;
  gl_widget.resize(1024, 768);
  gl_widget.show();

  return app.exec();
}