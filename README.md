# myTinyrenderer
***
本项目基于c++语言，自myTinyrenderer-1.0 ~ myTinyrenderer-3.0循序渐进，用于个人学习渲染管线并模拟openGL实现软光栅渲染器（简化版本）。其中，每一个类的构造函数代表一个里程碑，均可单独调用。对于分支版本的划分仅依据对代码是否进行重构。output.tga为输出效果。
***
共有以下三个分支：
1. tinyrenderer-1.0：
 - 绘制单点
 - 绘制线段
 - 绘制一个三角形
 - 依照obj文件绘制三角形（三角形光栅化）
 - 添加z-buffer后的三角形光栅化
 - 添加纹理映射
 - 实现Projection transformation和View transformation（近大远小效果）
 - 实现Model transformation（设置相机位置）

2. tinyrenderer-refactor-2.0：
 - 对tinyrenderer-1.0代码进行重构，增加IShader接口
 - 实现GouraudShader
 - 使用法向量映射代替插值法计算法向量
 - 实现Blinn-Phong Shader

3. tinyrenderer-3.0(main分支):
 - 对tinyrenderer-refactor-2.0代码进行重构，将z-buffer数据类型从TGAImage修改为float*（简化shadowbuffer的实现）
 - 实现shadowbuffer，添加硬阴影

参考：
- tinyrenderer https://github.com/ssloy/tinyrenderer
- bilibili games101计算机图形学课程 https://www.bilibili.com/video/BV1X7411F744/?spm_id_from=333.934.top_right_bar_window_custom_collection.content.click&vd_source=705ad62239bfa25e66b117f24e7d3098

obj文件素材来源：
- tinyrenderer https://github.com/ssloy/tinyrenderer
