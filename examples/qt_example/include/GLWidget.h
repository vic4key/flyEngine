#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtOpenGL>
#include <memory>
#include <set>

namespace fly
{
  class OpenGLAPI;
  template<class T>
  class AbstractRenderer;
  class Engine;
  class GameTimer;
  class CameraController;
}

class GLWidget : public QOpenGLWidget
{
public:
  GLWidget();
  ~GLWidget();
protected:
  virtual void initializeGL() override;
  virtual void resizeGL(int width, int height) override;
  virtual void paintGL() override;
  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void keyReleaseEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  std::set<int> _keysPressed;
  inline bool keyPressed(int key) { return _keysPressed.find(key) != _keysPressed.end(); }
private:
  std::unique_ptr<fly::Engine> _engine;
  std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>> _renderer;
  std::unique_ptr<fly::GameTimer> _gameTimer;
  std::unique_ptr<fly::CameraController> _camController;
  void initGame();
};

#endif