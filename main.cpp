
#include <vector>
#include <iostream>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model* model = NULL;
float* shadowbuffer = NULL;
const int width = 800;
const int height = 800;



Vec3f light_dir(1, 1, 1);
Vec3f       eye(1, 1, 3);
Vec3f    center(0, 0, 0);
Vec3f        up(0, 1, 0);

//  shadow
class IShadowShader {
    struct ShadowShader : public IShader {
        //Vec3f varying_intensity;

        virtual Vec4f vertex(int iface, int nthvert) {
            Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
            gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;          // transform it to screen coordinates
            //varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir);
            return gl_Vertex;
        }

        virtual bool fragment(Vec3f bar, TGAColor& color) {
            //float intensity = varying_intensity * bar;
            //color = TGAColor(255, 255, 255)*intensity; // well duh
            color = TGAColor(255, 255, 255);
            return false;                              // no, we do not discard this pixel
        }
    };
    struct PhoneShader : public IShader {
        Vec3f varying_intensity; // written by vertex shader, read by fragment shader
        Vec2f uv_coords[3];
        mat<4, 4, float> uniform_Mshadow;
        mat<4, 4, float> uniform_M;
        mat<4, 4, float> uniform_MIT;
        mat<3, 3, float> varying_tri;
        virtual Vec4f vertex(int iface, int nthvert) {
            Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
            gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;     // transform it to screen coordinates
            varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir); // get diffuse lighting intensity
            varying_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
            return gl_Vertex;
        }

        virtual bool fragment(Vec3f bar, TGAColor& color) {
            Vec4f sb_p = uniform_Mshadow * embed<4>(varying_tri * bar); // corresponding point in the shadow buffer
            sb_p = sb_p / sb_p[3];
            int idx = int(sb_p[0]) + int(sb_p[1]) * width; // index in the shadowbuffer array
            float shadow = .3 + .7 * (shadowbuffer[idx] < sb_p[2]+15.); // magic coeff to avoid z-fighting

            Vec2f pixeluv = uv_coords[0] * bar.x + uv_coords[1] * bar.y + uv_coords[2] * bar.z;
            TGAColor pixcolor = model->diffuse(pixeluv);

            Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(pixeluv))).normalize();  //经过Model变化后的顶点的法线
            Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize(); //光线经过Model变化
            Vec3f r = (n * (n * l * 2.f) - l).normalize();   // reflected light
            float spec = pow(std::max(r.z, 0.0f), model->specular(pixeluv));
            float intensity = std::max(0.f, n * l);
            //float intensity = varying_intensity * bar;   // interpolate intensity for the current pixel

            for (int i = 0; i < 3; i++) {
                color[i] = 5 + pixcolor[i] * (intensity + 0.6 * spec)*shadow;// 5为环境光照，intensity对应漫反射，spec对应高光
            }
            //color = TGAColor(255, 255, 255)*intensity; // well duh
            return false;                              // no, we do not discard this pixel
        }
    };
public:
    IShadowShader() {

        model = new Model("obj/african_head.obj");

        float* zbuffer = new float[width * height];
        shadowbuffer = new float[width * height];
        for (int i = width * height; --i; ) {
            zbuffer[i] = shadowbuffer[i] = -std::numeric_limits<float>::max();
        }
        light_dir.normalize();

        lookat(light_dir, center, up);// 从光源得到shadowbuffer
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(-1.f / (eye - center).norm());
        
        TGAImage shadow(width, height, TGAImage::RGB);

        ShadowShader shadowShader;
        Vec4f screen_coords[3];
        for (int i = 0; i < model->nfaces(); i++) {
            
            for (int j = 0; j < 3; j++) {
                screen_coords[j] = shadowShader.vertex(i, j);
            }
            triangle(screen_coords, shadowShader, shadow, shadowbuffer);
        }
        shadow.flip_vertically();
        shadow.write_tga_file("shadow.tga");
        
        Matrix M = Viewport * Projection * ModelView; //灯光空间的MVP

        lookat(eye, center, up);
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(-1.f / (eye - center).norm());


        TGAImage image(width, height, TGAImage::RGB);

        PhoneShader shader;
        shader.uniform_M = Projection * ModelView;
        shader.uniform_MIT = shader.uniform_M.invert_transpose();
        shader.uniform_Mshadow = M * (Viewport * Projection * ModelView).invert();
        Vec4f screen_coords2[3];
        for (int i = 0; i < model->nfaces(); i++) {
            

            for (int j = 0; j < 3; j++) {
                screen_coords2[j] = shader.vertex(i, j);
                shader.uv_coords[j] = model->uv(i, j);
            }
            triangle(screen_coords2, shader, image, zbuffer);
        }
        
        delete model;
        delete[] zbuffer;
        delete[] shadowbuffer;

        image.flip_vertically(); // to place the origin in the bottom left corner of the image
        image.write_tga_file("output.tga");
        
    }
};
int main() {
    IShadowShader t;
    return 0;
}
