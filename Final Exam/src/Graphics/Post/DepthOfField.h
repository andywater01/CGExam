#pragma once

#include "Graphics/Post/PostEffect.h"

class DepthOfField : public PostEffect
{
public:
	//Initializes framebuffer
	void Init(unsigned width, unsigned height) override;

	//Applies effect to this buffer
	void ApplyEffect(PostEffect* buffer) override;

	void Reshape(unsigned width, unsigned height) override;

	//Getters
	float GetDepth() const;

	float GetDownscale() const;
	float GetThreshold() const;
	unsigned GetPasses() const;

	//Setters
	void SetDepth(float intensity);
	
	void SetDownscale(float downscale);
	void SetThreshold(float threshold);
	void SetPasses(unsigned passes);

private:
	float _depth = 0.7f;
	float _downscale = 2.0f;
	float _threshold = 0.5f;
	unsigned _passes = 10;
	glm::vec2 _pixelSize;
};