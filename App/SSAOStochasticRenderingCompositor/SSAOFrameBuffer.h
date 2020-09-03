#pragma once
#include <kvs/Texture2D>
#include <kvs/FrameBufferObject>
#include <kvs/ProgramObject>
#include <kvs/Shader>

namespace local
{

/*===========================================================================*/
/**
 *  @brief  Ensemble averagin buffer class.
 */
/*===========================================================================*/
class SSAOFrameBuffer
{
  
private:
    GLuint m_id;
    kvs::FrameBufferObject m_framebuffer;
    kvs::Texture2D m_color_texture;
    kvs::Texture2D m_position_texture;
    kvs::Texture2D m_normal_texture;
    kvs::Texture2D m_depth_texture;
	kvs::Shader::ShadingModel* m_shader; ///< shading method
	kvs::ProgramObject m_shader_occl_pass;
	const kvs::Shader::ShadingModel& shader() const { return *m_shader; }
	bool m_enable_shading;

private:
    void create_shader_program( const size_t nsamples );
    void create_sampling_points( kvs::Real32 radius, const size_t nsamples );
	void create_framebuffer( const size_t width, const size_t height );
    void update_framebuffer( const size_t width, const size_t height );

public:
	SSAOFrameBuffer();
    void create( const size_t width, const size_t height, kvs::Real32 radius, const size_t nsamples );
	void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void release();
    void clear();
    void bind();
    void unbind();
    void draw();

	kvs::Real32 samplingSphereRadius();
	size_t numberOfSamplingPoints();
};

} // end of namespace kvs
