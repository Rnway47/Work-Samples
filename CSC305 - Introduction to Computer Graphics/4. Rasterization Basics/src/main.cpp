// C++ include
#include <iostream>
#include <string>
#include <vector>

// Utilities for the Assignment
#include "raster.h"

#include <gif.h>
#include <fstream>

#include <Eigen/Geometry>
// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION // Do not include this line twice in your project!
#include "stb_image_write.h"

using namespace std;
using namespace Eigen;

//Image height
const int H = 480;

//Camera settings
const double near_plane = 1.5; //AKA focal length
const double far_plane = near_plane * 100;
const double field_of_view = 0.7854; //45 degrees
const double aspect_ratio = 1.5;
const bool is_perspective = true;
const Vector3d camera_position(0, 0, 3); //e
const Vector3d camera_gaze(0, 0, -1); //g
const Vector3d camera_top(0, 1, 0); //t 

//Object
const std::string data_dir = DATA_DIR;
const std::string mesh_filename(data_dir + "bunny.off");
MatrixXd vertices; // n x 3 matrix (n points)
MatrixXi facets;   // m x 3 matrix (m triangles)

//Material for the object
const Vector3d obj_diffuse_color(0.5, 0.5, 0.5);
const Vector3d obj_specular_color(0.2, 0.2, 0.2);
const double obj_specular_exponent = 256.0;

//Lights
std::vector<Vector3d> light_positions;
std::vector<Vector3d> light_colors;
//Ambient light
const Vector3d ambient_light(0.3, 0.3, 0.3);

//Fills the different arrays
void setup_scene()
{
    //Loads file
    std::ifstream in(mesh_filename);
    if (!in.good())
    {
        std::cerr << "Invalid file " << mesh_filename << std::endl;
        exit(1);
    }
    std::string token;
    in >> token;
    int nv, nf, ne;
    in >> nv >> nf >> ne;
    vertices.resize(nv, 3);
    facets.resize(nf, 3);
    for (int i = 0; i < nv; ++i)
    {
        in >> vertices(i, 0) >> vertices(i, 1) >> vertices(i, 2);
    }
    for (int i = 0; i < nf; ++i)
    {
        int s;
        in >> s >> facets(i, 0) >> facets(i, 1) >> facets(i, 2);
        assert(s == 3);
    }

    //Lights
    light_positions.emplace_back(8, 8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(6, -8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(4, 8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(2, -8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(0, 8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(-2, -8, 0);
    light_colors.emplace_back(16, 16, 16);

    light_positions.emplace_back(-4, 8, 0);
    light_colors.emplace_back(16, 16, 16);
}

void build_uniform(UniformAttributes &uniform)
{
    //TODO: setup uniform
    uniform.color << 1, 0, 0, 1;
    //TODO: setup camera, compute w, u, v
    Vector3d w_vec;
    Vector3d u_vec;
    Vector3d v_vec;
    w_vec = -camera_gaze.normalized();
    u_vec = (camera_top).cross(w_vec).normalized();
    v_vec = w_vec.cross(u_vec);
    //TODO: compute the camera transformation
    Matrix4d cam_setup;
    Matrix4d cam_trans;
    uniform.view << u_vec, v_vec, w_vec, camera_position, 0, 0, 0, 1;
    uniform.view = uniform.view.inverse().eval();
    Matrix4d P;
    //TODO: setup projection matrix
    if (is_perspective)
    {
        Vector3d min_point;//l, b, n
        Vector3d max_point;// r, t, f
        min_point << -tan(field_of_view/2) * near_plane * aspect_ratio, -tan(field_of_view/2) * near_plane, -far_plane;
        max_point << tan(field_of_view/2) * near_plane * aspect_ratio, tan(field_of_view/2) * near_plane, -near_plane;
        Matrix4d orth, inverse;
        orth << 2/(max_point[0]-min_point[0]), 0, 0, -(max_point[0]+min_point[0])/(max_point[0]-min_point[0]),
            0, 2/(max_point[1]-min_point[1]), 0, -(max_point[1]+min_point[1])/(max_point[1]-min_point[1]),
            0, 0, 2/(max_point[2]-min_point[2]), -(min_point[2]+max_point[2])/(max_point[2]-min_point[2]),
            0, 0, 0, 1;
        inverse << -near_plane, 0, 0, 0,
                   0, -near_plane, 0, 0,
                   0, 0, (-near_plane)+(-far_plane), -(-far_plane * -near_plane),
                   0, 0, 1, 0;
        uniform.projective = orth*inverse;
        //TODO setup prespective camera
    }
    else
    {
        Vector3d min_point;
        Vector3d max_point;
        min_point << -tan(field_of_view/2) * near_plane * aspect_ratio, -tan(field_of_view/2) * near_plane, -far_plane;
        max_point << tan(field_of_view/2) * near_plane * aspect_ratio, tan(field_of_view/2) * near_plane, -near_plane;
        P << 2/(max_point[0]-min_point[0]), 0, 0, -(max_point[0]+min_point[0])/(max_point[0]-min_point[0]),
            0, 2/(max_point[1]-min_point[1]), 0, -(max_point[1]+min_point[1])/(max_point[1]-min_point[1]),
            0, 0, 2/(min_point[2]-max_point[2]), -(min_point[2]+max_point[2])/(min_point[2]-max_point[2]),
            0, 0, 0, 1;
        uniform.projective = P;
    }
}

void simple_render(Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic> &frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;

    program.VertexShader = [](const VertexAttributes &va, const UniformAttributes &uniform) {
        //TODO: fill the shader
        VertexAttributes out;
        out.position = uniform.projective * uniform.view * va.position;
        return out;
    };

    program.FragmentShader = [](const VertexAttributes &va, const UniformAttributes &uniform) {
        //TODO: fill the shader, uniform color
        return FragmentAttributes(uniform.color(0), uniform.color(1), uniform.color(2), uniform.color(3));
    };

    program.BlendingShader = [](const FragmentAttributes &fa, const FrameBufferAttributes &previous) {
        //TODO: fill the shader
        return FrameBufferAttributes(fa.color[0] * 255, fa.color[1] * 255, fa.color[2] * 255, fa.color[3] * 255);
    };

    std::vector<VertexAttributes> vertex_attributes;
    //TODO: build the vertex attributes from vertices and facets
    for(int i=0; i<facets.rows(); ++i){
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 0), 0), vertices(facets(i, 0), 1), vertices(facets(i, 0), 2)));
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 1), 0), vertices(facets(i, 1), 1), vertices(facets(i, 1), 2)));
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 2), 0), vertices(facets(i, 2), 1), vertices(facets(i, 2), 2)));
    }
    rasterize_triangles(program, uniform, vertex_attributes, frameBuffer);
}

Matrix4d compute_rotation(const double alpha)
{
    Vector3d barycenter(0,0,0);
    for(int i=0; i<vertices.rows(); ++i){
        barycenter += vertices.row(i);
    }
    barycenter /= vertices.rows();
    Matrix4d tran;
    tran << 1, 0, 0, barycenter(0),
            0, 1, 0, barycenter(1),
            0, 0, 1, barycenter(2),
            0, 0, 0, 1;
    Matrix4d tran_inv;
    tran_inv << 1, 0, 0, -barycenter(0),
                0, 1, 0, -barycenter(1),
                0, 0, 1, -barycenter(2),
                0, 0, 0, 1;

    //TODO: Compute the rotation matrix of angle alpha on the y axis around the object barycenter
    Matrix4d res;
    res << cos(alpha), 0, sin(alpha), 0,
           0, 1, 0, 0,
           -sin(alpha), 0, cos(alpha), 0,
           0, 0, 0, 1;
    Matrix4d result;
    result = tran * res * tran_inv;
    return result;
}

void wireframe_render(const double alpha, Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic> &frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;

    Matrix4d trafo = compute_rotation(alpha);
    uniform.rotation = trafo;
    program.VertexShader = [](const VertexAttributes &va, const UniformAttributes &uniform) {
        //TODO: fill the shader
        VertexAttributes out;
        out.position = uniform.projective * uniform.view * uniform.rotation * va.position;
        return out;
    };

    program.FragmentShader = [](const VertexAttributes &va, const UniformAttributes &uniform) {
        //TODO: fill the shader
        return FragmentAttributes(uniform.color(0), uniform.color(1), uniform.color(2), uniform.color(3));
    };

    program.BlendingShader = [](const FragmentAttributes &fa, const FrameBufferAttributes &previous) {
        //TODO: fill the shader
        return FrameBufferAttributes(fa.color[0] * 255, fa.color[1] * 255, fa.color[2] * 255, fa.color[3] * 255);
    };

    std::vector<VertexAttributes> vertex_attributes;
    for(int i=0; i<facets.rows(); ++i){
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 0), 0), vertices(facets(i, 0), 1), vertices(facets(i, 0), 2))); // a
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 1), 0), vertices(facets(i, 1), 1), vertices(facets(i, 1), 2))); // b
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 1), 0), vertices(facets(i, 1), 1), vertices(facets(i, 1), 2))); // b
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 2), 0), vertices(facets(i, 2), 1), vertices(facets(i, 2), 2))); // c
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 2), 0), vertices(facets(i, 2), 1), vertices(facets(i, 2), 2))); // c
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 0), 0), vertices(facets(i, 0), 1), vertices(facets(i, 0), 2))); // a
        // a, b, b, c, c, a
    }
    //TODO: generate the vertex attributes for the edges and rasterize the lines
    //TODO: use the transformation matrix
    rasterize_lines(program, uniform, vertex_attributes, 0.5, frameBuffer);
}

void get_shading_program(Program &program)
{
    program.VertexShader = [](const VertexAttributes &va, const UniformAttributes &uniform){
        //TODO: transform the position and the normal
        //TODO: compute the correct lighting
        VertexAttributes out;
        Vector4d lights_color;
        Vector4d diff_color(0.5, 0.5, 0.5, 1);
        Vector4d specular_color(0.2, 0.2, 0.2, 1);
        // Vector4d ambient(0.3, 0.3, 0.3, 1);
        Vector4d real_cam(0, 0, 3, 1);
        Vector4d light_position, light_color, Li, specular, D, diffuse;
        double specular_prep;
        lights_color << 0.3, 0.3, 0.3, 1;
        for (int j = 0; j < light_positions.size(); ++j){
            light_position << light_positions[j], 1;
            light_color << light_colors[j], 1;
            Li << (light_position - va.position).normalized();
            // Diffuse contribution
            diffuse = diff_color * std::max(Li.dot(va.normal), 0.0);

            // Specular contribution, use obj_specular_color
            specular_prep = ((real_cam - va.position).normalized() + (light_position - va.position).normalized()).normalized().dot(va.normal);
            specular_prep = std::max(specular_prep, 0.0);
            specular_prep = pow(specular_prep, obj_specular_exponent);
            specular = specular_color * specular_prep;

            // Attenuate lights according to the squared distance to the lights
            D << light_position - va.position;
            lights_color += (diffuse + specular).cwiseProduct(light_color) / D.squaredNorm();
        }
        //  std::cout << lights_color << std::endl;
        lights_color[3] = 1;
        out.position = uniform.projective * uniform.view * va.rotation * va.position;
        out.color = lights_color;
        out.normal = va.rotation.inverse().transpose() * va.normal;
        return out;
    };

    program.FragmentShader = [](const VertexAttributes &va, const UniformAttributes &uniform) {
        //TODO: create the correct fragment
        FragmentAttributes out(va.color(0), va.color(1), va.color(2), va.color(3));
        out.position = va.position;
        return out;    
    };

    program.BlendingShader = [](const FragmentAttributes &fa, const FrameBufferAttributes &previous) {
        //TODO: implement the depth check
        if (fa.position[2] < previous.depth){
            FrameBufferAttributes out(fa.color[0] * 255, fa.color[1] * 255, fa.color[2] * 255, fa.color[3] * 255);
            out.depth = fa.position[2];
            return out;
        }else
            return previous;    
    };
}

void flat_shading(const double alpha, Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic> &frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;
    get_shading_program(program);
    Eigen::Matrix4d trafo = compute_rotation(alpha);
    std::vector<VertexAttributes> vertex_attributes;
    Vector3d a, b, c;
    Vector4d normal;
    for(int i=0; i<facets.rows(); ++i){
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 0), 0), vertices(facets(i, 0), 1), vertices(facets(i, 0), 2)));
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 1), 0), vertices(facets(i, 1), 1), vertices(facets(i, 1), 2)));
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 2), 0), vertices(facets(i, 2), 1), vertices(facets(i, 2), 2)));
        c = vertices.row(facets(i,2));
        b = vertices.row(facets(i,1));
        a = vertices.row(facets(i,0));
        normal << (c-a).cross(b-a).normalized(), 0;
        for(int j=0; j<3; ++j){
            vertex_attributes[i*3+j].rotation << trafo;
            vertex_attributes[i*3+j].normal << normal;
        }
    }
    //TODO: compute the normals
    //TODO: set material colors

    rasterize_triangles(program, uniform, vertex_attributes, frameBuffer);
}

void pv_shading(const double alpha, Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic> &frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;
    get_shading_program(program);

    Eigen::Matrix4d trafo = compute_rotation(alpha);
    int row = vertices.rows();
    Eigen::MatrixXd per_vertex(row, 4);
    per_vertex = MatrixXd::Zero(row, 4);
   // std::cout << per_vertex << std::endl;
    Vector3d a, b, c;
    Vector4d normal;
    for(int j=0; j<facets.rows(); ++j){
        a = vertices.row(facets(j, 0));
        b = vertices.row(facets(j, 1));
        c = vertices.row(facets(j, 2));
        normal << (c-a).cross(b-a).normalized(), 1;
        per_vertex.row(facets(j, 0)) += normal;
        per_vertex.row(facets(j, 1)) += normal;
        per_vertex.row(facets(j, 2)) += normal;
    }
    //TODO: compute the vertex normals as vertex normal average
    std::vector<VertexAttributes> vertex_attributes;
    Vector4d lights_color;
    for(int i=0; i<facets.rows(); ++i){
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 0), 0), vertices(facets(i, 0), 1), vertices(facets(i, 0), 2)));
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 1), 0), vertices(facets(i, 1), 1), vertices(facets(i, 1), 2)));
        vertex_attributes.push_back(VertexAttributes(vertices(facets(i, 2), 0), vertices(facets(i, 2), 1), vertices(facets(i, 2), 2)));
        for(int k=0; k<3; ++k){
            vertex_attributes[i*3+k].normal = (per_vertex.row(facets(i, k))).normalized();
            vertex_attributes[i*3+k].rotation << trafo;
        }
    }
    //TODO: create vertex attributes
    //TODO: set material colors
    rasterize_triangles(program, uniform, vertex_attributes, frameBuffer);
}

int main(int argc, char *argv[])
{
    setup_scene();

    int W = H * aspect_ratio;
    Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic> frameBuffer(W, H);
    vector<uint8_t> image;

    simple_render(frameBuffer);
    framebuffer_to_uint8(frameBuffer, image);
    stbi_write_png("simple.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);

    frameBuffer.setConstant(FrameBufferAttributes());

    wireframe_render(0, frameBuffer);
    framebuffer_to_uint8(frameBuffer, image);
    stbi_write_png("wireframe.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);
    frameBuffer.setConstant(FrameBufferAttributes());

    const char *fileName = "wireframe.gif";
    int delay = 25;
    GifWriter g;
    GifBegin(&g, fileName, frameBuffer.rows(), frameBuffer.cols(), delay);
    for (double i = 0; i <= 6.28319; i = i+0.26)
    {
        frameBuffer.setConstant(FrameBufferAttributes());
        wireframe_render(i, frameBuffer);
        framebuffer_to_uint8(frameBuffer, image);
        GifWriteFrame(&g, image.data(), frameBuffer.rows(), frameBuffer.cols(), delay);
    }
    GifEnd(&g);
    frameBuffer.setConstant(FrameBufferAttributes());

    flat_shading(0, frameBuffer);
    framebuffer_to_uint8(frameBuffer, image);
    stbi_write_png("flat_shading.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);
    frameBuffer.setConstant(FrameBufferAttributes());

    fileName = "flat_Shading.gif";
    GifWriter flat;
    GifBegin(&flat, fileName, frameBuffer.rows(), frameBuffer.cols(), delay);
    for (double i = 0; i <= 6.28319; i = i+0.26)
    {
        frameBuffer.setConstant(FrameBufferAttributes());
        flat_shading(i, frameBuffer);
        framebuffer_to_uint8(frameBuffer, image);
        GifWriteFrame(&flat, image.data(), frameBuffer.rows(), frameBuffer.cols(), delay);
    }
    GifEnd(&flat);
    frameBuffer.setZero();

    pv_shading(0, frameBuffer);
    framebuffer_to_uint8(frameBuffer, image);
    stbi_write_png("pv_shading.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);
    frameBuffer.setConstant(FrameBufferAttributes());

    fileName = "pv_Shading.gif";
    GifWriter pv;
    GifBegin(&pv, fileName, frameBuffer.rows(), frameBuffer.cols(), delay);
    for (double i = 0; i <= 6.28319; i = i+0.26)
    {
        frameBuffer.setConstant(FrameBufferAttributes());
        pv_shading(i, frameBuffer);
        framebuffer_to_uint8(frameBuffer, image);
        GifWriteFrame(&pv, image.data(), frameBuffer.rows(), frameBuffer.cols(), delay);
    }
    GifEnd(&pv);
    frameBuffer.setConstant(FrameBufferAttributes());
    // add the animation
    
    return 0;
}
