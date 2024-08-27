#include <vector>
#include <iostream>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model *model     = NULL;
const int width  = 800;
const int height = 800;



Vec3f light_dir(1,1,1);
Vec3f       eye(1,1,3);
Vec3f    center(0,0,0);
Vec3f        up(0,1,0);


//  Blinn-Phong Shader
//  ���⣺��������ֻ�����ڵ�ǰ��ģ�Ͷ���������ռ䣩�������γ��ֱ任ʱ��������Ҳ��仯
//  ���������ʹ��TBN�ռ䷨������ ���� ����ռ䷨������
class IPhoneShader {
    struct PhoneShader : public IShader {
        Vec3f varying_intensity; // written by vertex shader, read by fragment shader
        Vec2f uv_coords[3];
        mat<4, 4, float> uniform_M;
        mat<4, 4, float> uniform_MIT;
        virtual Vec4f vertex(int iface, int nthvert) {
            Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
            gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;     // transform it to screen coordinates
            varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir); // get diffuse lighting intensity
            return gl_Vertex;
        }

        virtual bool fragment(Vec3f bar, TGAColor& color) {

            Vec2f pixeluv = uv_coords[0] * bar.x + uv_coords[1] * bar.y + uv_coords[2] * bar.z;
            TGAColor pixcolor = model->diffuse(pixeluv);

            Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(pixeluv))).normalize();  //����Model�仯��Ķ���ķ���
            Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize(); //���߾���Model�仯
            Vec3f r = (n * (n * l * 2.f) - l).normalize();   // reflected light
            float spec = pow(std::max(r.z, 0.0f), model->specular(pixeluv));
            float intensity = std::max(0.f, n * l);
            //float intensity = varying_intensity * bar;   // interpolate intensity for the current pixel

            for(int i=0;i<3;i++){
                color[i] = 5 + pixcolor[i] * (intensity + 0.6 * spec);// 5Ϊ�������գ�intensity��Ӧ�����䣬spec��Ӧ�߹�
            }
            //color = TGAColor(255, 255, 255)*intensity; // well duh
            return false;                              // no, we do not discard this pixel
        }
    };
public:
    IPhoneShader() {

        model = new Model("obj/african_head.obj");

        lookat(eye, center, up);
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(-1.f / (eye - center).norm());
        light_dir.normalize();

        TGAImage image(width, height, TGAImage::RGB);
        TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

        PhoneShader shader;
        shader.uniform_M = Projection * ModelView;
        shader.uniform_MIT = shader.uniform_M.invert_transpose();
        for (int i = 0; i < model->nfaces(); i++) {
            Vec4f screen_coords[3];

            for (int j = 0; j < 3; j++) {
                screen_coords[j] = shader.vertex(i, j);
                shader.uv_coords[j] = model->uv(i, j);
            }
            triangle(screen_coords, shader, image, zbuffer);
        }

        image.flip_vertically(); // to place the origin in the bottom left corner of the image
        zbuffer.flip_vertically();
        image.write_tga_file("output.tga");
        zbuffer.write_tga_file("zbuffer.tga");

        delete model;
    }
};


///////////////////////////////////////////////////////////////////////
//  using normal mapping texture instead of interpolating normal vectors
class NewShader {
    struct Shader : public IShader {
        Vec3f varying_intensity; // written by vertex shader, read by fragment shader
        Vec2f uv_coords[3];
        mat<4, 4, float> uniform_M;
        mat<4, 4, float> uniform_MIT;
        virtual Vec4f vertex(int iface, int nthvert) {
            Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
            gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;     // transform it to screen coordinates
            varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir); // get diffuse lighting intensity
            return gl_Vertex;
        }

        virtual bool fragment(Vec3f bar, TGAColor& color) {
      
            Vec2f pixeluv = uv_coords[0] * bar.x + uv_coords[1] * bar.y + uv_coords[2] * bar.z;
            TGAColor pixcolor = model->diffuse(pixeluv);

            Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(pixeluv))).normalize();  //����Model�仯��Ķ���ķ���
            Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize(); //���߾���Model�仯
            float intensity = std::max(0.f, n * l);
            //float intensity = varying_intensity * bar;   // interpolate intensity for the current pixel

            color = pixcolor * intensity;
            //color = TGAColor(255, 255, 255)*intensity; // well duh
            return false;                              // no, we do not discard this pixel
        }
    };
public:
    NewShader() {

        model = new Model("obj/african_head.obj");

        lookat(eye, center, up);
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(-1.f / (eye - center).norm());
        light_dir.normalize();

        TGAImage image(width, height, TGAImage::RGB);
        TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

        Shader shader;
        shader.uniform_M = Projection * ModelView;
        shader.uniform_MIT = shader.uniform_M.invert_transpose();
        for (int i = 0; i < model->nfaces(); i++) {
            Vec4f screen_coords[3];

            for (int j = 0; j < 3; j++) {
                screen_coords[j] = shader.vertex(i, j);
                shader.uv_coords[j] = model->uv(i, j);
            }
            triangle(screen_coords, shader, image, zbuffer);
        }

        image.flip_vertically(); // to place the origin in the bottom left corner of the image
        zbuffer.flip_vertically();
        image.write_tga_file("output.tga");
        zbuffer.write_tga_file("zbuffer.tga");

        delete model;
    }
};

////////////////////////////////////////////////////////////////////////////////
//  GouraudShader
class Original{
    struct GouraudShader : public IShader {
        Vec3f varying_intensity; // written by vertex shader, read by fragment shader
        Vec2f uv_coords[3];
        virtual Vec4f vertex(int iface, int nthvert) {
            Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
            gl_Vertex = Viewport*Projection*ModelView*gl_Vertex;     // transform it to screen coordinates
            varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert)*light_dir); // get diffuse lighting intensity
            return gl_Vertex;
        }

        virtual bool fragment(Vec3f bar, TGAColor &color) {
            float intensity = varying_intensity*bar;   // interpolate intensity for the current pixel
            Vec2f pixeluv = uv_coords[0] * bar.x+ uv_coords[1] * bar.y +uv_coords[2] * bar.z ;
            TGAColor pixcolor = model->diffuse(pixeluv);
            color = pixcolor * intensity;
            //color = TGAColor(255, 255, 255)*intensity; // well duh
            return false;                              // no, we do not discard this pixel
        }
    };
public:
    Original() {

        model = new Model("obj/african_head.obj");

        lookat(eye, center, up);
        viewport(width/8, height/8, width*3/4, height*3/4);
        projection(-1.f/(eye-center).norm());
        light_dir.normalize();

        TGAImage image  (width, height, TGAImage::RGB);
        TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

        GouraudShader shader;
        for (int i=0; i<model->nfaces(); i++) {
            Vec4f screen_coords[3];
            
            for (int j=0; j<3; j++) {
                screen_coords[j] = shader.vertex(i, j);
                shader.uv_coords[j] = model->uv(i, j);
            }
            triangle(screen_coords, shader, image, zbuffer);
        }

        image.  flip_vertically(); // to place the origin in the bottom left corner of the image
        zbuffer.flip_vertically();
        image.  write_tga_file("output.tga");
        zbuffer.write_tga_file("zbuffer.tga");

        delete model;
    }
};

int main() {
    IPhoneShader t;
    return 0;
}
