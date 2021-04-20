#include "DepthOfField.h"

void DepthOfField::Init(unsigned width, unsigned height)
{
    int index = int(_buffers.size());
    _buffers.push_back(new Framebuffer());
    _buffers[index]->AddColorTarget(GL_RGBA8);
    _buffers[index]->AddDepthTarget();
    _buffers[index]->Init(width, height);

    //Set up shaders
    index = int(_shaders.size());
    _shaders.push_back(Shader::Create());
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
    _shaders[index]->LoadShaderPartFromFile("shaders/Post/Depth_Of_Field_frag.glsl", GL_FRAGMENT_SHADER);
    _shaders[index]->Link();
    index++;
    _shaders.push_back(Shader::Create());
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
    _shaders[index]->Link();
    index++;
    _shaders.push_back(Shader::Create());
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
    _shaders[index]->LoadShaderPartFromFile("shaders/Post/BlurHorizontal_frag.glsl", GL_FRAGMENT_SHADER);
    _shaders[index]->Link();
    index++;
    _shaders.push_back(Shader::Create());
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
    _shaders[index]->LoadShaderPartFromFile("shaders/Post/BlurVertical_frag.glsl", GL_FRAGMENT_SHADER);
    _shaders[index]->Link();

}

void DepthOfField::ApplyEffect(PostEffect* buffer)
{
    BindShader(0);
    _shaders[0]->SetUniform("u_depth", _depth);

    buffer->BindColorAsTexture(0, 0, 0);

    _buffers[0]->RenderToFSQ();

    buffer->UnbindTexture(0);

    UnbindShader();

    //Draws previous buffer to first render target
    BindShader(1);

    buffer->BindColorAsTexture(0, 0, 0);

    _buffers[0]->RenderToFSQ();

    buffer->UnbindTexture(0);

    UnbindShader();


    //Performs high pass on the first render target
    BindShader(2);
    _shaders[1]->SetUniform("uThreshold", _threshold);

    BindColorAsTexture(0, 0, 0);

    _buffers[1]->RenderToFSQ();

    UnbindTexture(0);

    UnbindShader();


    //Computes blur, vertical and horizontal
    for (unsigned i = 0; i < _passes; i++)
    {
        //Horizontal pass
        BindShader(3);
        _shaders[2]->SetUniform("uPixelSize", _pixelSize.x);

        BindColorAsTexture(1, 0, 0);

        _buffers[2]->RenderToFSQ();

        UnbindTexture(0);

        UnbindShader();

        //Vertical pass
        BindShader(3);
        _shaders[3]->SetUniform("uPixelSize", _pixelSize.y);

        BindColorAsTexture(2, 0, 0);

        _buffers[1]->RenderToFSQ();

        UnbindTexture(0);

        UnbindShader();
    }


    //Composite the scene and the bloom
    BindShader(4);

    buffer->BindColorAsTexture(0, 0, 0);
    BindColorAsTexture(1, 0, 1);

    _buffers[0]->RenderToFSQ();

    UnbindTexture(1);
    UnbindTexture(0);

    UnbindShader();

}

float DepthOfField::GetDepth() const
{
    return _depth;
}

void DepthOfField::SetDepth(float intensity)
{
    _depth = intensity;
}


void DepthOfField::Reshape(unsigned width, unsigned height)
{
    _buffers[0]->Reshape(width, height);
    _buffers[1]->Reshape(unsigned(width / _downscale), unsigned(height / _downscale));
    _buffers[2]->Reshape(unsigned(width / _downscale), unsigned(height / _downscale));
    _buffers[3]->Reshape(width, height);
}

float DepthOfField::GetDownscale() const
{
    return _downscale;
}

float DepthOfField::GetThreshold() const
{
    return _threshold;
}

unsigned DepthOfField::GetPasses() const
{
    return _passes;
}

void DepthOfField::SetDownscale(float downscale)
{
    _downscale = downscale;
    Reshape(_buffers[0]->_width, _buffers[0]->_height);
}

void DepthOfField::SetThreshold(float threshold)
{
    _threshold = threshold;
}

void DepthOfField::SetPasses(unsigned passes)
{
    _passes = passes;
}