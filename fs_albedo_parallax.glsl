#version 330 
layout(location = 0) out vec3 fragmentColor;
in vec3 pos_view;
in vec3 normal_view;
in vec2 uv_out;
in vec3 tangent_view;
in vec3 bitangent_view;
// Uniform variables are the same for each shader variation, the compiler will optimize away unused variables anyway
uniform vec3 d_col;
uniform sampler2D ts_diff;
uniform sampler2D ts_alpha;
uniform sampler2D ts_norm;
uniform sampler2D ts_height;
uniform vec3 lpos_cs; // light position view space
uniform vec3 I_in; // light intensity
uniform mat4 v_to_l [4]; // view space to light space
uniform float fs [4]; // frustum_splits
uniform int nfs; // num frustum splits
uniform sampler2DArrayShadow ts_sm;
// Material constants
uniform float ka;
uniform float kd;
uniform float ks;
uniform float s_e;
void main()
{
  vec3 l = normalize(lpos_cs - pos_view); 
  float diffuse = clamp(dot(l, normal_view), 0.f, 1.f);
  float specular = pow(clamp(dot(reflect(-l, normal_view), normalize(-pos_view)), 0.f, 1.f), s_e);
  vec3 albedo = texture(ts_diff, uv_out).rgb;
  fragmentColor = I_in * albedo * (ka + kd * diffuse + ks * specular);
  int index = nfs-1;
  for (int i = nfs-2; i >= 0; i--) {
    index -= int(-pos_view.z < fs[i]);
  }
  vec4 shadow_coord = v_to_l[index] * vec4(pos_view, 1.f);
  shadow_coord.xyz /= shadow_coord.w;
  shadow_coord = shadow_coord * 0.5f + 0.5f;
  shadow_coord.z -= 0.000083f;
  fragmentColor *= 1.f - texture(ts_sm, vec4(shadow_coord.xy, index, shadow_coord.z)) * 0.5f;
}