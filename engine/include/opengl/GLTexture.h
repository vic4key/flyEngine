#ifndef GLTEXTURE_H
#define GLTEXTURE_H

#include <GL/glew.h>
#include <math/FlyMath.h>

namespace fly
{
  class GLTexture
  {
  public:
    GLTexture(GLenum target);
    GLTexture(GLuint id, GLenum target);
    GLuint id() const;
    void bind() const;
    void image2D(GLint level, GLint internal_format, const Vec2u& size, GLint border, GLenum format, GLenum type, const void* data);
    void image3D(GLint level, GLint internal_format, const Vec3u& size, GLint border, GLenum format, GLenum type, const void* data);
    void param(GLenum name, GLint val) const;
    void param(GLenum name, const GLfloat* val) const;
    unsigned width() const;
    unsigned height() const;
    unsigned depth() const;
    ~GLTexture();
  private:
    GLuint _id;
    GLenum _target;
    unsigned _width;
    unsigned _height;
    unsigned _depth;
  };
}

#endif // !GLTEXTURE_H
