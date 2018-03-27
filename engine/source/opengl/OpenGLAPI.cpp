#include <opengl/OpenGLAPI.h>
#include <iostream>
#include <opengl/GLVertexArray.h>
#include <opengl/GLBuffer.h>
#include <Model.h>
#include <Vertex.h>
#include <Mesh.h>
#include <opengl/GLWrappers.h>
#include <Timing.h>
#include <StaticModelRenderable.h>
#include <SOIL/SOIL.h>
#include <opengl/GLTexture.h>

namespace fly
{
  OpenGLAPI::OpenGLAPI()
  {
    glewExperimental = true;
    auto result = glewInit();
    if (result == GLEW_OK) {
      GLint major_version, minor_version;
      GL_CHECK(glGetIntegerv(GL_MAJOR_VERSION, &major_version));
      GL_CHECK(glGetIntegerv(GL_MINOR_VERSION, &minor_version));
      std::cout << "OpenGLAPI::OpenGLAPI(): Initialized GLEW, GL Version: " << major_version << "." << minor_version << std::endl;
    }
    else {
      std::cout << "OpenGLAPI::OpenGLAPI() Failed to initialized GLEW: " << glewGetErrorString(result) << std::endl;
    }
    Timing timing;
    initShaders();
    std::cout << "OpenGLAPI::initShaders() took " << timing << std::endl;
  }
  ZNearMapping OpenGLAPI::getZNearMapping() const
  {
    return ZNearMapping::MINUS_ONE;
  }
  void OpenGLAPI::initShaders()
  {
    _simpleFullscreenQuadShader = std::make_shared<GLShaderProgram>();
    _simpleFullscreenQuadShader->create();
    _simpleFullscreenQuadShader->addShaderFromFile("assets/opengl/vs_screen.glsl", GLShaderProgram::ShaderType::VERTEX);
    _simpleFullscreenQuadShader->addShaderFromFile("assets/opengl/fs_simple.glsl", GLShaderProgram::ShaderType::FRAGMENT);
    _simpleFullscreenQuadShader->link();

    _simpleShader = std::make_shared<GLShaderProgram>();
    _simpleShader->create();
    _simpleShader->addShaderFromFile("assets/opengl/vs_simple.glsl", GLShaderProgram::ShaderType::VERTEX);
    _simpleShader->addShaderFromFile("assets/opengl/fs_simple.glsl", GLShaderProgram::ShaderType::FRAGMENT);
    _simpleShader->link();
  }
  void OpenGLAPI::setViewport(const Vec2u & size) const
  {
    GL_CHECK(glViewport(0, 0, size[0], size[1]));
  }
  void OpenGLAPI::renderFullScreenQuad() const
  {
    _simpleFullscreenQuadShader->bind();
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
  void OpenGLAPI::renderModel(const StaticModelRenderable& smr, const Mat4f& mvp, unsigned lod) const
  {
    smr._modelLods[lod]->_vao->bind();
    _simpleShader->bind();
    GL_CHECK(glUniformMatrix4fv(_simpleShader->uniformLocation("MVP"), 1, false, mvp.ptr()));
    for (const auto& mesh_desc : smr._modelLods[lod]->_meshDesc) {
      GL_CHECK(glActiveTexture(GL_TEXTURE0));
      const auto& diffuse_tex = smr._modelLods[lod]->_materialDesc[mesh_desc._materialIndex]._diffuseTexture;
      if (diffuse_tex) {
        diffuse_tex->bind();
      }
      GL_CHECK(glUniform1i(_simpleShader->uniformLocation("ts"), 0));
      GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_desc._numIndices, GL_UNSIGNED_INT, mesh_desc._indexOffset, mesh_desc._baseVertex));
    }
  }
  OpenGLAPI::ModelData::ModelData(const std::shared_ptr<Model>& model, OpenGLAPI* api)
  {
    _meshDesc.resize(model->getMeshes().size());
    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
    for (unsigned i = 0; i < _meshDesc.size(); i++) {
      auto m = model->getMeshes()[i];
      _meshDesc[i]._baseVertex = static_cast<unsigned>( vertices.size());
      _meshDesc[i]._indexOffset = reinterpret_cast<void*>(indices.size() * sizeof(indices.front()));
      _meshDesc[i]._materialIndex = m->getMaterialIndex();
      _meshDesc[i]._mesh = m.get();
      _meshDesc[i]._numIndices = static_cast<unsigned>(m->getIndices().size());
      vertices.insert(vertices.end(), m->getVertices().begin(), m->getVertices().end());
      indices.insert(indices.end(), m->getIndices().begin(), m->getIndices().end());
    }
    _materialDesc.resize(model->getMaterials().size());
    for (unsigned i = 0; i < model->getMaterials().size(); i++) {
      auto m = model->getMaterials()[i];
      if (m.getDiffusePath() != "") {
        auto tex = SOIL_load_OGL_texture(m.getDiffusePath().c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_COMPRESS_TO_DXT);
        if (tex) {
          _materialDesc[i]._diffuseTexture = std::make_shared<GLTexture>(tex, GL_TEXTURE_2D);
        }
      }
    }
    _vao = std::make_shared<GLVertexArray>();
    _vao->bind();
    _vbo = std::make_shared<GLBuffer>(GL_ARRAY_BUFFER);
    _vbo->setData(&vertices.front(), vertices.size());
    _ibo = std::make_shared<GLBuffer>(GL_ELEMENT_ARRAY_BUFFER);
    _ibo->setData(&indices.front(), indices.size());
    for (unsigned i = 0; i < 5; i++) {
      GL_CHECK(glEnableVertexAttribArray(i));
    }
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _position))));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _normal))));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _uv))));
    GL_CHECK(glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _tangent))));
    GL_CHECK(glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _bitangent))));
  }
}