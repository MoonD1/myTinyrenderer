#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <cmath>
#include <vector>
using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const int width = 800;
const int height = 800;
const int depth = 255;
Vec3f eye = Vec3f(1, 1, 3);
Vec3f center(0, 0, 0);
const Vec3f up(0, 1, 0);
Vec3f light_dir = Vec3f(0, 1, 1).normalize();
float* zbuffer;


//////////////////////////////////////////////////////////////////////////////////
//  Model
class CameraMove {
	Matrix lookat(Vec3f eye, Vec3f center, Vec3f up) {
		Matrix ModelView = Matrix::identity(4);
		Vec3f z = (eye - center).normalize();
		Vec3f x = (up ^ z).normalize();
		Vec3f y = (z ^ x).normalize();
		for (int i = 0; i < 3; i++) {
			ModelView[0][i] = x[i];
			ModelView[1][i] = y[i];
			ModelView[2][i] = z[i];
			ModelView[i][3] = -center[i];
		}
		return ModelView;
	}
	Matrix viewport(int x, int y, int w, int h) {
		Matrix m = Matrix::identity(4);
		m[0][3] = x + w / 2.f;
		m[1][3] = y + h / 2.f;
		m[2][3] = depth / 2.f;

		m[0][0] = w / 2.f;
		m[1][1] = h / 2.f;
		m[2][2] = depth / 2.f;
		return m;
	}
	Vec3f triangle(Vec3i* tri, Vec2i node) {
		Vec3i t0 = tri[0];
		Vec3i t1 = tri[1];
		Vec3i t2 = tri[2];
		Vec3i P(node.x, node.y, 1);
		Vec3i PA = t0 - P;
		Vec3i AB = t1 - t0;
		Vec3i AC = t2 - t0;
		Vec3i x = { AB.x, AC.x ,PA.x };
		Vec3i y = { AB.y, AC.y ,PA.y };
		Vec3i u = x ^ y;
		if (u.z == 0) {
			return Vec3f(-1, 1, 1);
		}
		return Vec3f(1. - (u.x + u.y) / (float)u.z, (float)u.x / (float)u.z, (float)u.y / (float)u.z);
	}

	vector<Vec2i> getBox(Vec3i* tri) {
		int left = min(tri[0].x, min(tri[1].x, tri[2].x));
		int right = max(tri[0].x, max(tri[1].x, tri[2].x));
		int bottom = min(tri[0].y, min(tri[1].y, tri[2].y));
		int top = max(tri[0].y, max(tri[1].y, tri[2].y));
		Vec2i left_top = { left, top };
		Vec2i right_bottom = { right, bottom };
		return { left_top, right_bottom };
	}

	void Triangle(Vec3i* tri, vector<Vec2i> box, TGAImage& image, float* ins, Model* model, Vec2i* uvs) {
		Vec2i left_top = box[0];
		Vec2i right_bottom = box[1];
		for (int i = left_top.x; i < right_bottom.x; i++) {
			for (int j = right_bottom.y; j < left_top.y; j++) {
				Vec2i node = { i, j };
				Vec3f v = triangle(tri, node);
				if (v.x >= 0. && v.x <= 1. && v.y >= 0. && v.y <= 1. && v.z >= 0. && v.z <= 1.) {
					Vec2i pixeluv = uvs[0] * v.x + uvs[1] * v.y + uvs[2] * v.z;
					TGAColor pixcolor = model->diffuse(pixeluv);
					float z = (float)tri[0].z * v.x + (float)tri[1].z * v.y + (float)tri[2].z * v.z;
					float ityp = ins[0] * v.x + ins[1] * v.y + ins[2] * v.z;
					if (zbuffer[node.x + node.y * 800] < z) {
						zbuffer[node.x + node.y * 800] = z;
						//image.set(i, j, color);
						//image.set(i, j, TGAColor(pixcolor.r * ins[0], pixcolor.g * ins[1], pixcolor.b * ins[2], 255));
						image.set(i, j,  TGAColor(pixcolor.bgra[2], pixcolor.bgra[1], pixcolor.bgra[0], 255) * ityp);
					}

				}
			}
		}
	}
public:
	CameraMove() {
		TGAImage image(800, 800, TGAImage::RGB);

		Model* model = new Model("./obj/african_head.obj");
		Matrix Projection = Matrix::identity(4);
		Projection[3][2] = -1.f / (eye - center).norm();
		Matrix view = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

		zbuffer = new float[800 * 800];
		for (int i = 0; i < 800 * 800; i++) {
			zbuffer[i] = -numeric_limits<float>::max();
		}
		Matrix ModelView = lookat(eye, center, up);

		for (int i = 0; i < model->nfaces(); i++) {
			vector<int> face = model->face(i);
			Vec3i screen_coords[3];
			Vec3f world_coords[3];
			float intensity[3];
			for (int j = 0; j < 3; j++) {
				Vec3f v = model->vert(face[j]);
				world_coords[j] = v;
				intensity[j] = model->norm(i, j) * light_dir;
				Matrix vm = Matrix(4, 1);
				vm[0][0] = v.x;
				vm[1][0] = v.y;
				vm[2][0] = v.z;
				vm[3][0] = 1.;
				//M
				vm = ModelView * vm;
				//P
				vm = Projection * vm;
				//V
				vm = view * vm;
				//screen_coords[j] = Vec3i((v.x + 1.) * width / 2, (v.y + 1.) * height / 2, v.z);
				screen_coords[j] = Vec3i(vm[0][0] / vm[3][0], vm[1][0] / vm[3][0], vm[2][0] / vm[3][0]);

			}

			Vec2i uv_coords[3] = { Vec2i(model->uv(i, 0)), Vec2i(model->uv(i, 1)), Vec2i(model->uv(i, 2)) };

			vector<Vec2i> box = getBox(screen_coords);

			//float intensity;
			//Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
			//n.normalize();
			//intensity = n * light_dir;

			Triangle(screen_coords, box, image, intensity, model, uv_coords);

		}
		image.flip_vertically();
		image.write_tga_file("output.tga");

		delete model;
	}
};
//////////////////////////////////////////////////////////////////////////////
//  Projection and View
class Camera{
	Matrix viewport(int x, int y, int w, int h) {
		Matrix m = Matrix::identity(4);
		m[0][3] = x + w / 2.f;
		m[1][3] = y + h / 2.f;
		m[2][3] = depth / 2.f;

		m[0][0] = w / 2.f;
		m[1][1] = h / 2.f;
		m[2][2] = depth / 2.f;
		return m;
	}
	Vec3f triangle(Vec3i* tri, Vec2i node) {
		Vec3i t0 = tri[0];
		Vec3i t1 = tri[1];
		Vec3i t2 = tri[2];
		Vec3i P(node.x, node.y, 1);
		Vec3i PA = t0 - P;
		Vec3i AB = t1 - t0;
		Vec3i AC = t2 - t0;
		Vec3i x = { AB.x, AC.x ,PA.x };
		Vec3i y = { AB.y, AC.y ,PA.y };
		Vec3i u = x ^ y;
		if (u.z == 0) {
			return Vec3f(-1, 1, 1);
		}
		return Vec3f(1. - (u.x + u.y) / (float)u.z, (float)u.x / (float)u.z, (float)u.y / (float)u.z);
	}

	vector<Vec2i> getBox(Vec3i* tri) {
		int left = min(tri[0].x, min(tri[1].x, tri[2].x));
		int right = max(tri[0].x, max(tri[1].x, tri[2].x));
		int bottom = min(tri[0].y, min(tri[1].y, tri[2].y));
		int top = max(tri[0].y, max(tri[1].y, tri[2].y));
		Vec2i left_top = { left, top };
		Vec2i right_bottom = { right, bottom };
		return { left_top, right_bottom };
	}

	void Triangle(Vec3i* tri, vector<Vec2i> box, TGAImage& image, float ins, Model* model, Vec2i* uvs) {
		Vec2i left_top = box[0];
		Vec2i right_bottom = box[1];
		for (int i = left_top.x; i < right_bottom.x; i++) {
			for (int j = right_bottom.y; j < left_top.y; j++) {
				Vec2i node = { i, j };
				Vec3f v = triangle(tri, node);
				if (v.x >= 0. && v.x <= 1. && v.y >= 0. && v.y <= 1. && v.z >= 0. && v.z <= 1.) {
					float z = (float)tri[0].z * v.x + (float)tri[1].z * v.y + (float)tri[2].z * v.z;
					Vec2i pixeluv = uvs[0] * v.x + uvs[1] * v.y + uvs[2] * v.z;
					if (zbuffer[node.x + node.y * 800] < z) {
						zbuffer[node.x + node.y * 800] = z;
						TGAColor pixcolor = model->diffuse(pixeluv);
						//image.set(i, j, color);
						image.set(i, j, TGAColor(pixcolor.bgra[2] * ins, pixcolor.bgra[1] * ins, pixcolor.bgra[0] * ins, 255));
					}

				}
			}
		}
	}
public:
	Camera() {
		Vec3f light_dir = { 0, 0, -1 };
		TGAImage image(800, 800, TGAImage::RGB);
		Model* model = new Model("./obj/african_head.obj");
		zbuffer = new float[800 * 800];
		for (int i = 0; i < 800 * 800; i++) {
			zbuffer[i] = -numeric_limits<float>::max();
		}
		for (int i = 0; i < model->nfaces(); i++) {
			vector<int> face = model->face(i);
			Vec3i screen_coords[3];
			Vec3f world_coords[3];
			
			for (int j = 0; j < 3; j++) {
				//M
				Vec3f v = model->vert(face[j]);
				world_coords[j] = v;

				Matrix vm = Matrix(4, 1);	
				vm[0][0] = v.x;					
				vm[1][0] = v.y;
				vm[2][0] = v.z;
				vm[3][0] = 1.;
				//P
				Matrix Projection = Matrix::identity(4);
				Projection[3][2] = -1.f / eye.z;
				vm = Projection * vm;

				//V
				Matrix view = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
				vm = view * vm;
				//screen_coords[j] = Vec3i((v.x + 1.) * width / 2, (v.y + 1.) * height / 2, v.z);
				screen_coords[j] = Vec3i(vm[0][0]/vm[3][0], vm[1][0]/vm[3][0], vm[2][0]/vm[3][0]);
				
			}
			
			Vec2i uv_coords[3] = { Vec2i(model->uv(i, 0)), Vec2i(model->uv(i, 1)), Vec2i(model->uv(i, 2)) };

			vector<Vec2i> box = getBox(screen_coords);

			float intensity;
			Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
			n.normalize();
			intensity = n * light_dir;
			if (intensity > 0) {
				Triangle(screen_coords, box, image, intensity, model, uv_coords);
			}

		}
		image.flip_vertically();
		image.write_tga_file("output.tga");
		delete model;
		delete zbuffer;
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
//  texture
class Texture {
	Vec3f triangle(Vec3i* tri, Vec2i node) {
		Vec3i t0 = tri[0];
		Vec3i t1 = tri[1];
		Vec3i t2 = tri[2];
		Vec3i P(node.x, node.y, 1);
		Vec3i PA = t0 - P;
		Vec3i AB = t1 - t0;
		Vec3i AC = t2 - t0;
		Vec3i x = { AB.x, AC.x ,PA.x };
		Vec3i y = { AB.y, AC.y ,PA.y };
		Vec3i u = x ^ y;
		if (u.z == 0) {
			return Vec3f(-1, 1, 1);
		}
		return Vec3f(1. - (u.x + u.y) / (float)u.z, (float)u.x / (float)u.z, (float)u.y / (float)u.z);
	}

	vector<Vec2i> getBox(Vec3i* tri) {
		int left = min(tri[0].x, min(tri[1].x, tri[2].x));
		int right = max(tri[0].x, max(tri[1].x, tri[2].x));
		int bottom = min(tri[0].y, min(tri[1].y, tri[2].y));
		int top = max(tri[0].y, max(tri[1].y, tri[2].y));
		Vec2i left_top = { left, top };
		Vec2i right_bottom = { right, bottom };
		return { left_top, right_bottom };
	}

	void Triangle(Vec3i* tri, vector<Vec2i> box, TGAImage& image, float ins, Model* model, Vec2i* uvs) {
		Vec2i left_top = box[0];
		Vec2i right_bottom = box[1];
		for (int i = left_top.x; i < right_bottom.x; i++) {
			for (int j = right_bottom.y; j < left_top.y; j++) {
				Vec2i node = { i, j };
				Vec3f v = triangle(tri, node);
				if (v.x >= 0. && v.x <= 1. && v.y >= 0. && v.y <= 1. && v.z >= 0. && v.z <= 1.) {
					float z = (float)tri[0].z * v.x + (float)tri[1].z * v.y + (float)tri[2].z * v.z;
					Vec2i pixeluv = uvs[0] * v.x + uvs[1] * v.y + uvs[2] * v.z;
					if (zbuffer[node.x + node.y * 800] < z) {
						zbuffer[node.x + node.y * 800] = z;
						TGAColor pixcolor = model->diffuse(pixeluv);
						//image.set(i, j, color);
						image.set(i, j, TGAColor(pixcolor.bgra[2] * ins, pixcolor.bgra[1] * ins, pixcolor.bgra[0], 255));
					}

				}
			}
		}
	}
public:
	Texture() {
		Vec3f light_dir = { 0, 0, -1 };
		TGAImage image(800, 800, TGAImage::RGB);
		Model* model = new Model("./obj/african_head.obj");
		zbuffer = new float[800 * 800];
		for (int i = 0; i < 800 * 800; i++) {
			zbuffer[i] = -numeric_limits<float>::max();
		}
		for (int i = 0; i < model->nfaces(); i++) {
			vector<int> face = model->face(i);
			Vec3f v0 = model->vert(face[0]);
			Vec3f v1 = model->vert(face[1]);
			Vec3f v2 = model->vert(face[2]);
			Vec3i tri[3] = { Vec3i((v0.x + 1.) * 800 / 2., (v0.y + 1.) * 800 / 2., (v0.z + 1.) * 800 / 2),
			Vec3i((v1.x + 1.) * 800 / 2., (v1.y + 1.) * 800 / 2., (v1.z + 1.) * 800 / 2),
			Vec3i((v2.x + 1.) * 800 / 2., (v2.y + 1.) * 800 / 2., (v2.z + 1.) * 800 / 2) };

			Vec2i uv_coords[3] = { Vec2i(model->uv(i, 0)), Vec2i(model->uv(i, 1)), Vec2i(model->uv(i, 2)) };

			vector<Vec2i> box = getBox(tri);

			float intensity;
			Vec3f n = (v2 - v0) ^ (v1 - v0);
			n.normalize();
			intensity = n * light_dir;
			if (intensity > 0) {
				Triangle(tri, box, image, intensity, model, uv_coords);
			}

		}
		image.flip_vertically();
		image.write_tga_file("output.tga");
		delete model;
	}
};
/////////////////////////////////////////////////////////////////////////////////////////
//  draw triangle with z-buffer
class ZBuffer {
	Vec3f triangle(Vec3i* tri, Vec2i node) {
		Vec3i t0 = tri[0];
		Vec3i t1 = tri[1];
		Vec3i t2 = tri[2];
		Vec3i P(node.x, node.y, 1);
		Vec3i PA = t0 - P;
		Vec3i AB = t1 - t0;
		Vec3i AC = t2 - t0;
		Vec3i x = { AB.x, AC.x ,PA.x };
		Vec3i y = { AB.y, AC.y ,PA.y };
		Vec3i u = x ^ y;
		if (u.z == 0) {
			return Vec3f(-1, 1, 1);
		}
		return Vec3f(1. - (u.x + u.y) / (float)u.z, (float)u.x / (float)u.z, (float)u.y / (float)u.z);
	}

	vector<Vec2i> getBox(Vec3i* tri) {
		int left = min(tri[0].x, min(tri[1].x, tri[2].x));
		int right = max(tri[0].x, max(tri[1].x, tri[2].x));
		int bottom = min(tri[0].y, min(tri[1].y, tri[2].y));
		int top = max(tri[0].y, max(tri[1].y, tri[2].y));
		Vec2i left_top = { left, top };
		Vec2i right_bottom = { right, bottom };
		return { left_top, right_bottom };
	}

	void Triangle(Vec3i* tri, vector<Vec2i> box, TGAImage& image, TGAColor color) {
		Vec2i left_top = box[0];
		Vec2i right_bottom = box[1];
		for (int i = left_top.x; i < right_bottom.x; i++) {
			for (int j = right_bottom.y; j < left_top.y; j++) {
				Vec2i node = { i, j };
				Vec3f v = triangle(tri, node);
				if (v.x >= 0. && v.x <= 1. && v.y >= 0. && v.y <= 1. && v.z >= 0. && v.z <= 1.) {
					float z = (float)tri[0].z * v.x + (float)tri[1].z * v.y + (float)tri[2].z * v.z;
					if (zbuffer[node.x + node.y * 800] < z) {
						zbuffer[node.x + node.y * 800] = z;
						image.set(i, j, color);
					}

				}
			}
		}
	}
public:
	ZBuffer() {
		Vec3f light_dir = { 0, 0, -1 };
		TGAImage image(800, 800, TGAImage::RGB);
		Model* model = new Model("./obj/african_head.obj");
		zbuffer = new float[800 * 800];
		for (int i = 0; i < 800 * 800; i++) {
			zbuffer[i] = -numeric_limits<float>::max();
		}
		for (int i = 0; i < model->nfaces(); i++) {
			vector<int> face = model->face(i);
			Vec3f v0 = model->vert(face[0]);
			Vec3f v1 = model->vert(face[1]);
			Vec3f v2 = model->vert(face[2]);
			Vec3i tri[3] = { Vec3i((v0.x + 1.) * 800 / 2., (v0.y + 1.) * 800 / 2., (v0.z + 1.) * 800 / 2),
			Vec3i((v1.x + 1.) * 800 / 2., (v1.y + 1.) * 800 / 2., (v1.z + 1.) * 800 / 2),
			Vec3i((v2.x + 1.) * 800 / 2., (v2.y + 1.) * 800 / 2., (v2.z + 1.) * 800 / 2) };
			vector<Vec2i> box = getBox(tri);

			float intensity;
			Vec3f n = (v2 - v0) ^ (v1 - v0);
			n.normalize();
			intensity = n * light_dir;
			if (intensity > 0) {
				Triangle(tri, box, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
			}

		}
		image.flip_vertically();
		image.write_tga_file("output.tga");
		delete model;
	}
};
////////////////////////////////////////////////////////////////////////////////////
//  draw triangle with light
class TriangleWithLight{
	Vec3f triangle(Vec2i* tri, Vec2i node) {
		Vec3i t0(tri[0].x, tri[0].y, 1);
		Vec3i t1(tri[1].x, tri[1].y, 1);
		Vec3i t2(tri[2].x, tri[2].y, 1);
		Vec3i P(node.x, node.y, 1);
		Vec3i PA = t0 - P;
		Vec3i AB = t1 - t0;
		Vec3i AC = t2 - t0;
		Vec3i x = { AB.x, AC.x ,PA.x };
		Vec3i y = { AB.y, AC.y ,PA.y };
		Vec3i u = x ^ y;
		if (u.z == 0) {
			return Vec3f(-1, 1, 1);
		}
		return Vec3f(1. - (u.x + u.y) / (float)u.z, (float)u.x / (float)u.z, (float)u.y / (float)u.z);
	}

	vector<Vec2i> getBox(Vec2i* tri) {
		int left = min(tri[0].x, min(tri[1].x, tri[2].x));
		int right = max(tri[0].x, max(tri[1].x, tri[2].x));
		int bottom = min(tri[0].y, min(tri[1].y, tri[2].y));
		int top = max(tri[0].y, max(tri[1].y, tri[2].y));
		Vec2i left_top = { left, top };
		Vec2i right_bottom = { right, bottom };
		return { left_top, right_bottom };
	}

	void Triangle(Vec2i* tri, vector<Vec2i> box, TGAImage& image, TGAColor color) {
		Vec2i left_top = box[0];
		Vec2i right_bottom = box[1];
		for (int i = left_top.x; i < right_bottom.x; i++) {
			for (int j = right_bottom.y; j < left_top.y; j++) {
				Vec2i node = { i, j };
				Vec3f v = triangle(tri, node);
				if (v.x >= 0. && v.x <= 1. && v.y >= 0. && v.y <= 1. && v.z >= 0. && v.z <= 1.) {
					image.set(i, j, color);
				}
			}
		}
	}
public:
	TriangleWithLight() {
		Vec3f light_dir = { 0, 0, -1 };
		TGAImage image(800, 800, TGAImage::RGB);
		Model* model = new Model("./obj/african_head.obj");
		for (int i = 0; i < model->nfaces(); i++) {
			vector<int> face = model->face(i);
			Vec3f v0 = model->vert(face[0]);
			Vec3f v1 = model->vert(face[1]);
			Vec3f v2 = model->vert(face[2]);
			Vec2i tri[3] = { Vec2i((v0.x + 1.) * 800 / 2., (v0.y + 1.) * 800 / 2.),
			Vec2i((v1.x + 1.) * 800 / 2., (v1.y + 1.) * 800 / 2.),
			Vec2i((v2.x + 1.) * 800 / 2., (v2.y + 1.) * 800 / 2.) };
			vector<Vec2i> box = getBox(tri);

			float intensity;
			Vec3f n = (v2 - v0) ^ (v1 - v0);
			n.normalize();
			intensity = n * light_dir;
			if (intensity > 0) {
				Triangle(tri, box, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
			}
		
		}
		image.flip_vertically();
		image.write_tga_file("output.tga");
		delete model;
	}
};
/////////////////////////////////////////////////////////////////////////////////
//  draw triangle
class DrawTriangle{
	Vec3f triangle(Vec2i* tri, Vec2i node) {
		Vec3i t0(tri[0].x, tri[0].y, 1);
		Vec3i t1(tri[1].x, tri[1].y, 1);
		Vec3i t2(tri[2].x, tri[2].y, 1);
		Vec3i P(node.x, node.y, 1);
		Vec3i PA = t0 - P;
		Vec3i AB = t1 - t0;
		Vec3i AC = t2 - t0;
		Vec3i x = { AB.x, AC.x ,PA.x };
		Vec3i y = { AB.y, AC.y ,PA.y };
		Vec3i u = x ^ y;
		if (u.z == 0) {
			return Vec3f(-1, 1, 1);
		}
		return Vec3f(1. - (u.x + u.y) / (float)u.z, (float)u.x / (float)u.z, (float)u.y / (float)u.z);
	}

	vector<Vec2i> getBox(Vec2i* tri) {
		int left = min(tri[0].x, min(tri[1].x, tri[2].x));
		int right = max(tri[0].x, max(tri[1].x, tri[2].x));
		int bottom = min(tri[0].y, min(tri[1].y, tri[2].y));
		int top = max(tri[0].y, max(tri[1].y, tri[2].y));
		Vec2i left_top = { left, top };
		Vec2i right_bottom = { right, bottom };
		return { left_top, right_bottom };
	}

	void Triangle(Vec2i* tri, vector<Vec2i> box, TGAImage& image, TGAColor color) {
		Vec2i left_top = box[0];
		Vec2i right_bottom = box[1];
		for (int i = left_top.x; i < right_bottom.x; i++) {
			for (int j = right_bottom.y; j < left_top.y; j++) {
				Vec2i node = { i, j };
				Vec3f v = triangle(tri, node);
				if (v.x > 0. && v.x < 1. && v.y > 0. && v.y < 1. && v.z > 0. && v.z < 1.) {
					image.set(i, j, color);
				}
			}
		}
	}


	//缺点：无法获得插值（用于着色）
	bool inTriangle(Vec2i* tri, Vec2i node) {
		Vec3i t0(tri[0].x, tri[0].y, 1);
		Vec3i t1(tri[1].x, tri[1].y, 1);
		Vec3i t2(tri[2].x, tri[2].y, 1);
		Vec3i P(node.x, node.y, 1);
		Vec3i AB = t0 - t1;
		Vec3i BC = t1 - t2;
		Vec3i CA = t2 - t0;
		Vec3i PA = t1 - P;
		Vec3i PB = t2 - P;
		Vec3i PC = t0 - P;
		int direction = (AB ^ PA).z;
		if (direction > 0) {
			return (BC ^ PB).z > 0 && (CA ^ PC).z > 0;
		}
		return (BC ^ PB).z < 0 && (CA ^ PC).z < 0;
	}
public:
	DrawTriangle() {
		TGAImage image(800, 800, TGAImage::RGB);
	
	//	Vec2i tri[3] = { Vec2i(100, 100), Vec2i(600, 400), Vec2i(400, 500) };
	//	for (int i = 0; i < 800; i++) {
	//		for (int j = 0; j < 800; j++) {
	//			Vec2i node = { i, j };
				//if (inTriangle(tri, node)) {
				//	image.set(i, j, red);
				//}
	//			Vec3f v = triangle(tri, node);
	//			if (v.x > 0. && v.x < 1. && v.y > 0. && v.y < 1. && v.z > 0. && v.z < 1.) {
	//				image.set(i, j, white);
	//			}
	//		}
	//	}
	
		Model* model = new Model("./obj/african_head.obj");
		for (int i = 0; i < model->nfaces(); i++) {
			vector<int> face = model->face(i);
			Vec3f v0 = model->vert(face[0]);
			Vec3f v1 = model->vert(face[1]);
			Vec3f v2 = model->vert(face[2]);
			Vec2i tri[3] = { Vec2i((v0.x + 1.) * 800 / 2., (v0.y + 1.) * 800 / 2.),
			Vec2i((v1.x + 1.) * 800 / 2., (v1.y + 1.) * 800 / 2.),
			Vec2i((v2.x + 1.) * 800 / 2., (v2.y + 1.) * 800 / 2.)};
			vector<Vec2i> box = getBox(tri);
			Triangle(tri, box, image, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));
		}
		image.flip_vertically();
		image.write_tga_file("output.tga");
		delete model;
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////
//  draw line
class Line {
	// Bresenham’s Line Drawing Algorithm
	void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
		bool flag = false;

		if (abs(y1 - y0) > abs(x1 - x0)) {
			swap(x0, y0);
			swap(x1, y1);
			flag = true;
		}
		if (x1 < x0) {
			swap(x0, x1);
			swap(y0, y1);
		}
		int dx = x1 - x0;
		int dy = abs(y1 - y0);
		int err = 0;
		int y = y0;
		for (int x = x0; x <= x1; x++) {
			flag ? image.set(y, x, color) : image.set(x, y, color);
			err += dy;
			if ((err << 1) >= dx) {
				y1 > y0 ? y++ : y--;
				err -= dx;
			}
		}

	}

	// 缺点：浮点运算耗能多
	void DDAline(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
		float step = abs(x1 - x0) > abs(y1 - y0) ? abs(x1 - x0) : abs(y1 - y0);
		float x = x0;
		float y = y0;
		float dx = (x1 - x0) / step;
		float dy = (y1 - y0) / step;
		for (int i = 0; i <= step; i++) {
			image.set(x, y, color);
			x = x + dx;
			y = y + dy;
		}
	}

	// 缺点:1.t的影响  2.起点终点互换结果有偏差  3.线段不一定连续
	void line1(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
		for (float t = 0; t <= 1; t += 0.01) {
			float x = (x1 - x0) * t + x0;
			float y = (y1 - y0) * t + y0;
			image.set(x, y, color);
		}
	}
public:
	Line() {
		//TGAImage image(100, 100, TGAImage::RGB);
		//line(13, 20, 80, 40, image, white);
		//line(20, 13, 40, 80, image, red);
		//line(80, 40, 13, 20, image, red);
		TGAImage image(800, 800, TGAImage::RGB);
		Model* model = new Model("./obj/african_head.obj");
		for (int i = 0; i < model->nfaces(); i++) {
			vector<int> face = model->face(i);
			for (int j = 0; j < 3; j++) {
				Vec3f v0 = model->vert(face[j]);
				Vec3f v1 = model->vert(face[(j + 1) % 3]);
				int x0 = (v0.x + 1.) * 800 / 2.;
				int y0 = (v0.y + 1.) * 800 / 2.;
				int x1 = (v1.x + 1.) * 800 / 2.;
				int y1 = (v1.y + 1.) * 800 / 2.;
				line(x0, y0, x1, y1, image, white);
			}
		}
		image.flip_vertically();
		image.write_tga_file("output.tga");
		delete model;
	}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////
//  draw node
class Node{
public:
	Node() {
		TGAImage image(100, 100, TGAImage::RGB);
		image.set(52, 41, red);
		image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
		image.write_tga_file("output.tga");
	}
};

int main() {
	CameraMove t;
	return 0;
}