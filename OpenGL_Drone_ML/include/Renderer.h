#pragma once

#include "GLError.h"
#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"

// Some people like to make this a singleton so there's only one instance
class Renderer 
{
    public:
        void Clear() const;
        void Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const;
};