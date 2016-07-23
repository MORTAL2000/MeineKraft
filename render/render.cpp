#include <fstream>
#include <iostream>
#include "render.h"

typedef enum { png, jpg } FileFormat;

// Texture loading order; right, left, top, bottom, back, front
GLuint load_cube_map(std::vector<std::string> faces, FileFormat file_format) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    GLint internalFormat;
    switch (file_format) {
        case png:
            internalFormat = GL_RGBA;
            break;
        case jpg:
            internalFormat = GL_RGB;
            break;
        default:
            internalFormat = GL_RGBA;
    }

    int i = 0;
    for (auto filepath : faces) {
        SDL_Surface *image = IMG_Load(filepath.c_str());
        int width = image->w;
        int height = image->h;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width,
                     height, 0, internalFormat, GL_UNSIGNED_BYTE, image->pixels);
        free(image);
        i++;
    }
    return texture;
}

// Camera combined rotation matrix (y, x) & translation matrix
Mat4<GLfloat> Render::FPSViewRH(Vec3 eye, float pitch, float yaw) {
    float rad = M_PI / 180;
    float cosPitch = cos(pitch * rad);
    float sinPitch = sin(pitch * rad);
    float cosYaw = cos(yaw * rad);
    float sinYaw = sin(yaw * rad);
    Vec3 xaxis = {cosYaw, 0, -sinYaw};
    Vec3 yaxis = {sinYaw * sinPitch, cosPitch, cosYaw * sinPitch};
    Vec3 zaxis = {sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw};
    Mat4<GLfloat> matrix;
    matrix[0][0] = xaxis.x;
    matrix[0][1] = yaxis.x;
    matrix[0][2] = zaxis.x;
    matrix[0][3] = 0.0f;
    matrix[1][0] = xaxis.y;
    matrix[1][1] = yaxis.y;
    matrix[1][2] = zaxis.y;
    matrix[1][3] = 0.0f;
    matrix[2][0] = xaxis.z;
    matrix[2][1] = yaxis.z;
    matrix[2][2] = zaxis.z;
    matrix[2][3] = 0.0f;
    matrix[3][0] = -dot(xaxis, eye);
    matrix[3][1] = -dot(yaxis, eye);
    matrix[3][2] = -dot(zaxis, eye); // GLM says no minus , other's say minus
    matrix[3][3] = 1.0f;
    return matrix;
}

Mat4<GLfloat> perspective_matrix(float z_near, float z_far, float fov,
                            float aspect) {
    float rad = M_PI / 180;
    float tanHalf = tan(fov * rad / 2);
    float a = 1 / (tanHalf * aspect);
    float b = 1 / tanHalf;
    float c = -(z_far + z_near) / (z_far - z_near);
    float d = -(2 * z_far * z_near) / (z_far - z_near);
    Mat4<GLfloat> matrix;
    matrix[0] = {a, 0.0f, 0.0f, 0.0f};
    matrix[1] = {0.0f, b, 0.0f, 0.0f};
    matrix[2] = {0.0f, 0.0f, c, d};
    matrix[3] = {0.0f, 0.0f, -1.0f, 0.0f};
    return matrix;
}

Mat4<GLfloat> Render::scaling_matrix(float scale) {
    Mat4<GLfloat> matrix;
    matrix[0] = {scale, 0.0f, 0.0f, 0.0f};
    matrix[1] = {0.0f, scale, 0.0f, 0.0f};
    matrix[2] = {0.0f, 0.0f, scale, 0.0f};
    matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    return matrix;
}

Mat4<GLfloat> Render::translation_matrix(float x, float y, float z) {
    Mat4<GLfloat> matrix;
    matrix[0] = {1.0f, 0.0f, 0.0f, x};
    matrix[1] = {0.0f, 1.0f, 0.0f, y};
    matrix[2] = {0.0f, 0.0f, 1.0f, z};
    matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    return matrix;
}

// Assumes the file exists and will seg. fault otherwise.
const std::string Render::load_shader_source(std::string filename) {
    std::ifstream ifs(filename);
    return std::string((std::istreambuf_iterator<char>(ifs)),
                              (std::istreambuf_iterator<char>()));
}

// Transformation matrix for the y-plane
Mat4<GLfloat> Render::transformation_matrix_x(float theta) {
    float x = M_PI / 180;
    Mat4<GLfloat> matrix;
    matrix[0] = {1.0f, 0.0f, 0.0f, 0.0f};
    matrix[1] = {0.0f, cosf(theta * x), -sinf(theta * x), 0.0f};
    matrix[2] = {0.0f, sinf(theta * x), cosf(theta * x), 0.0f};
    matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    return matrix;
}

Mat4<GLfloat> Render::transformation_matrix_y(float theta) {
    float x = M_PI / 180;
    Mat4<GLfloat> matrix;
    matrix[0] = {cosf(theta * x), 0.0f, sinf(theta * x), 0.0f};
    matrix[1] = {0.0f, 1.0f, 0.0f, 0.0f};
    matrix[2] = {-sinf(theta * x), 0.0f, cosf(theta * x), 0.0f};
    matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    return matrix;
}

Mat4<GLfloat> Render::transformation_matrix_z(float theta) {
    float x = M_PI / 180;
    Mat4<GLfloat> matrix;
    matrix[0] = {cosf(theta * x), -sinf(theta * x), 0.0f, 0.0f};
    matrix[1] = {sinf(theta * x), cosf(theta * x), 0.0f, 0.0f};
    matrix[2] = {0.0f, 0.0f, 1.0f, 0.0f};
    matrix[3] = {0.0f, 0.0f, 0.0f, 1.0f};
    return matrix;
}

Render::Render(SDL_Window *window): skybox(Cube()) {
    glewExperimental = (GLboolean) true;
    glewInit();
    skybox.scale = 500.0f;

    this->window = window;

    // Load all block & skybox textures
    std::vector<std::string> cube_faces = {"res/blocks/grass/bottom.jpg", "res/blocks/grass/bottom.jpg",
                                           "res/blocks/grass/bottom.jpg", "res/blocks/grass/bottom.jpg",
                                           "res/blocks/grass/bottom.jpg", "res/blocks/grass/bottom.jpg"};
    this->textures[Texture::GRASS] = load_cube_map(cube_faces, jpg);

    std::vector<std::string> skybox_faces = {"res/sky/right.jpg", "res/sky/left.jpg",
                                             "res/sky/top.jpg",   "res/sky/bottom.jpg",
                                             "res/sky/back.jpg",  "res/sky/front.jpg"};
    this->textures[Texture::SKYBOX] = load_cube_map(skybox_faces, jpg);

    // Upload only one cube's quads since the model is the same for all cubes
    Cube cube = Cube();

    GLuint VBO;                         // VBO - vertex buffer object
    glGenBuffers(1, &VBO);              // generate a 'name for the object'
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // bind/make VBO the active object
    glBufferData(GL_ARRAY_BUFFER, cube.byte_size_of_vertices(), NULL,
                 GL_STATIC_DRAW); // reserve a large buffer for cube quads
    this->VBO = VBO;

    auto vector = cube.to_floats();
    glBufferSubData(GL_ARRAY_BUFFER, 0, cube.byte_size_of_vertices(), vector.data());

    auto vertexShaderSource =
            load_shader_source("shaders/block/vertex-shader.glsl");
    auto raw_str = vertexShaderSource.c_str();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &raw_str, NULL);
    glCompileShader(vertexShader);
    GLint vertexShaderStatus;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexShaderStatus);

    auto fragmentShaderSource =
            load_shader_source("shaders/block/fragment-shader.glsl");
    raw_str = fragmentShaderSource.c_str();
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &raw_str, NULL);
    glCompileShader(fragmentShader);
    GLint fragmentShaderStatus;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentShaderStatus);

    printf("Fragment: %i, Vertex: %i\n", fragmentShaderStatus,
           vertexShaderStatus);
    char buffer[512];
    glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
    printf("%s\n", buffer);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);
    this->shader_program = shaderProgram;

    // Bind Vertex Array Object - OpenGL core REQUIRES a VERTEX ARRAY OBJECT in
    // order to render ANYTHING.
    // An object that stores vertex array bindings
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO); // make the VAO the active one ..
    this->VAO = VAO;

    // Then set our vertex attributes pointers, only doable AFTER linking
    GLuint positionAttrib = glGetAttribLocation(shaderProgram, "position");
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(positionAttrib);

    GLuint colorAttrib = glGetAttribLocation(shaderProgram, "vColor");
    glVertexAttribPointer(colorAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void *)offsetof(Vertex, color));
    glEnableVertexAttribArray(colorAttrib);

    // Model
    this->gl_model = glGetUniformLocation(shaderProgram, "model");

    // View / camera space
    this->gl_camera_view = glGetUniformLocation(shaderProgram, "camera_view");

    // Projection
    GLuint projection = glGetUniformLocation(shaderProgram, "projection");
    int height, width;
    SDL_GetWindowSize(this->window, &width, &height);
    float aspect = width / height;
    auto perspective_mat = perspective_matrix(1, -20, 60, aspect);
    glUniformMatrix4fv(projection, 1, GL_FALSE, perspective_mat.data());

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube.byte_size_of_indices(), cube.indices.data(),
                 GL_STATIC_DRAW);

    // Camera
    Vec3 position  = {0.0f, 0.0f, 1.0f};  // cam position
    Vec3 direction = {0.0f, 0.0f, -1.0f}; // position of where the cam is looking
    Vec3 world_up  = {0.0, 1.0f, 0.0f};   // world up
    this->camera = std::make_shared<Camera>(position, direction, world_up);
}

void Render::render_world(const World *world) {
    glEnable(GL_MULTISAMPLE);

    auto transMat_camera_view = FPSViewRH(camera->position, camera->pitch, camera->yaw);

    // Draw
    glUseProgram(shader_program);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.4f, 0.3f, 0.7f, 1.0f);
    glUniformMatrix4fv(camera_view, 1, GL_FALSE, transMat_camera_view.data());

    /** Render skybox **/
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_FRONT);   // cull back face
    glFrontFace(GL_CCW);    // GL_CCW for counter clock-wise
    glBindTexture(GL_TEXTURE_CUBE_MAP, textures[Texture::SKYBOX]);

    // Model
    auto model = Mat4<GLfloat>();
    model = model.translate(skybox.position);
    model = model.scale(skybox.scale);
    glUniformMatrix4fv(gl_model, 1, GL_TRUE, model.data());

    // GOTTA BIND THE VAO TO TELL OPENGL WHERE THE INDICES ARE FROM
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, 0);

    /** Render cubes **/
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK);    // cull back face
    glFrontFace(GL_CCW);    // GL_CCW for counter clock-wise

    for (auto chunk : world->chunks) {
        for (auto cube : chunk.blocks) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, textures[cube.texture]);

            // Model - transform_z * transform_y * transform_x * transform_translation * transform_scaling
            model = Mat4<GLfloat>();
            model = model.translate(cube.position);
            model = model.scale(cube.scale);
            model = model * transformation_matrix_x(cube.theta_x);
            model = model * transformation_matrix_y(cube.theta_y);
            model = model * transformation_matrix_z(cube.theta_z);
            glUniformMatrix4fv(gl_model, 1, GL_TRUE, model.data());

            // GOTTA BIND THE VAO TO TELL OPENGL WHERE THE INDICES ARE FROM
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, 0);
        }
    }
}

Render::~Render() {
    glDeleteVertexArrays(1, (const GLuint *) this->VAO);
    glDeleteBuffers(1, (const GLuint *) this->VBO);
}
