//project verse, 2017-2021
//by jose pazos perez
//all rights reserved uwu

#pragma once

#ifdef USE_OPENGL

#include "r_opengl.h"

#include "r_windowdata.h"
#include "r_renderdata.h"

#include "r_vertex.h"

namespace Verse::Graphics::GL
{
    void config();

    //INITIALIZATION
    //----------------------------------------
    OpenGL create(WindowData &win);
    namespace Init
    {
        void createContext(OpenGL &gl, WindowData &win);
        void createShaderData(OpenGL &gl);
    
        void createFramebuffers(OpenGL &gl, WindowData &win);
        void createVertexArrays(OpenGL &gl);
        void validateShaderData(OpenGL &gl);
        void createVertexBuffers(OpenGL &gl);
        void createIndexBuffer(OpenGL &gl);
        void configureProperties();
    }
    //----------------------------------------


    //BUFFERS
    //----------------------------------------
    FramebufferData createFramebuffer(Vec2<> size, FramebufferType type);

    template <typename V, std::enable_if_t<Reflection::is_reflectable<V>, bool> = true>
    VertexArrayData createVertexArray() {
        VertexArrayData vao;
        glGenVertexArrays(1, &vao.id_);
        vao.attributes = Vertex::getAttributeDescriptions<V>();
        glCheckError();
        return vao;
    }

    BufferData createBuffer(VertexArrayData &vao);
    BufferData createVertexBuffer(VertexArrayData &vao);
    //----------------------------------------


    //RENDER
    //----------------------------------------
    void renderTest(WindowData &win, RenderData &render);
    //----------------------------------------


    //GUI
    //----------------------------------------
    namespace GUI
    {
        void initImGUI(OpenGL &gl, WindowData &win);
    }
    //----------------------------------------


    //CLEANUP
    //----------------------------------------
    void clean(OpenGL &gl);
    //----------------------------------------
}

#endif
