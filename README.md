# GradientBlur

#### 介绍
本项目使用官方的XComponent3D示例作为目标，开发基于积分图的渐变模糊效果。

基于XComponent组件调用Native API来创建EGL/GLES环境，从而使用标准OpenGL ES以及计算着色器Compute Shader实现图像渐变模糊效果。

#### 使用说明
本Demo目前需要使用hdc工具，将图像文件传输到设备，步骤如下：
1. 使用DevEco Studio安装APP(需要FULL SDK 11)
2. 在当前目录下打开终端，输入如下指令
```shell
hdc file send .\image.png data/app/el1/bundle/public/image.png

hdc shell #进入设备终端后
cd data/app/el1/bundle/public
cp data/app/el1/bundle/public/image.png com.samples.XComponent3D/ #当图片复制到沙盒目录下
```

在主页面，默认展示的是3D图形绘制效果，通过点击tab可以切换到第二个图像渐变模糊效果。

#### 工程目录
```
entry/src/main/ets/
|---entryability
|	|	|---EntryAbility.ts
|---pages
|	|---Index.ets				//首页
entry/src/main/cpp/
|---include
	|---util
		|---log.h 				// 日志工具类
		|---napi_manager.h		// 负责管理注册进来的多个XComponent控件
		|---napi_util.h			// 工具类
		|---native_common.h		// napi函数注册入口
		|---types.h				// 常量类
	|---app_napi.h				// 实现XComponent的生命周期函数，注册napi绘制接口
	|---opengl_draw.h			// 3D类绘制类，用于绘制立方体或者长方体
	|---shader.h				// shader编译类
|---shape
	|---base_shape.h			// 基类
	|---cube.cpp
	|---cube.h					
	|---rectangle.cpp
	|---rectangle.h				// 渐变模糊效果实现类
|---types
	|---libnativerender			
		|---nativerender.d.ts	// 对外接口，用于界面进行调用
|---app_napi.app				
|---module.cpp					
|---napi_manager.cpp			
|---napi_util.cpp				
|---opengl_draw.cpp				
```

#### 相关权限
不涉及


#### 依赖
不涉及


#### 约束与限制
本示例仅支持标准系统上运行，支持设备：RK3568; DevEco Studio 4.1; FULL SDK 11
