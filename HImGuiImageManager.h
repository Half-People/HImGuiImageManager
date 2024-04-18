//
//  HImGuiImageManager.h
//
//  Copyright (c) 2024 HalfPeople. All rights reserved.
//  MIT License
//
#pragma once
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "imgui.h"
#ifndef HIMAGE_MANAGER_GIF_IMAGE_ENABLED
#define HIMAGE_MANAGER_GIF_IMAGE_ENABLED 1    //If you do not want to use this function, please change it to '0'
#endif // !HIMAGE_MANAGER_GIF_IMAGE_ENABLED
#ifndef HIMAGE_MANAGER_URL_IMAGE_ENABLED
#define HIMAGE_MANAGER_URL_IMAGE_ENABLED 1    //If you need use this function, please change it to '1' (Need 'httplib.h'  Download ->  https://github.com/yhirose/cpp-httplib/tree/master)
#define HIMAGE_MANAGER_URL_OPENSSL_SUPPORT 1  //if you need OpenSSL support ,Please change it to '1'  (Need 'OpenSSL' (cpp-httplib currently supports only version 3.0 or later.)Download -> https://github.com/openssl/openssl/tree/master  Build->https://github.com/openssl/openssl/blob/master/INSTALL.md#building-openssl)
//https://youtu.be/PMHEoBkxYaQ?si=fYpChXxw_uEitGMT If you still can't understand it after watching the documentation, you can watch this video. His teachings are very detailed. I think it can help you.
#endif

struct HTexture
{
	int width, height, channel;
	unsigned char* texture_data;
};
struct HImage
{
	int width = 0, height = 0, channel = 0;
	HTextureID texture = 0;

	inline HTextureID GL_Texture() { return (void*)(long long)texture; }
	void SetInfo(int width_, int height_, int channel_, HTextureID texture_)
	{
		width = width_;
		height = height_;
		channel = channel_;
		texture = texture_;
	}
	void SetInfo(HTexture htexture)
	{
		width = htexture.width;
		height = htexture.height;
		channel = htexture.channel;
	}
};
typedef void* (*CreateTextureCallback)(uint8_t* data, int w, int h, char fmt);
typedef void (*DeleteTextureCallback)(void* tex);
typedef std::vector<unsigned char> HBitImage;
typedef void* HTextureID;

#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED || HIMAGE_MANAGER_URL_IMAGE_ENABLED
namespace Draw_Loading
{
	void Draw_Loading_Style_1(const ImVec2& pos, float radius);
}
#endif

struct HImageManagerIO
{
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED || HIMAGE_MANAGER_URL_IMAGE_ENABLED
	typedef void (*DrawLoadingCallback)(const ImVec2& half_pos, float half_size);
#endif // HIMAGE_MANAGER_GIF_IMAGE_ENABLED || HIMAGE_MANAGER_URL_IMAGE_ENABLED

	CreateTextureCallback CreateTexture = 0;
	DeleteTextureCallback DeleteTexture = 0;
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
	const char* url_image_cache_files_path = ".";
#endif // 0
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED || HIMAGE_MANAGER_URL_IMAGE_ENABLED
	DrawLoadingCallback DrawLoading = Draw_Loading::Draw_Loading_Style_1;
	int MaximumThreadExecutionTime_Seconds = 5;
	int ThreadPoolMaximumNuberOfThreads = -1;//std::thread::hardware_concurrency();
#endif // HIMAGE_MANAGER_GIF_IMAGE_ENABLED || HIMAGE_MANAGER_URL_IMAGE_ENABLED

	double HGetFunctionRuningSpeed(void(*function)());
};

namespace HImageManager
{
	HImageManagerIO& GetIO();
	namespace ImageLoader
	{
		HTextureID StaticImageLoader(const char* filename, CreateTextureCallback load = 0);
		void DeleteStaticImage(HTextureID texture, DeleteTextureCallback unload = 0);
		bool GetImage(HBitImage& bit_image, size_t& bit_image_size, HImage*& image_out, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
		bool GetImage(const char* filename, HImage*& image_out, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
		inline HTextureID GetImage(const char* filename, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0) { HImage* image; if (GetImage(filename, image, life_cycle, load, unload)) { return image->texture; } else { return 0; } }
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
		bool GetImage_gif(const char* filename, HImage*& image_out, float speed = 1000, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
		bool GetImage_gif(HBitImage& bit_image, size_t& bit_image_size, HImage*& image_out, float speed = 1000, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
#endif
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
		bool GetImage_url(const char* url, const char* path, const char* id, HImage*& image_out, bool CacheFile = false, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
		bool GetImage_url_gif(const char* url, const char* path, const char* id, HImage*& image_out, float speed = 1000, bool CacheFile = false, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
#endif
#endif
	}

	namespace DrawList
	{
		inline void AddImage(ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float life_cycle = 1.5, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0) { draw_list->AddImage(HImageManager::ImageLoader::GetImage(filename, life_cycle, load, unload), p_min, p_max, uv_min, uv_max, col); }
		inline void AddImageRounded(ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float rounding, float life_cycle = 1.5, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE, ImDrawFlags flags = 0, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0)
		{
			draw_list->AddImageRounded(HImageManager::ImageLoader::GetImage(filename, life_cycle, load, unload), p_min, p_max, uv_min, uv_max, col, rounding, flags);
		}
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
		void AddImage_gif(ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float speed = 1000, float life_cycle = 1.5, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE, HImageManagerIO::DrawLoadingCallback draw_loading = 0, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
		void AddImageRounded_gif(ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float rounding, float speed = 1000, float life_cycle = 1.5, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE, ImDrawFlags flags = 0, HImageManagerIO::DrawLoadingCallback draw_loading = 0, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
#endif
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
		void AddImage_url(ImDrawList* draw_list, const char* url, const char* path, const char* id, const ImVec2& p_min, const ImVec2& p_max, bool CacheFile = false, float life_cycle = 1.5, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE, HImageManagerIO::DrawLoadingCallback draw_loading = 0, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
		void AddImageRounded_url(ImDrawList* draw_list, const char* url, const char* path, const char* id, const ImVec2& p_min, const ImVec2& p_max, float rounding, bool CacheFile = false, float life_cycle = 1.5, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE, ImDrawFlags flags = 0, HImageManagerIO::DrawLoadingCallback draw_loading = 0, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
		void AddImage_url_gif(ImDrawList* draw_list, const char* url, const char* path, const char* id, const ImVec2& p_min, const ImVec2& p_max, float rounding, bool CacheFile = false, float speed = 1000, float life_cycle = 1.5, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE, ImDrawFlags flags = 0, HImageManagerIO::DrawLoadingCallback draw_loading = 0, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
#endif
#endif
	}

	bool ImageButton_plus(const char* label, const char* Bace_ButtonImageFileName, const char* Hovered_ButtonImageFileName, const char* Active_ButtonImageFileName, const ImVec2& size_arg, float life_cycle = 1.5, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), CreateTextureCallback load = 0, DeleteTextureCallback unload = 0, ImGuiButtonFlags flags = ImGuiButtonFlags_None);
	void Image(HBitImage& bit_image, size_t& bit_image_size, const ImVec2& size = ImVec2(150, 150), float rounding = 0, float life_cycle = 1.5, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0), CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
	void Image(const char* filename, const ImVec2& size = ImVec2(150, 150), float rounding = 0, float life_cycle = 1.5, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0), CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
	void Image_gif(const char* filename, const ImVec2& size = ImVec2(150, 150), float speed = 1000, float life_cycle = 1.5, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0), CreateTextureCallback load = 0, DeleteTextureCallback unload = 0, HImageManagerIO::DrawLoadingCallback draw_loading = 0);
	void Image_gif(HBitImage& bit_image, size_t& bit_image_size, const ImVec2& size = ImVec2(150, 150), float speed = 1000, float life_cycle = 1.5, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0), CreateTextureCallback load = 0, DeleteTextureCallback unload = 0, HImageManagerIO::DrawLoadingCallback draw_loading = 0);
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
	void Image_url_gif(const char* url, const char* path, const char* id, const ImVec2& size = ImVec2(150, 150), float speed = 1000, bool CacheFile = false, float rounding = 0, float life_cycle = 1.5, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0), HImageManagerIO::DrawLoadingCallback draw_loading = 0, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
#endif
#endif
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
	void Image_url(const char* url, const char* path, const char* id, const ImVec2& size = ImVec2(150, 150), bool CacheFile = false, float rounding = 0, float life_cycle = 1.5, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0), HImageManagerIO::DrawLoadingCallback draw_loading = 0, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0);
#if _HAS_CXX17
	void ClearOldUrlFiles(int Hour, int minute, int second);
#endif // _HAS_CXX17
#if (!_HAS_CXX17) && _WIN32
	void ClearOldUrlFile(int Hour, int minute, int second);
#endif // (!_HAS_CXX17) && _WIN32
#endif // HIMAGE_MANAGER_URL_IMAGE_ENABLED
	void updata(float delta_time);
	const char* ImageToBitCode_DevelopmentTool(const char* filename, bool print = false);
	void ShowResourceManager(bool* p_open = 0);
}

#define HImGuiImage_CreateTextureCallBack_OpenGL [](uint8_t* data, int w, int h, char fmt) -> void* {														   \
GLuint tex;																																					   \
																																							   \
glGenTextures(1, &tex);																																		   \
glBindTexture(GL_TEXTURE_2D, tex);																															   \
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);																							   \
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);																							   \
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);																						   \
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);																						   \
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, data);													   \
glBindTexture(GL_TEXTURE_2D, 0);																															   \
return (void*)tex;																																			   \
}																																							   \

#define HImGuiImage_DeleteTextureCallBack_OpenGL [](void* tex) {																								\
GLuint texID = (GLuint)tex;																																		\
glDeleteTextures(1, &texID);																																	\
}