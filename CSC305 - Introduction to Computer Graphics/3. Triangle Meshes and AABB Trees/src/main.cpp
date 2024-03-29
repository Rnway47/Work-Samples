////////////////////////////////////////////////////////////////////////////////
// C++ include
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <utility>

// Utilities for the Assignment
#include "utils.h"

// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION // Do not include this line twice in your project!
#include "stb_image_write.h"

// Shortcut to avoid Eigen:: everywhere, DO NOT USE IN .h
using namespace Eigen;

////////////////////////////////////////////////////////////////////////////////
// Class to store tree
////////////////////////////////////////////////////////////////////////////////
class AABBTree
{
public:
    class Node
    {
    public:
        AlignedBox3d bbox;
        int parent;   // Index of the parent node (-1 for root)
        int left;     // Index of the left child (-1 for a leaf)
        int right;    // Index of the right child (-1 for a leaf)
        int triangle; // Index of the node triangle (-1 for internal nodes)
    };

    std::vector<Node> nodes;
    int root;

    AABBTree() = default;                           // Default empty constructor
    AABBTree(const MatrixXd &V, const MatrixXi &F); // Build a BVH from an existing mesh
};

////////////////////////////////////////////////////////////////////////////////
// Scene setup, global variables
////////////////////////////////////////////////////////////////////////////////
const std::string data_dir = DATA_DIR;
const std::string filename("raytrace.png");
const std::string mesh_filename(data_dir + "bunny.off");

//Camera settings
const double focal_length = 2;
const double field_of_view = 0.7854; //45 degrees
const bool is_perspective = true;
const Vector3d camera_position(0, 0, 2);

// Triangle Mesh
MatrixXd vertices; // n x 3 matrix (n points)
MatrixXi facets;   // m x 3 matrix (m triangles)
AABBTree bvh;

//Material for the object, same material for all objects
const Vector4d obj_ambient_color(0.0, 0.5, 0.0, 0);
const Vector4d obj_diffuse_color(0.5, 0.5, 0.5, 0);
const Vector4d obj_specular_color(0.2, 0.2, 0.2, 0);
const double obj_specular_exponent = 256.0;
const Vector4d obj_reflection_color(0.7, 0.7, 0.7, 0);

// Precomputed (or otherwise) gradient vectors at each grid node
const int grid_size = 20;
std::vector<std::vector<Vector2d>> grid;

//Lights
std::vector<Vector3d> light_positions;
std::vector<Vector4d> light_colors;
//Ambient light
const Vector4d ambient_light(0.2, 0.2, 0.2, 0);

//Fills the different arrays
void setup_scene()
{
    //Loads file
    std::ifstream in(mesh_filename);
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

    //setup tree
    bvh = AABBTree(vertices, facets);

    //Lights
    light_positions.emplace_back(8, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(6, -8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(4, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(2, -8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(0, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(-2, -8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(-4, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);
}

////////////////////////////////////////////////////////////////////////////////
// BVH Code
////////////////////////////////////////////////////////////////////////////////

AlignedBox3d bbox_from_triangle(const Vector3d &a, const Vector3d &b, const Vector3d &c)
{
    AlignedBox3d box;
    box.extend(a);
    box.extend(b);
    box.extend(c);
    return box;
}

// MatrixXd sort_centroid(int axis, MatrixXd centroids){ //sort the centroids matrix based on the given axis index (0,1,2)
//     VectorXd sort_col;
//     sort_col = centroids.col(axis);
//     std::vector<std::pair <double, int>> sort_list; //make it a pair to keep track of index
//     for(int i=0; i<centroids.rows(); ++i){
//         sort_list.push_back(std::make_pair(sort_col[i], i));
//         sort(sort_list.begin(), sort_list.end());
//     }
//     MatrixXd sorted_centroids(centroids.rows(), 3);
//     for (int i=0; i<centroids.rows(); ++i){ //construct the sorted matrix
//         sorted_centroids.row(i) = centroids.row(sort_list[i].second);
//     }
//     return sorted_centroids;
// }

AABBTree::AABBTree(const MatrixXd &V, const MatrixXi &F)
{
    // Compute the centroids of all the triangles in the input mesh
    MatrixXd centroids(F.rows(), V.cols());
    centroids.setZero();
    for (int i = 0; i < F.rows(); ++i)
    {
        for (int k = 0; k < F.cols(); ++k)
        {
            centroids.row(i) += V.row(F(i, k));
        }
        centroids.row(i) /= F.cols();
    }

    // //points for extending bounding box
    // AlignedBox3d bbox;
    // Vector3d mid_point(0, 0, 0);
    // for(int i=0; i<centroids.rows(); ++i){ //bounding box
    //     mid_point = centroids.row(i);
    //     bbox.extend(mid_point);
    // }

    // // start sorting the vertices based on longest axis
    // double x_axis = bbox.max()[0] - bbox.min()[0];
    // double y_axis = bbox.max()[1] - bbox.min()[1];
    // double z_axis = bbox.max()[2] - bbox.min()[2];
    // double max_axis = std::max(std::max(x_axis, y_axis), z_axis);

    // MatrixXd left_set;
    // MatrixXd right_set;
    // VectorXd axis_column;
    // int left_thresh;
    // int right_thresh;

    // if(max_axis == x_axis){ //sort using x_cord
    //     left_thresh = std::ceil(max_axis/2);
    //     left_thresh = std::floor(max_axis/2);
    //     left_set.resize(left_thresh, 3);
    //     right_set.resize(right_thresh, 3);
    //     axis_column = centroids.col(0);
    //     for(int i=0; i<centroids.rows(); i++){
    //         if(axis_column[i] <= left_thresh){
    //             left_set.row(i) = centroids.row(i);
    //         }else{
    //             right_set.row(i) = centroids.row(i);
    //         }
    //     }
    // }else if(max_axis == y_axis){ // ... y_cord
    //     left_thresh = std::ceil(max_axis/2);
    //     left_thresh = std::floor(max_axis/2);
    //     left_set.resize(left_thresh, 3);
    //     right_set.resize(right_thresh, 3);
    //     axis_column = centroids.col(1);
    //     for(int i=0; i<centroids.rows(); i++){
    //         if(axis_column[i] <= left_thresh){
    //             left_set.row(i) = centroids.row(i);
    //         }else{
    //             right_set.row(i) = centroids.row(i);
    //         }
    //     }

    // }else if(max_axis == z_axis){ // ... z_cord
    //     left_thresh = std::ceil(max_axis/2);
    //     left_thresh = std::floor(max_axis/2);
    //     left_set.resize(left_thresh, 3);
    //     right_set.resize(right_thresh, 3);
    //     axis_column = centroids.col(1);
    //     for(int i=0; i<centroids.rows(); i++){
    //         if(axis_column[i] <= left_thresh){
    //             left_set.row(i) = centroids.row(i);
    //         }else{
    //             right_set.row(i) = centroids.row(i);
    //         }
    //     }
    // }
    // TODO
    // Split each set of primitives into 2 sets of roughly equal size,
    // based on sorting the centroids along one direction or another.
}

////////////////////////////////////////////////////////////////////////////////
// Intersection code
////////////////////////////////////////////////////////////////////////////////

double ray_triangle_intersection(const Vector3d &ray_origin, const Vector3d &ray_direction, const Vector3d &a, const Vector3d &b, const Vector3d &c, Vector3d &p, Vector3d &N)
{
    // TODO
    // Compute whether the ray intersects the given triangle.
    // If you have done the parallelogram case, this should be very similar to it.
    Matrix3d inv_A;
    inv_A << b-a, c-a, -ray_direction;
    Vector3d B_con = ray_origin-a;
    Vector3d section_check = inv_A.colPivHouseholderQr().solve(B_con);
    if(section_check[2] > 0 && section_check[0] >= 0 && section_check[1] >= 0 && (section_check[0] + section_check[1]) <= 1){
        p = ray_origin + section_check[2] * ray_direction;
        N = (c-a).cross(b-a).normalized();
        return section_check[2];
    }else{
        return -1;
    }
}

bool ray_box_intersection(const Vector3d &ray_origin, const Vector3d &ray_direction, const AlignedBox3d &box)
{
    // TODO
    // Compute whether the ray intersects the given box.
    // we are not testing with the real surface here anyway.
    Vector3d min_point(0,0,0);
    Vector3d max_point(0,0,0);
    min_point = box.min();
    max_point = box.max();
    double psu_min = 0;
    double psu_max = 0;
    double dis_min = (min_point[0]-ray_origin[0])*(-ray_direction[0]);
    double dis_max = (max_point[0]-ray_origin[0])*(-ray_direction[0]);
    dis_min = std::min(dis_min, dis_max);
    dis_max = std::max(dis_min, dis_max);
    for(int i=1; i<3; ++i){
        psu_min = (min_point[i]-ray_origin[i])*(-ray_direction[i]);
        psu_max = (max_point[i]-ray_origin[i])*(-ray_direction[i]);
        dis_min = std::max(dis_min, std::min(std::min(psu_min, psu_max), dis_max));
        dis_max = std::min(dis_max, std::max(std::max(psu_min, psu_max), dis_min));
    }
    return dis_max > std::max(dis_min, 0.0);
}

//Finds the closest intersecting object returns its index
//In case of intersection it writes into p and N (intersection point and normals)
bool find_nearest_object(const Vector3d &ray_origin, const Vector3d &ray_direction, Vector3d &p, Vector3d &N)
{
    Vector3d tmp_p, tmp_N;
    int closest_index = -1;
    double closest_t = std::numeric_limits<double>::max(); //closest t is "+ infinity"
    Vector3d point_a(0,0,0);
    Vector3d point_b(0,0,0);
    Vector3d point_c(0,0,0);
    for(int i = 0; i < facets.rows(); ++i){
        point_a = vertices.row(facets(i,0));
        point_b = vertices.row(facets(i,1));
        point_c = vertices.row(facets(i,2));
        const double t = ray_triangle_intersection(ray_origin, ray_direction, point_a, point_b, point_c, tmp_p, tmp_N);
        if (t >= 0){
            if (t < closest_t){
                closest_index = i;
                closest_t = t;
                p = tmp_p;
                N = tmp_N;
            }
        }
    }
    // TODO
    // Method (1): Traverse every triangle and return the closest hit.
    // Method (2): Traverse the BVH tree and test the intersection with a
    // triangles at the leaf nodes that intersects the input ray.
    if (closest_index >= 0){
        return true;
    }else{
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Raytracer code
////////////////////////////////////////////////////////////////////////////////
Vector4d shoot_ray(const Vector3d &ray_origin, const Vector3d &ray_direction)
{
    //Intersection point and normal, these are output of find_nearest_object
    Vector3d p, N;

    const bool nearest_object = find_nearest_object(ray_origin, ray_direction, p, N);

    if (!nearest_object)
    {
        // Return a transparent color
        return Vector4d(0, 0, 0, 0);
    }

    // Ambient light contribution
    const Vector4d ambient_color = obj_ambient_color.array() * ambient_light.array();

    // Punctual lights contribution (direct lighting)
    Vector4d lights_color(0, 0, 0, 0);
    for (int i = 0; i < light_positions.size(); ++i)
    {
        const Vector3d &light_position = light_positions[i];
        const Vector4d &light_color = light_colors[i];

        Vector4d diff_color = obj_diffuse_color;

        // Diffuse contribution
        const Vector3d Li = (light_position - p).normalized();
        const Vector4d diffuse = diff_color * std::max(Li.dot(N), 0.0);

        // Specular contribution
        const Vector3d Hi = (Li - ray_direction).normalized();
        const Vector4d specular = obj_specular_color * std::pow(std::max(N.dot(Hi), 0.0), obj_specular_exponent);
        // Vector3d specular(0, 0, 0);

        // Attenuate lights according to the squared distance to the lights
        const Vector3d D = light_position - p;
        lights_color += (diffuse + specular).cwiseProduct(light_color) / D.squaredNorm();
    }

    // Rendering equation
    Vector4d C = ambient_color + lights_color;

    //Set alpha to 1
    C(3) = 1;

    return C;
}

////////////////////////////////////////////////////////////////////////////////

void raytrace_scene()
{
    std::cout << "Simple ray tracer." << std::endl;

    int w = 640;
    int h = 480;
    MatrixXd R = MatrixXd::Zero(w, h);
    MatrixXd G = MatrixXd::Zero(w, h);
    MatrixXd B = MatrixXd::Zero(w, h);
    MatrixXd A = MatrixXd::Zero(w, h); // Store the alpha mask

    // The camera always points in the direction -z
    // The sensor grid is at a distance 'focal_length' from the camera center,
    // and covers an viewing angle given by 'field_of_view'.
    double aspect_ratio = double(w) / double(h);
    //TODO
    double image_y = tan(field_of_view/2) * focal_length;
    double image_x = image_y * aspect_ratio;

    // The pixel grid through which we shoot rays is at a distance 'focal_length'
    const Vector3d image_origin(-image_x, image_y, camera_position[2] - focal_length);
    const Vector3d x_displacement(2.0 / w * image_x, 0, 0);
    const Vector3d y_displacement(0, -2.0 / h * image_y, 0);

    for (unsigned i = 0; i < w; ++i)
    {
        for (unsigned j = 0; j < h; ++j)
        {
            const Vector3d pixel_center = image_origin + (i + 0.5) * x_displacement + (j + 0.5) * y_displacement;

            // Prepare the ray
            Vector3d ray_origin;
            Vector3d ray_direction;

            if (is_perspective)
            {
                // Perspective camera
                ray_origin = camera_position;
                ray_direction = (pixel_center - camera_position).normalized();
            }
            else
            {
                // Orthographic camera
                ray_origin = pixel_center;
                ray_direction = Vector3d(0, 0, -1);
            }

            const Vector4d C = shoot_ray(ray_origin, ray_direction);
            R(i, j) = C(0);
            G(i, j) = C(1);
            B(i, j) = C(2);
            A(i, j) = C(3);
        }
    }

    // Save to png
    write_matrix_to_png(R, G, B, A, filename);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    setup_scene();

    raytrace_scene();
    return 0;
}
