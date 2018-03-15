#include "opengl/GLWrappers.h"
#include <fstream>
#include <vector>
#include <iostream>
//#include "OpenGLUtils.h"

namespace fly
{
  void GLVertexArray::create()
  {
    GL_CHECK(glGenVertexArrays(1, &_id));
  }

  void GLVertexArray::bind()
  {
    GL_CHECK(glBindVertexArray(_id));
  }

  GLVertexArray::~GLVertexArray()
  {
    GL_CHECK(glDeleteVertexArrays(1, &_id));
  }

  void GLShaderProgram::create()
  {
    GL_CHECK(_id = glCreateProgram());
  }

  void GLShaderProgram::addShaderFromFile(std::string fname, ShaderType type, const std::map<std::string, std::regex>& replacements)
  {
    _fnames.push_back(fname);

    GLenum shader_type;
    switch (type)
    {
    case ShaderType::VERTEX:
      shader_type = GL_VERTEX_SHADER;
      break;
    case ShaderType::FRAGMENT:
      shader_type = GL_FRAGMENT_SHADER;
      break;
    case ShaderType::COMPUTE:
      shader_type = GL_COMPUTE_SHADER;
      break;
    case ShaderType::GEOMETRY:
      shader_type = GL_GEOMETRY_SHADER;
      break;
    }
    GLuint shader_id;
    GL_CHECK(shader_id = glCreateShader(shader_type));

    std::string shader_code_str;
    std::ifstream is(fname);
    std::string line;
    while (std::getline(is, line)) {
      shader_code_str += line + "\n";
    }

    is.close();

    for (auto& r : replacements) {
      shader_code_str = std::regex_replace(shader_code_str, r.second, r.first.c_str());
    }

    //shader_code_str = std::regex_replace(shader_code_str, std::regex("LOCALSIZE"), "8");

    const char* code_str_ptr = shader_code_str.c_str();
    GL_CHECK(glShaderSource(shader_id, 1, &code_str_ptr, NULL));
    GL_CHECK(glCompileShader(shader_id));

    GLint compile_status;
    int log_length;
    GL_CHECK(glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status));
    GL_CHECK(glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length));
    if (compile_status == GL_FALSE) {
      std::cout << log_length << std::endl;
      std::vector<char> message(log_length + 1);
      GL_CHECK(glGetShaderInfoLog(shader_id, log_length, NULL, &message[0]));
      std::cout << "Error in " << fname << std::endl;
      std::cout << shader_code_str << std::endl;
      printf("%s\n", &message[0]);
   //   printf("Error in %s: %s\n", fname.c_str(), &message[0]);
      return;
    }

    GL_CHECK(glAttachShader(_id, shader_id));
  }

  void GLShaderProgram::link()
  {
    GL_CHECK(glLinkProgram(_id));

    GLint linkStatus = GL_FALSE;
    glGetProgramiv(_id, GL_LINK_STATUS, &linkStatus);

    if (linkStatus != GL_TRUE) {
      int maxLength;
      int infologLength;
      glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &maxLength);
      char* infoLog = new char[maxLength];
      glGetProgramInfoLog(_id, maxLength, &infologLength, infoLog);
      if (infologLength > 0) {
        std::cerr << "failed to link _program: " << infoLog << std::endl;
        for (auto& f : _fnames) {
          std::cerr << f << std::endl;
        }
      }

      std::cerr << "failed to link _program but error log is empty" << std::endl;

      delete[] infoLog;
    }
  }
  void GLShaderProgram::bind()
  {
    GL_CHECK(glUseProgram(_id));
  }
  GLuint GLShaderProgram::id()
  {
    return _id;
  }
  GLint GLShaderProgram::uniformLocation(const std::string & name)
  {
    if (!_uniformLocations.count(name)) {
      _uniformLocations[name] = glGetUniformLocation(_id, name.c_str());
    }
    return _uniformLocations[name];
  }
  GLShaderProgram::~GLShaderProgram()
  {
    GL_CHECK(glDeleteProgram(_id));
  }

  void GLBuffer::create(GLenum target)
  {
    //std::cout << "GLVertexBuffer::create" << std::endl;
    _target = target;
    GL_CHECK(glGenBuffers(1, &_id));
  }
  void GLBuffer::setData(const void* data, size_t size_in_bytes)
  {
    GL_CHECK(glBufferData(_target, size_in_bytes, data, GL_STATIC_DRAW));
  }
  void GLBuffer::bind()
  {
    GL_CHECK(glBindBuffer(_target, _id));
  }
  GLBuffer::~GLBuffer()
  {
    
    GL_CHECK(glDeleteBuffers(1, &_id));
  }
 
  GLTexture::GLTexture(GLuint id, GLenum target) : _id(id), _target(target)
  {
  }

  GLTexture::GLTexture(GLenum target) : _target(target)
  {
  }
  void GLTexture::create()
  {
    GL_CHECK(glGenTextures(1, &_id));
  }
  void GLTexture::bind()
  {
    GL_CHECK(glBindTexture(_target, _id));
  }
  GLuint GLTexture::id()
  {
    return _id;
  }
  void GLTexture::setMinificationFilter(GLint filter)
  {
    GL_CHECK(glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, filter));
  }
  void GLTexture::setMagnificationFilter(GLint filter)
  {
    GL_CHECK(glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, filter));
  }
  void GLTexture::setCompareMode(GLenum mode, GLenum func)
  {
    GL_CHECK(glTexParameteri(_target, GL_TEXTURE_COMPARE_MODE, mode));
    GL_CHECK(glTexParameteri(_target, GL_TEXTURE_COMPARE_FUNC, func));
  }
  void GLTexture::setData(int width, int height, GLint internal_format, GLenum format, GLenum type, void * data)
  {
    _width = width;
    _height = height;
    GL_CHECK(glTexImage2D(_target, 0, internal_format, width, height, 0, format, type, data));
  }
  void GLTexture::setData(int width, int height, int depth, GLint internal_format, GLenum format, GLenum type, void * data)
  {
    _width = width;
    _height = height;
    _depth = depth;
    GL_CHECK(glTexImage3D(_target, 0, internal_format, width, height, depth, 0, format, type, data));
  }
  void GLTexture::texStorage2D(int width, int height, GLint internal_format, int levels)
  {
    _width = width;
    _height = height;
    GL_CHECK(glTexStorage2D(_target, levels, internal_format, width, height));
  }
  void GLTexture::texStorage3D(int width, int height, int depth, GLint internal_format, int levels)
  {
    _width = width;
    _height = height;
    _depth = depth;
    GL_CHECK(glTexStorage3D(_target, levels, internal_format, width, height, depth));
  }
  GLTexture::~GLTexture()
  {
 //   std::cout << "delete texture" << std::endl;
    GL_CHECK(glDeleteTextures(1, &_id));
  }
  int GLTexture::width()
  {
    return _width;
  }
  int GLTexture::height()
  {
    return _height;
  }
  int GLTexture::depth()
  {
    return _depth;
  }
  void GLFramebuffer::create(int width, int height)
  {
    _width = width;
    _height = height;
    GL_CHECK(glGenFramebuffers(1, &_id));
  }
  void GLFramebuffer::bind()
  {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, _id));
  }
  GLuint GLFramebuffer::id()
  {
    return _id;
  }
  int GLFramebuffer::width()
  {
    return _width;
  }
  int GLFramebuffer::height()
  {
    return _height;
  }
  void GLFramebuffer::setViewport()
  {
    GL_CHECK(glViewport(0, 0, _width, _height));
  }
  void GLFramebuffer::setDepthTexture(const std::shared_ptr<GLTexture>& depth_texture)
  {
    _depthBuffer = depth_texture;
    GL_CHECK(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture != nullptr ? depth_texture->id() : 0, 0));
  }
  void GLFramebuffer::addTexture(const std::shared_ptr<GLTexture>& texture, GLenum attachment)
  {
    _textures.push_back(texture);
    _colorAttachments.push_back(attachment);
    GL_CHECK(glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture->id(), 0));

  }
  void GLFramebuffer::addTextureLayer(const std::shared_ptr<GLTexture>& texture, GLenum attachment, int layer)
  {
    bool found = false;
    for (auto& t : _textures) {
      if (t == texture) {
        found = true;
      }
    }
    if (!found) {
      _textures.push_back(texture);
    }

    GL_CHECK(glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, texture->id(), 0, layer));
  }
  void GLFramebuffer::setRenderbuffer(const std::shared_ptr<GLRenderbuffer>& renderbuffer, GLenum attachment)
  {
    _renderbuffer = renderbuffer;
    GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer->_id));
  }
  void GLFramebuffer::setDrawbuffers(const std::vector<GLenum>& draw_buffers)
  {
    GL_CHECK(glDrawBuffers(draw_buffers.size(), &draw_buffers[0]));
  }
  void GLFramebuffer::setDrawbuffersFromColorAttachments()
  {
    setDrawbuffers(_colorAttachments);
  }
  void GLFramebuffer::checkStatus()
  {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      std::cout << "Framebuffer " << _id << " incomplete" << std::endl;
    }
  }
  std::vector<std::shared_ptr<GLTexture>>& GLFramebuffer::textures()
  {
    return _textures;
  }
  GLFramebuffer::~GLFramebuffer()
  {
  //  std::cout << "delete fb:" << _width << " " << _height << std::endl;
    GL_CHECK(glDeleteFramebuffers(1, &_id));
  }
  void GLRenderbuffer::create()
  {
    GL_CHECK(glGenRenderbuffers(1, &_id));
  }
  void GLRenderbuffer::bind()
  {
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, _id));
  }
  void GLRenderbuffer::allocate(GLenum internal_format, int width, int height)
  {
    GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height));
  }
  GLRenderbuffer::~GLRenderbuffer()
  {
    GL_CHECK(glDeleteRenderbuffers(1, &_id));
  }
  GLShaderStorageBuffer::GLShaderStorageBuffer()
  {
    GL_CHECK(glGenBuffers(1, &_id));
  }
  GLShaderStorageBuffer::~GLShaderStorageBuffer()
  {
    GL_CHECK(glDeleteBuffers(1, &_id));
  }
  void GLShaderStorageBuffer::bindBufferBase(int index)
  {
    GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, _id));
  }
  GLQuery::GLQuery(GLenum target) : _target(target)
  {
    GL_CHECK(glGenQueries(1, &_id));
  }
  GLQuery::~GLQuery()
  {
    GL_CHECK(glDeleteQueries(1, &_id));
  }
  void GLQuery::begin()
  {
    if (_ready) {
      GL_CHECK(glBeginQuery(_target, _id));
    }
  }
  void GLQuery::end()
  {
    if (_ready) {
      GL_CHECK(glEndQuery(_target));
    }
  }
  int GLQuery::getResult()
  {
    GL_CHECK(glGetQueryObjectiv(_id, GL_QUERY_RESULT_AVAILABLE, &_ready));
    if (_ready) {
      GL_CHECK(glGetQueryObjectiv(_id, GL_QUERY_RESULT, &_result));
    }
    return _result;
  }
}