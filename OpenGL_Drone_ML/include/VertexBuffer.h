#pragma once

// m_RendererID keeps track of object / buffer IDs
class VertexBuffer
{
    private:
        unsigned int m_RendererID;
    public:
        VertexBuffer(const void* data, unsigned int size);
        ~VertexBuffer();

        void Bind() const;
        void Unbind() const;
};