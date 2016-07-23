#version 330 core

in vec3 position;
in vec4 vColor;

out vec4 fColor; // This name must match the name in the fragment shader in order to work
out vec3 fTexcoord;

// Model
uniform mat4 model;

// View or a.k.a camera matrix
uniform mat4 camera_view;

// Projection or a.k.a perspecitve matrix
uniform mat4 projection;

void main() {
  gl_Position = projection * camera_view * model * vec4(position, 1.0f);
  fColor = vColor;
  fTexcoord = normalize(position);
}
