// C++ include
#include <iostream>
#include <string>
#include <vector>
#include <cmath> //use it to square numbers
// Utilities for the Assignment
#include "utils.h"
// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION // Do not include this line twice in your project!
#include "stb_image_write.h"
// Shortcut to avoid Eigen:: everywhere, DO NOT USE IN .h
using namespace Eigen;

void raytrace_sphere()
{
    std::cout << "Simple ray tracer, one sphere with orthographic projection" << std::endl;
    const std::string filename("sphere_orthographic.png");
    MatrixXd C = MatrixXd::Zero(800, 800); // Store the color
    MatrixXd A = MatrixXd::Zero(800, 800); // Store the alpha mask
    const Vector3d camera_origin(0, 0, 3);
    const Vector3d camera_view_direction(0, 0, -1);
    // The camera is orthographic, pointing in the direction -z and covering the
    // unit square (-1,1) in x and y
    const Vector3d image_origin(-1, 1, 1);
    const Vector3d x_displacement(2.0 / C.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / C.rows(), 0);
    // Single light source
    const Vector3d light_position(-1, 1, 1);
    for (unsigned i = 0; i < C.cols(); ++i)
    {
        for (unsigned j = 0; j < C.rows(); ++j)
        {
            const Vector3d pixel_center = image_origin + double(i) * x_displacement + double(j) * y_displacement;
            // Prepare the ray
            const Vector3d ray_origin = pixel_center;
            const Vector3d ray_direction = camera_view_direction;
            // Intersect with the sphere
            // NOTE: this is a special case of a sphere centered in the origin and for orthographic rays aligned with the z axis
            Vector2d ray_on_xy(ray_origin(0), ray_origin(1));
            const double sphere_radius = 0.9;
            if (ray_on_xy.norm() < sphere_radius)
            {
                // The ray hit the sphere, compute the exact intersection point
                Vector3d ray_intersection(
                    ray_on_xy(0), ray_on_xy(1),
                    sqrt(sphere_radius * sphere_radius - ray_on_xy.squaredNorm()));
                // Compute normal at the intersection point
                Vector3d ray_normal = ray_intersection.normalized();
                // Simple diffuse model
                C(i, j) = (light_position - ray_intersection).normalized().transpose() * ray_normal;
                // Clamp to zero
                C(i, j) = std::max(C(i, j), 0.);
                // Disable the alpha mask for this pixel
                A(i, j) = 1;
            }
        }
    }
    // Save to png
    write_matrix_to_png(C, C, C, A, filename);
}




void raytrace_parallelogram()
{
    std::cout << "Simple ray tracer, one parallelogram with orthographic projection" << std::endl;
    const std::string filename("plane_orthographic.png");
    MatrixXd C = MatrixXd::Zero(800, 800); // Store the color
    MatrixXd A = MatrixXd::Zero(800, 800); // Store the alpha mask
    const Vector3d camera_origin(0, 0, 3);
    const Vector3d camera_view_direction(0, 0, -1);
    // The camera is orthographic, pointing in the direction -z and covering the unit square (-1,1) in x and y
    const Vector3d image_origin(-1, 1, 1);
    const Vector3d x_displacement(2.0 / C.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / C.rows(), 0);
    // TODO: Parameters of the parallelogram (position of the lower-left corner + two sides)
    const Vector3d pgram_origin(-0.5, -0.5, 0);
    const Vector3d pgram_u(0, 0.7, -10);
    const Vector3d pgram_v(1, 0.4, 0);
    // Single light source
    const Vector3d light_position(-1, 1, 1);
    for (unsigned i = 0; i < C.cols(); ++i)
    {
        for (unsigned j = 0; j < C.rows(); ++j)
        {
            const Vector3d pixel_center = image_origin + double(i) * x_displacement + double(j) * y_displacement;
            // Prepare the ray
            const Vector3d ray_origin = pixel_center;
            const Vector3d ray_direction = camera_view_direction;
            Matrix3d inv_A;
            inv_A << pgram_u, pgram_v, -1 * ray_direction;
            Vector3d B_con = ray_origin - pgram_origin;
            Vector3d section_check = inv_A.colPivHouseholderQr().solve(B_con);
            if (section_check[2] > 0 && 0 <= section_check[0] && section_check[1] >= 0 && section_check[0] <= 1 && section_check[1] <= 1) // check if distance > 0, 0 <= u, v < 1
            {
                // TODO: The ray hit the parallelogram, compute the exact intersection point
                    Vector3d ray_intersection = (ray_origin + section_check[2] * ray_direction); //e + td ray origin + ray direction * distance
                    // TODO: Compute normal at the intersection point
                    Vector3d ray_normal = (pgram_v.cross(pgram_u)).normalized();
                    // Simple diffuse model
                    C(i, j) = (light_position - ray_intersection).normalized().transpose() * ray_normal;
                    // Clamp to zero
                    C(i, j) = std::max(C(i, j), 0.);
                    // Disable the alpha mask for this pixel
                    A(i, j) = 1;
                }
        }
    }
    // Save to png
    write_matrix_to_png(C, C, C, A, filename);
}




void raytrace_perspective()
{
    std::cout << "Simple ray tracer, one parallelogram with perspective projection" << std::endl;
    const std::string filename("plane_perspective.png");
    MatrixXd C = MatrixXd::Zero(800, 800); // Store the color
    MatrixXd A = MatrixXd::Zero(800, 800); // Store the alpha mask

    const Vector3d camera_origin(0, 0, 3);
    const Vector3d camera_view_direction(0, 0, -1);

    // The camera is perspective, pointing in the direction -z and covering the unit square (-1,1) in x and y
    const Vector3d image_origin(-1, 1, 1);
    const Vector3d x_displacement(2.0 / C.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / C.rows(), 0);

    // TODO: Parameters of the parallelogram (position of the lower-left corner + two sides)
    const Vector3d pgram_origin(-0.5, -0.5, 0);
    const Vector3d pgram_u(0, 0.7, -10);
    const Vector3d pgram_v(1, 0.4, 0);

    // Single light source
    const Vector3d light_position(-1, 1, 1);

    for (unsigned i = 0; i < C.cols(); ++i)
    {
        for (unsigned j = 0; j < C.rows(); ++j)
        {
            const Vector3d pixel_center = image_origin + double(i) * x_displacement + double(j) * y_displacement;
            // TODO: Prepare the ray (origin point and direction)
            const Vector3d ray_origin = camera_origin;
            const Vector3d ray_direction = (pixel_center - camera_origin);

            // TODO: Check if the ray intersects with the parallelogram
            Matrix3d inv_A;
            inv_A << pgram_u, pgram_v, -1 * ray_direction;
            Vector3d B_con = ray_origin - pgram_origin;
            Vector3d section_check = inv_A.colPivHouseholderQr().solve(B_con);
            if (section_check[2] > 0 && 0 <= section_check[0] && section_check[1] >= 0 && section_check[0] < 1 && section_check[1] < 1)
            {
                // TODO: The ray hit the parallelogram, compute the exact intersection
                // point
                Vector3d ray_intersection = (ray_origin + section_check[2] * ray_direction);
                // TODO: Compute normal at the intersection point
                Vector3d ray_normal = (pgram_v.cross(pgram_u)).normalized();

                // Simple diffuse model
                C(i, j) = (light_position - ray_intersection).normalized().transpose() * ray_normal;
                // Clamp to zero
                C(i, j) = std::max(C(i, j), 0.);
                // Disable the alpha mask for this pixel
                A(i, j) = 1;
            }
        }
    }
    // Save to png
    write_matrix_to_png(C, C, C, A, filename);
}




void raytrace_shading()
{
    std::cout << "Simple ray tracer, one sphere with different shading" << std::endl;
    const std::string filename("shading.png");
    MatrixXd C = MatrixXd::Zero(800, 800); // Store the color, R
    MatrixXd G = MatrixXd::Zero(800, 800); // G
    MatrixXd B = MatrixXd::Zero(800, 800); // B
    MatrixXd A = MatrixXd::Zero(800, 800); // Store the alpha mask

    const Vector3d camera_origin(0, 0, 3);
    const Vector3d camera_view_direction(0, 0, -1);

    // The camera is perspective, pointing in the direction -z and covering the unit square (-1,1) in x and y
    const Vector3d image_origin(-1, 1, 1);
    const Vector3d x_displacement(2.0 / A.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / A.rows(), 0);

    //Sphere setup
    const Vector3d sphere_center(0, 0, 0);
    const double sphere_radius = 0.9;

    //material params
    const Vector3d diffuse_color(1, 0, 1); 
    const double specular_exponent = 100;
    const Vector3d specular_color(0., 0, 1);

    // Single light source
    const Vector3d light_position(-1, 1, 1);
    double ambient = 0.1;

    for (unsigned i = 0; i < C.cols(); ++i)
    {
        for (unsigned j = 0; j < C.rows(); ++j)
        {
            const Vector3d pixel_center = image_origin + double(i) * x_displacement + double(j) * y_displacement;

            // Prepare the ray
            const Vector3d ray_origin = camera_origin;
            const Vector3d ray_direction = (pixel_center - camera_origin).normalized();

            //check if the ray intersects with the sphere
            double discrim = pow(ray_direction.dot((ray_origin - sphere_center)), 2) -
            (ray_direction.dot(ray_direction)) * (((ray_origin - sphere_center)).dot((ray_origin - sphere_center)) - sphere_radius * sphere_radius); //calculate the discrimant
            if (discrim > 0)
            {
                // The ray hit the sphere, compute the exact intersection point
                double distance = (((-1 * ray_direction)).dot((ray_origin - sphere_center)) - sqrt(discrim)) / (ray_direction.dot(ray_direction)); //we will take the one closer to ray origin
                Vector3d ray_intersection = ray_origin + distance * ray_direction;

                // Compute normal at the intersection point
                Vector3d ray_normal = (ray_intersection - sphere_center).normalized();

                // TODO: Add shading parameter here
                double diffuse_coe = (light_position - ray_intersection).normalized().dot(ray_normal);
                diffuse_coe = std::max(diffuse_coe, 0.);
                Vector3d diffuse = (diffuse_color * (diffuse_coe));
                double specular_coe = ((camera_origin - ray_intersection).normalized() + (light_position - ray_intersection).normalized()).normalized().dot(ray_normal);
                specular_coe = std::max(specular_coe, 0.);
                specular_coe = pow(specular_coe, specular_exponent);
                Vector3d specular = (specular_color * specular_coe);
                // Simple diffuse model
                C(i, j) = ambient + diffuse[0] + specular[0];
                G(i, j) = ambient + diffuse[1] + specular[1];
                B(i, j) = ambient + diffuse[2] + specular[2];

                // Disable the alpha mask for this pixel
                A(i, j) = 1;
            }
        }
    }
    // Save to png
    write_matrix_to_png(C, G, B, A, filename);
}




int main()
{
    raytrace_sphere();
    raytrace_parallelogram();
    raytrace_perspective();
    raytrace_shading();
    return 0;
}















