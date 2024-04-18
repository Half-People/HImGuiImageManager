//
//  HImGuiImageManager.cpp
//
//  Copyright (c) 2024 HalfPeople. All rights reserved.
//  MIT License
//
#define STB_IMAGE_IMPLEMENTATION
#define IMGUI_DEFINE_MATH_OPERATORS
#include "HImGuiImageManager.h"
#include "stb_image.h"
#include <unordered_map>
#include <string>

#include "imgui_internal.h"
#include <fstream>
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED || HIMAGE_MANAGER_GIF_IMAGE_ENABLED
#include <thread>

#else
#include <chrono>
#endif

#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
#if HIMAGE_MANAGER_URL_OPENSSL_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif // 0
#include "httplib.h"
#endif // HIMAGE_MANAGER_URL_IMAGE_ENABLED

#if !defined(STBI_VERSION)
#error Need to include third-party libraries ("stb_image.h")
#endif // (STBI_VERSION)

struct HImageInfo
{
	DeleteTextureCallback unload = 0;
	float life_cycle = 1.5;
	HImage image;
};
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
struct HImageInfo_gif : public HImageInfo
{
	int frames = 1;
	int* delays;
	unsigned char* data = 0;

	int current_frame = 1;
	float delay_buffer = 0;

	inline unsigned char* get_frame_image(int frame) {
		return data + ((long long)image.width * image.height * 4 * frame);
	}

	inline int get_frame_delay(int frame) {
		return delays[frame]; // don't remember what the /10 was all about, probably just time unit conversion
	}
};

struct AsynchronousGIF_info
{
	AsynchronousGIF_info(const char*& filename_, HImage** image_out_, float speed_, float life_cycle_, CreateTextureCallback load_, DeleteTextureCallback unload_)
	{
		filename = filename_;
		image_out = image_out_;
		life_cycle = life_cycle_;
		load = load_;
		speed = speed_;
		unload = unload_;
	}
	const char* filename;
	HImage** image_out;
	float life_cycle;
	float speed;
	CreateTextureCallback load = 0;
	DeleteTextureCallback unload = 0;
};
struct AsynchronousGIF_info_Bit
{
	AsynchronousGIF_info_Bit(HBitImage& image_, size_t& size_, HImage** image_out_, float speed_, float life_cycle_, CreateTextureCallback load_, DeleteTextureCallback unload_)
	{
		image = &image_;
		size = &size_;
		image_out = image_out_;
		life_cycle = life_cycle_;
		load = load_;
		speed = speed_;
		unload = unload_;
	}
	HBitImage* image;
	size_t* size;
	HImage** image_out;
	float life_cycle;
	float speed;
	CreateTextureCallback load = 0;
	DeleteTextureCallback unload = 0;
};
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
struct AsynchronousGIF_URL_info
{
	AsynchronousGIF_URL_info(const char* id_, const char* url_, const char* path_, bool CacheFile_, HImage** image_out_, float speed_, float life_cycle_, CreateTextureCallback load_, DeleteTextureCallback unload_)
	{
		id = id_;
		url = url_;
		path = path_;
		CacheFile = CacheFile_;
		image_out = image_out_;
		life_cycle = life_cycle_;
		load = load_;
		unload = unload_;
		speed = speed_;
	}
	const char* id;
	const char* url;
	const char* path;
	bool CacheFile;
	HImage** image_out;
	float life_cycle;
	float speed;
	CreateTextureCallback load;
	DeleteTextureCallback unload;
};
#endif
#endif
HImageManagerIO IO;

std::unordered_map<std::string, HImageInfo> hashMap;
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
std::unordered_map<std::string, HImageInfo> url_hashMap;
std::unordered_map<std::string, HTexture> Asyn_url_waitingloader_lists;
#if _HAS_CXX17
#include <filesystem>
#endif // _HAS_CXX17
#endif
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
std::unordered_map<std::string, HImageInfo_gif> gif_hashMap;
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
std::unordered_map<std::string, HImageInfo_gif> gif_url_hashMap;
#endif
#endif
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED || HIMAGE_MANAGER_URL_IMAGE_ENABLED
std::vector<std::string> Asynchronouslist;
#endif
std::vector<HTextureID> StaticImages;

HImageManagerIO& HImageManager::GetIO()
{
	return IO;
}

bool GetHTextureFormFile(HBitImage& bit_image, size_t& bit_image_size, HImageInfo& info, CreateTextureCallback loader)
{
	HTexture t;
	t.texture_data = stbi_load_from_memory(bit_image.data(), bit_image_size, &t.width, &t.height, &t.channel, 4);
	if (t.texture_data == NULL)
	{
		printf("\n Error : Load HBitImage %d", (long long)&bit_image);
		return false;
	}
	info.image.SetInfo(t);
	info.image.texture = loader(t.texture_data, t.width, t.height, t.channel);

	stbi_image_free(t.texture_data);
}

bool GetHTextureFormFile(const char* filename, HImageInfo& info, CreateTextureCallback loader)
{
	HTexture t;
	t.texture_data = stbi_load(filename, &t.width, &t.height, &t.channel, 4);
	if (t.texture_data == NULL)
	{
		printf("\n Error : Load Image %s", filename);
		return false;
	}
	info.image.SetInfo(t);
	info.image.texture = loader(t.texture_data, t.width, t.height, t.channel);

	stbi_image_free(t.texture_data);
}
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
bool GetHTextureFormFile(const char* filename, HImageInfo_gif& info)
{
	FILE* f = stbi__fopen(filename, "rb");
	if (!f) return stbi__errpuc("can't fopen", "Unable to open file");
	fseek(f, 0, SEEK_END);
	int bufSize = ftell(f);
	unsigned char* buf = (unsigned char*)malloc(sizeof(unsigned char) * bufSize);
	if (buf)
	{
		fseek(f, 0, SEEK_SET);
		fread(buf, 1, bufSize, f);
		info.data = stbi_load_gif_from_memory(buf, bufSize, &info.delays, &info.image.width, &info.image.height, &info.frames, &info.image.channel, 4);
		free(buf);
		buf = NULL;
	}
	else
	{
		printf("HImGuiImageManager ->GetHTextureFormFile (GIF)-> Error -> Out of memory");
	}
	fclose(f);
}
bool GetHTextureFormFile(HBitImage*& bit_image, size_t& size, HImageInfo_gif& info)
{
	info.data = stbi_load_gif_from_memory(bit_image->data(), size, &info.delays, &info.image.width, &info.image.height, &info.frames, &info.image.channel, 4);
}
void GifUpdata(HImageInfo_gif& info, float speed, CreateTextureCallback create, DeleteTextureCallback delete_)
{
	float delay = info.delays[info.current_frame] / speed;
	if (info.delay_buffer >= delay)
	{
		if (info.image.texture)
		{
			if (delete_)
				delete_(info.image.texture);
			else
				IO.DeleteTexture(info.image.texture);

			info.image.texture = 0;
		}

		if (create)
			info.image.texture = create(info.get_frame_image(info.current_frame), info.image.width, info.image.height, info.image.channel);
		else
			info.image.texture = IO.CreateTexture(info.get_frame_image(info.current_frame), info.image.width, info.image.height, info.image.channel);

		info.current_frame++;
		if (info.current_frame >= info.frames)
			info.current_frame = 1;
		info.delay_buffer = 0;
	}
	else
		info.delay_buffer += GImGui->IO.DeltaTime;
}
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
bool GetHTextureFormURL(const char* url, const char* path, const char* id, bool CacheFile, HImageInfo_gif& info)
{
	httplib::Client client(url); // 替换为实际的URL
	auto response = client.Get(path); // 替换为实际的图像路径
	if (response) {
		// 从响应中获取图像数据
		std::vector<unsigned char> imageData(response->body.begin(), response->body.end());

		if (CacheFile)
		{
			std::ofstream file(std::string(IO.url_image_cache_files_path).append("\\").append(id), std::ios::binary);
			if (file.good())
			{
				file.write(response->body.data(), response->body.size());
				file.close();
			}
		}
		info.data = stbi_load_gif_from_memory(imageData.data(), imageData.size(), &info.delays, &info.image.width, &info.image.height, &info.frames, &info.image.channel, 4);
		imageData.clear();
	}
	client.stop();
}
#endif // HIMAGE_MANAGER_URL_IMAGE_ENABLED
#endif
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED

void AsynURL_ImageLoader(std::string url, std::string path, std::string id, bool CacheFile)
{
	Asynchronouslist.push_back(id);
	httplib::Client client(url); // 替换为实际的URL

	auto response = client.Get(path); // 替换为实际的图像路径
	if (response) {
		// 从响应中获取图像数据
		std::vector<unsigned char> imageData(response->body.begin(), response->body.end());

		if (CacheFile)
		{
			std::ofstream file(std::string(IO.url_image_cache_files_path).append("\\").append(id), std::ios::binary);
			if (file.good())
			{
				file.write(response->body.data(), response->body.size());
				file.close();
			}
		}
		HTexture t;

		t.texture_data = stbi_load_from_memory(imageData.data(), imageData.size(), &t.width, &t.height, &t.channel, 4);
		response->body.clear();
		imageData.clear();
		client.stop();
		Asyn_url_waitingloader_lists[id] = t;
	}
	else
	{
		HTexture t;
		t.channel = -1;
		Asyn_url_waitingloader_lists[id] = t;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	if (Asyn_url_waitingloader_lists.count(id) > 0)
	{
		for (size_t i = 0; i < Asynchronouslist.size(); i++)
		{
			if (Asynchronouslist[i] == id)
				Asynchronouslist.erase(Asynchronouslist.begin() + i);
		}
		Asyn_url_waitingloader_lists.erase(id);
	}
}

#endif // HIMAGE_MANAGER_URL_IMAGE_ENABLED

HTextureID HImageManager::ImageLoader::StaticImageLoader(const char* filename, CreateTextureCallback load)
{
	int w, h, c;
	unsigned char* data = stbi_load(filename, &w, &h, &c, 4);
	if (data == NULL)
	{
		printf("\n Error : Load Image %s", filename);
		return 0;
	}
	if (load)
		StaticImages.push_back(load(data, w, h, c));
	else
		StaticImages.push_back(IO.CreateTexture(data, w, h, c));

	stbi_image_free(data);
	return StaticImages.back();
}

void HImageManager::ImageLoader::DeleteStaticImage(HTextureID texture, DeleteTextureCallback unload)
{
	if (unload)
		unload(texture);
	else
		IO.DeleteTexture(texture);

	if (texture)
	{
		for (size_t i = 0; i < StaticImages.size(); i++)
		{
			if (StaticImages[i] == texture)
			{
				StaticImages.erase(StaticImages.begin() + i);
				return;
			}
		}
	}
}

bool HImageManager::ImageLoader::GetImage(HBitImage& bit_image, size_t& bit_image_size, HImage*& image_out, float life_cycle, CreateTextureCallback load, DeleteTextureCallback unload)
{
	std::string name = std::to_string((long long)&bit_image);
	if (hashMap.count(name) > 0) {
		HImageInfo& info = hashMap[name];
		image_out = &info.image;
		info.life_cycle = life_cycle;
	}
	else
	{
		HImageInfo info;
		info.life_cycle = life_cycle;
		bool r;
		if (load && unload)
		{
			info.unload = unload;
			r = GetHTextureFormFile(bit_image, bit_image_size, info, load);
		}
		else
		{
			r = GetHTextureFormFile(bit_image, bit_image_size, info, IO.CreateTexture);
		}
		hashMap[name] = info;
		image_out = &info.image;
		return r && &info.image;
	}
}

bool HImageManager::ImageLoader::GetImage(const char* filename, HImage*& image_out, float life_cycle, CreateTextureCallback load, DeleteTextureCallback unload)
{
	if (hashMap.count(filename) > 0) {
		HImageInfo& info = hashMap[filename];
		image_out = &info.image;
		info.life_cycle = life_cycle;
	}
	else
	{
		HImageInfo info;
		info.life_cycle = life_cycle;
		bool r;
		if (load && unload)
		{
			info.unload = unload;
			r = GetHTextureFormFile(filename, info, load);
		}
		else
		{
			r = GetHTextureFormFile(filename, info, IO.CreateTexture);
		}
		hashMap[filename] = info;
		image_out = &info.image;
		return r && &info.image;
	}
}

#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
bool HImageManager::ImageLoader::GetImage_url(const char* url, const char* path, const char* id, HImage*& image_out, bool CacheFile, float life_cycle, CreateTextureCallback load, DeleteTextureCallback unload)
{
	if (url_hashMap.count(id) > 0) {
		HImageInfo& info = url_hashMap[id];
		image_out = &info.image;
		info.life_cycle = life_cycle;
		return true;
	}
	else
	{
		HImageInfo info;
		info.life_cycle = life_cycle;
		bool r;
		if (CacheFile)
		{
			if (load && unload)
			{
				info.unload = unload;
				r = GetHTextureFormFile(std::string(IO.url_image_cache_files_path).append("\\").append(id).c_str(), info, load);
			}
			else
			{
				r = GetHTextureFormFile(std::string(IO.url_image_cache_files_path).append("\\").append(id).c_str(), info, IO.CreateTexture);
			}
			if (r)
			{
				std::ofstream tb(std::string(IO.url_image_cache_files_path).append("\\").append(id).append(".HImageManagerTime"));
				if (tb.good())
				{
					tb << std::chrono::system_clock::now().time_since_epoch().count();
					tb.close();
				}
				url_hashMap[id] = info;
				image_out = &info.image;
				return r && &info.image;
			}
		}

		for (size_t i = 0; i < Asynchronouslist.size(); i++)
		{
			if (Asynchronouslist[i] == id)
			{
				if (Asyn_url_waitingloader_lists.count(id) > 0)
				{
					if (load && unload)
					{
						Asynchronouslist.erase(Asynchronouslist.begin() + i);
						HTexture t = Asyn_url_waitingloader_lists[id];
						Asyn_url_waitingloader_lists.erase(id);
						if (t.channel)
						{
							info.image.SetInfo(t);
							info.unload = unload;
							info.image.texture = load(t.texture_data, t.width, t.height, t.channel);
							stbi_image_free(t.texture_data);
						}
						else
							return false;
					}
					else
					{
						Asynchronouslist.erase(Asynchronouslist.begin() + i);
						HTexture t = Asyn_url_waitingloader_lists[id];
						Asyn_url_waitingloader_lists.erase(id);
						if (t.channel)
						{
							info.image.SetInfo(t);
							info.image.texture = IO.CreateTexture(t.texture_data, t.width, t.height, t.channel);
							stbi_image_free(t.texture_data);
						}
						else
							return false;
					}

					url_hashMap[id] = info;
					image_out = &info.image;
					return r && &info.image;
				}
				else
				{
					return false;
				}
			}
		}
		std::thread t(AsynURL_ImageLoader, url, path, id, CacheFile);
		t.detach();
		return false;
	}
	return false;
}
#endif // HIMAGE_MANAGER_URL_IMAGE_ENABLED
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED || HIMAGE_MANAGER_URL_IMAGE_ENABLED
void Draw_Loading::Draw_Loading_Style_1(const ImVec2& pos, float radius)
{
	ImGuiStyle* style = &ImGui::GetStyle();

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	DrawList->PathClear();

	int num_segments = 30;
	float start = fabsf(sinf(ImGui::GetTime() * 1.8f) * (num_segments - 5));

	float a_min = 3.14159265358979323846f * 2.0f * start / num_segments;
	float a_max = 3.14159265358979323846f * 2.0f * (num_segments - 3) / num_segments;

	ImVec2 centre = ImVec2(pos.x - (radius / 20), pos.y + (radius / 20));

	for (int i = 0; i <= num_segments; i++) {
		float a = a_min + (i / (float)num_segments) * (a_max - a_min);
		DrawList->PathLineTo(ImVec2(centre.x + (cosf(a + ImGui::GetTime() * 8) * radius), centre.y + sinf(a + ImGui::GetTime() * 7) * radius));
	}

	DrawList->PathStroke(ImGui::GetColorU32(ImGuiCol_FrameBgHovered), false, 16);
}
#endif

#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
void AsynchronousProcessingGIF(AsynchronousGIF_info AsynInfo)
{
	HImageInfo_gif info;
	info.life_cycle = AsynInfo.life_cycle;

	if (AsynInfo.load && AsynInfo.unload)
	{
		info.unload = AsynInfo.unload;
		GetHTextureFormFile(AsynInfo.filename, info);
	}
	else
	{
		GetHTextureFormFile(AsynInfo.filename, info);
	}
	GifUpdata(info, AsynInfo.speed, AsynInfo.load, AsynInfo.unload);
	gif_hashMap[AsynInfo.filename] = info;
	*AsynInfo.image_out = &info.image;
	for (size_t i = 0; i < Asynchronouslist.size(); i++)
	{
		if (Asynchronouslist[i] == AsynInfo.filename)
			Asynchronouslist.erase(Asynchronouslist.begin() + i);
	}
}

void AsynchronousProcessingGIF_Bit(AsynchronousGIF_info_Bit AsynInfo)
{
	HImageInfo_gif info;
	info.life_cycle = AsynInfo.life_cycle;

	if (AsynInfo.load && AsynInfo.unload)
	{
		info.unload = AsynInfo.unload;
		GetHTextureFormFile(AsynInfo.image, *AsynInfo.size, info);
	}
	else
	{
		GetHTextureFormFile(AsynInfo.image, *AsynInfo.size, info);
	}
	GifUpdata(info, AsynInfo.speed, AsynInfo.load, AsynInfo.unload);
	std::string id = std::to_string((int)AsynInfo.image);
	gif_hashMap[id] = info;
	*AsynInfo.image_out = &info.image;
	for (size_t i = 0; i < Asynchronouslist.size(); i++)
	{
		if (Asynchronouslist[i] == id)
			Asynchronouslist.erase(Asynchronouslist.begin() + i);
	}
}

bool HImageManager::ImageLoader::GetImage_gif(const char* filename, HImage*& image_out, float speed, float life_cycle, CreateTextureCallback load, DeleteTextureCallback unload)
{
	if (gif_hashMap.count(filename) > 0) {
		HImageInfo_gif& info = gif_hashMap[filename];
		info.life_cycle = life_cycle;
		GifUpdata(info, speed, load, unload);
		image_out = &info.image;
		return true;
	}
	else
	{
		for (size_t i = 0; i < Asynchronouslist.size(); i++)
		{
			if (Asynchronouslist[i] == filename)
				return false;
		}
		Asynchronouslist.push_back(filename);
		std::thread t(AsynchronousProcessingGIF, AsynchronousGIF_info(filename, &image_out, speed, life_cycle, load, unload));
		t.detach();
		return false;
	}
}

bool HImageManager::ImageLoader::GetImage_gif(HBitImage& bit_image, size_t& bit_image_size, HImage*& image_out, float speed, float life_cycle, CreateTextureCallback load, DeleteTextureCallback unload)
{
	std::string id = std::to_string((int)&bit_image);
	if (gif_hashMap.count(id) > 0) {
		HImageInfo_gif& info = gif_hashMap[id];
		info.life_cycle = life_cycle;
		GifUpdata(info, speed, load, unload);
		image_out = &info.image;
		return true;
	}
	else
	{
		for (size_t i = 0; i < Asynchronouslist.size(); i++)
		{
			if (Asynchronouslist[i] == id)
				return false;
		}
		Asynchronouslist.push_back(id);
		std::thread t(AsynchronousProcessingGIF_Bit, AsynchronousGIF_info_Bit(bit_image, bit_image_size, &image_out, speed, life_cycle, load, unload));
		t.detach();
		return false;
	}
}

void HImageManager::DrawList::AddImage_gif(ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float speed, float life_cycle, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, HImageManagerIO::DrawLoadingCallback draw_loading, CreateTextureCallback load, DeleteTextureCallback unload)
{
	HImage* image;
	HImageManager::ImageLoader::GetImage_gif(filename, image, speed, life_cycle, load, unload);
	if (image)
		draw_list->AddImage(image->texture, p_min, p_max, uv_min, uv_max, col);
	else
	{
		ImVec2 size = ImRect(p_min, p_max).GetSize();
		draw_list->AddRectFilled(p_min, p_max, ImGui::GetColorU32(ImGuiCol_FrameBg));
		float radius = std::min(size.x, size.y) / 4;
		ImVec2 half_pos = p_min + size / 2;
		if (draw_loading)
			draw_loading(half_pos, radius);
		else
			IO.DrawLoading(half_pos, radius);
	}
}

void HImageManager::DrawList::AddImageRounded_gif(ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float rounding, float speed, float life_cycle, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, ImDrawFlags flags, HImageManagerIO::DrawLoadingCallback draw_loading, CreateTextureCallback load, DeleteTextureCallback unload)
{
	HImage* image = 0;
	HImageManager::ImageLoader::GetImage_gif(filename, image, speed, life_cycle, load, unload);
	if (image)
		draw_list->AddImageRounded(image->texture, p_min, p_max, uv_min, uv_max, col, rounding, flags);
	else
	{
		ImVec2 size = ImRect(p_min, p_max).GetSize();
		draw_list->AddRectFilled(p_min, p_max, ImGui::GetColorU32(ImGuiCol_FrameBg), rounding);
		float radius = std::min(size.x, size.y) / 4;
		ImVec2 half_pos = p_min + size / 2;
		if (draw_loading)
			draw_loading(half_pos, radius);
		else
			IO.DrawLoading(half_pos, radius);
	}
}

void HImageManager::Image_gif(const char* filename, const ImVec2& size, float speed, float life_cycle, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col, CreateTextureCallback load, DeleteTextureCallback unload, HImageManagerIO::DrawLoadingCallback draw_loading)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	if (border_col.w > 0.0f)
		bb.Max += ImVec2(2, 2);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, 0))
		return;
	HImage* image = 0;
	HImageManager::ImageLoader::GetImage_gif(filename, image, speed, life_cycle, load, unload);
	if (!image)
	{
		window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg));

		float radius = std::min(size.x, size.y) / 4;
		ImVec2 half_pos = bb.Min + size / 2;
		if (draw_loading)
			draw_loading(half_pos, radius);
		else
			IO.DrawLoading(half_pos, radius);
		return;
	}

	if (border_col.w > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(border_col), 0.0f);
		window->DrawList->AddImage(image->texture, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, ImGui::GetColorU32(tint_col));
	}
	else
	{
		window->DrawList->AddImage(image->texture, bb.Min, bb.Max, uv0, uv1, ImGui::GetColorU32(tint_col));
	}
}

void HImageManager::Image_gif(HBitImage& bit_image, size_t& bit_image_size, const ImVec2& size, float speed, float life_cycle, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col, CreateTextureCallback load, DeleteTextureCallback unload, HImageManagerIO::DrawLoadingCallback draw_loading)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	if (border_col.w > 0.0f)
		bb.Max += ImVec2(2, 2);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, 0))
		return;
	HImage* image = 0;
	HImageManager::ImageLoader::GetImage_gif(bit_image, bit_image_size, image, speed, life_cycle, load, unload);
	if (!image)
	{
		window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg));

		float radius = std::min(size.x, size.y) / 4;
		ImVec2 half_pos = bb.Min + size / 2;
		if (draw_loading)
			draw_loading(half_pos, radius);
		else
			IO.DrawLoading(half_pos, radius);
		return;
	}

	if (border_col.w > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(border_col), 0.0f);
		window->DrawList->AddImage(image->texture, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, ImGui::GetColorU32(tint_col));
	}
	else
	{
		window->DrawList->AddImage(image->texture, bb.Min, bb.Max, uv0, uv1, ImGui::GetColorU32(tint_col));
	}
}

#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
void AsynchronousProcessingURL_GIF(AsynchronousGIF_URL_info AsynInfo)
{
	HImageInfo_gif info;
	info.life_cycle = AsynInfo.life_cycle;

	if (AsynInfo.load && AsynInfo.unload)
	{
		info.unload = AsynInfo.unload;
		GetHTextureFormURL(AsynInfo.url, AsynInfo.path, AsynInfo.id, AsynInfo.CacheFile, info);
	}
	else
	{
		GetHTextureFormURL(AsynInfo.url, AsynInfo.path, AsynInfo.id, AsynInfo.CacheFile, info);
	}

	GifUpdata(info, AsynInfo.speed, AsynInfo.load, AsynInfo.unload);

	gif_url_hashMap[AsynInfo.id] = info;
	*AsynInfo.image_out = &info.image;

	for (size_t i = 0; i < Asynchronouslist.size(); i++)
	{
		if (Asynchronouslist[i] == AsynInfo.id)
			Asynchronouslist.erase(Asynchronouslist.begin() + i);
	}
	return;
}
bool HImageManager::ImageLoader::GetImage_url_gif(const char* url, const char* path, const char* id, HImage*& image_out, float speed, bool CacheFile, float life_cycle, CreateTextureCallback load, DeleteTextureCallback unload)
{
	if (gif_url_hashMap.count(id) > 0) {
		HImageInfo_gif& info = gif_url_hashMap[id];
		info.life_cycle = life_cycle;
		GifUpdata(info, speed, load, unload);
		image_out = &info.image;
		return true;
	}
	else
	{
		HImageInfo_gif info;
		info.life_cycle = life_cycle;
		bool r;
		if (CacheFile)
		{
			if (load && unload)
			{
				info.unload = unload;
				r = GetHTextureFormFile(std::string(IO.url_image_cache_files_path).append("\\").append(id).c_str(), info);
			}
			else
			{
				r = GetHTextureFormFile(std::string(IO.url_image_cache_files_path).append("\\").append(id).c_str(), info);
			}
			if (r)
			{
				std::ofstream tb(std::string(IO.url_image_cache_files_path).append("\\").append(id).append(".HImageManagerTime"));
				if (tb.good())
				{
					tb << std::chrono::system_clock::now().time_since_epoch().count();
					tb.close();
				}
				gif_url_hashMap[id] = info;
				image_out = &info.image;
				return r && &info.image;
			}
		}

		for (size_t i = 0; i < Asynchronouslist.size(); i++)
		{
			if (Asynchronouslist[i] == id)
				return false;
		}
		Asynchronouslist.push_back(id);
		std::thread t(AsynchronousProcessingURL_GIF, AsynchronousGIF_URL_info(id, url, path, CacheFile, &image_out, speed, life_cycle, load, unload));
		t.detach();
		return false;
	}
}
void HImageManager::Image_url_gif(const char* url, const char* path, const char* id, const ImVec2& size, float speed, bool CacheFile, float rounding, float life_cycle, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col, HImageManagerIO::DrawLoadingCallback draw_loading, CreateTextureCallback load, DeleteTextureCallback unload)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	if (border_col.w > 0.0f)
		bb.Max += ImVec2(2, 2);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, 0))
		return;

	HImage* image = 0;
	HImageManager::ImageLoader::GetImage_url_gif(url, path, id, image, speed, CacheFile, life_cycle, load, unload);
	if (!image)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), rounding);

		float radius = std::min(size.x, size.y) / 4;
		ImVec2 half_pos = bb.Min + size / 2;
		if (draw_loading)
			draw_loading(half_pos, radius);
		else
			IO.DrawLoading(half_pos, radius);
		return;
	}

	if (border_col.w > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(border_col), rounding);
		window->DrawList->AddImageRounded(image->texture, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, ImGui::GetColorU32(tint_col), rounding);
	}
	else
	{
		window->DrawList->AddImageRounded(image->texture, bb.Min, bb.Max, uv0, uv1, ImGui::GetColorU32(tint_col), rounding);
	}
}
void HImageManager::DrawList::AddImage_url_gif(ImDrawList* draw_list, const char* url, const char* path, const char* id, const ImVec2& p_min, const ImVec2& p_max, float rounding, bool CacheFile, float speed, float life_cycle, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, ImDrawFlags flags, HImageManagerIO::DrawLoadingCallback draw_loading, CreateTextureCallback load, DeleteTextureCallback unload)
{
	HImage* image = 0;
	HImageManager::ImageLoader::GetImage_url_gif(url, path, id, image, speed, CacheFile, life_cycle, load, unload);
	if (!image)
	{
		draw_list->AddRect(p_min, p_max, ImGui::GetColorU32(ImGuiCol_FrameBg), rounding);
		ImVec2 size = ImRect(p_min, p_max).GetSize();
		float radius = std::min(size.x, size.y) / 4;
		ImVec2 half_pos = p_min + size / 2;
		if (draw_loading)
			draw_loading(half_pos, radius);
		else
			IO.DrawLoading(half_pos, radius);
		return;
	}

	draw_list->AddImageRounded(image->texture, p_min, p_max, uv_min, uv_max, col, rounding, flags);
}
#endif // HIMAGE_MANAGER_GIF_IMAGE_ENABLED
#endif
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED

void HImageManager::DrawList::AddImage_url(ImDrawList* draw_list, const char* url, const char* path, const char* id, const ImVec2& p_min, const ImVec2& p_max, bool CacheFile, float life_cycle, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, HImageManagerIO::DrawLoadingCallback draw_loading, CreateTextureCallback load, DeleteTextureCallback unload)
{
	HImage* image = 0;
	HImageManager::ImageLoader::GetImage_url(url, path, id, image, CacheFile, life_cycle, load, unload);
	if (image)
	{
		draw_list->AddImage(image->texture, p_min, p_max, uv_min, uv_max, col);
	}
	else
	{
		ImVec2 size = ImRect(p_min, p_max).GetSize();
		draw_list->AddRectFilled(p_min, p_max, ImGui::GetColorU32(ImGuiCol_FrameBg), 0);
		float radius = std::min(size.x, size.y) / 4;
		ImVec2 half_pos = p_min + size / 2;
		if (draw_loading)
			draw_loading(half_pos, radius);
		else
			IO.DrawLoading(half_pos, radius);
	}
}

void HImageManager::DrawList::AddImageRounded_url(ImDrawList* draw_list, const char* url, const char* path, const char* id, const ImVec2& p_min, const ImVec2& p_max, float rounding, bool CacheFile, float life_cycle, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, ImDrawFlags flags, HImageManagerIO::DrawLoadingCallback draw_loading, CreateTextureCallback load, DeleteTextureCallback unload)
{
	HImage* image = 0;
	HImageManager::ImageLoader::GetImage_url(url, path, id, image, CacheFile, life_cycle, load, unload);
	if (image)
	{
		draw_list->AddImageRounded(image->texture, p_min, p_max, uv_min, uv_max, col, rounding);
	}
	else
	{
		ImVec2 size = ImRect(p_min, p_max).GetSize();
		draw_list->AddRectFilled(p_min, p_max, ImGui::GetColorU32(ImGuiCol_FrameBg), rounding);
		float radius = std::min(size.x, size.y) / 4;
		ImVec2 half_pos = p_min + size / 2;
		if (draw_loading)
			draw_loading(half_pos, radius);
		else
			IO.DrawLoading(half_pos, radius);
	}
}

void HImageManager::Image_url(const char* url, const char* path, const char* id, const ImVec2& size, bool CacheFile, float rounding, float life_cycle, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col, HImageManagerIO::DrawLoadingCallback draw_loading, CreateTextureCallback load, DeleteTextureCallback unload)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	if (border_col.w > 0.0f)
		bb.Max += ImVec2(2, 2);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, 0))
		return;

	HImage* image = 0;
	HImageManager::ImageLoader::GetImage_url(url, path, id, image, CacheFile, life_cycle, load, unload);
	if (!image)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), rounding);

		float radius = std::min(size.x, size.y) / 4;
		ImVec2 half_pos = bb.Min + size / 2;
		if (draw_loading)
			draw_loading(half_pos, radius);
		else
			IO.DrawLoading(half_pos, radius);
		return;
	}
	if (border_col.w > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(border_col), rounding);
		window->DrawList->AddImageRounded(image->texture, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, ImGui::GetColorU32(tint_col), rounding);
	}
	else
	{
		window->DrawList->AddImageRounded(image->texture, bb.Min, bb.Max, uv0, uv1, ImGui::GetColorU32(tint_col), rounding);
	}
}
#if _HAS_CXX17
void HImageManager::ClearOldUrlFiles(int Hour, int minute, int second)
{
	for (const auto& entry : std::filesystem::directory_iterator(IO.url_image_cache_files_path))
	{
		if (std::filesystem::is_regular_file(entry))
		{
			std::filesystem::path path = entry.path();
			if (path.filename().string().find(".HImageManagerTime") != std::string::npos)
			{
				std::ifstream f(path);
				if (f.good())
				{
					std::string timeStr;
					std::getline(f, timeStr);
					f.close();
					// 将时间字符串转换为整数
					long long timeValue = std::stoll(timeStr);
					timeStr.clear();

					auto input = std::chrono::system_clock::now() - std::chrono::hours(Hour) - std::chrono::minutes(minute) - std::chrono::minutes(second);
					if (timeValue < input.time_since_epoch().count())
					{
						std::filesystem::remove(path.string().substr(0, path.string().find_last_of(".")));
						std::filesystem::remove(path);
					}
				}
			}
		}
	}
}
#endif // 0
#if (!_HAS_CXX17) && _WIN32
void HImageManager::ClearOldUrlFile(int Hour, int minute, int second)
{
	std::string searchPath = std::string(IO.url_image_cache_files_path) + "/*.*";
	struct _finddata_t fileInfo;
	intptr_t handle = _findfirst(searchPath.c_str(), &fileInfo);

	if (handle == -1) {
		printf("\nFailed to open directory: %s", IO.url_image_cache_files_path);
		return;
	}

	do {
		if (strcmp(fileInfo.name, ".") != 0 && strcmp(fileInfo.name, "..") != 0) {
			std::string path = std::string(IO.url_image_cache_files_path) + "\\" + fileInfo.name;
			if (!(fileInfo.attrib & _A_SUBDIR)) {
				if (path.find(".HImageManagerTime") != std::string::npos)
				{
					std::ifstream f(path);
					if (f.good())
					{
						std::string timeStr;
						std::getline(f, timeStr);
						f.close();
						// 将时间字符串转换为整数
						long long timeValue = std::stoll(timeStr);
						timeStr.clear();

						auto input = std::chrono::system_clock::now() - std::chrono::hours(Hour) - std::chrono::minutes(minute) - std::chrono::minutes(second);
						if (timeValue < input.time_since_epoch().count())
						{
							remove(path.substr(0, path.find_last_of(".")).c_str());
							remove(path.c_str());
						}
					}
				}
			}
		}
	} while (_findnext(handle, &fileInfo) == 0);

	_findclose(handle);
}
#endif // (!_HAS_CXX17) && _WIN32

#endif
bool HImageManager::ImageButton_plus(const char* label, const char* Bace_ButtonImageFileName, const char* Hovered_ButtonImageFileName, const char* Active_ButtonImageFileName, const ImVec2& size_arg, float life_cycle, const ImVec2& uv_min, const ImVec2& uv_max, CreateTextureCallback load, DeleteTextureCallback unload, ImGuiButtonFlags flags)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(size, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

	// Render
	HImage* image = 0;
	HImageManager::ImageLoader::GetImage((held && hovered) ? Active_ButtonImageFileName : hovered ? Hovered_ButtonImageFileName : Bace_ButtonImageFileName, image, life_cycle, load, unload);

	//const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	ImGui::RenderNavHighlight(bb, id);
	//ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
	window->DrawList->AddImageRounded(image->texture, bb.Min, bb.Max, ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255), style.FrameRounding);

	if (g.LogEnabled)
		ImGui::LogSetNextTextDecoration("[", "]");
	ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return pressed;
}
void HImageManager::Image(HBitImage& bit_image, size_t& bit_image_size, const ImVec2& size, float rounding, float life_cycle, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col, CreateTextureCallback load, DeleteTextureCallback unload)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	if (border_col.w > 0.0f)
		bb.Max += ImVec2(2, 2);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, 0))
		return;
	HImage* image;
	HImageManager::ImageLoader::GetImage(bit_image, bit_image_size, image, life_cycle, load, unload);

	if (border_col.w > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(border_col), rounding);
		window->DrawList->AddImageRounded(image->texture, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, ImGui::GetColorU32(tint_col), rounding);
	}
	else
	{
		window->DrawList->AddImageRounded(image->texture, bb.Min, bb.Max, uv0, uv1, ImGui::GetColorU32(tint_col), rounding);
	}
}
void HImageManager::Image(const char* filename, const ImVec2& size, float rounding, float life_cycle, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col, CreateTextureCallback load, DeleteTextureCallback unload)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	if (border_col.w > 0.0f)
		bb.Max += ImVec2(2, 2);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, 0))
		return;
	HImage* image;
	HImageManager::ImageLoader::GetImage(filename, image, life_cycle, load, unload);

	if (border_col.w > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(border_col), rounding);
		window->DrawList->AddImageRounded(image->texture, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, ImGui::GetColorU32(tint_col), rounding);
	}
	else
	{
		window->DrawList->AddImageRounded(image->texture, bb.Min, bb.Max, uv0, uv1, ImGui::GetColorU32(tint_col), rounding);
	}
}

void HImageManager::updata(float delta_time)
{
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
	if (!gif_hashMap.empty())
	{
		auto iter = gif_hashMap.begin();
		while (iter != gif_hashMap.end()) {
			iter->second.life_cycle -= delta_time;
			if (iter->second.life_cycle < 0)
			{
				if (iter->second.image.texture)
				{
					if (iter->second.unload)
						iter->second.unload(iter->second.image.texture);
					else
						IO.DeleteTexture(iter->second.image.texture);
				}
				iter = gif_hashMap.erase(iter);
			}
			else
				++iter;
		}
	}
#endif
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
	if (!url_hashMap.empty())
	{
		auto iter = url_hashMap.begin();
		while (iter != url_hashMap.end()) {
			iter->second.life_cycle -= delta_time;
			if (iter->second.life_cycle < 0)
			{
				if (iter->second.image.texture)
				{
					if (iter->second.unload)
						iter->second.unload(iter->second.image.texture);
					else
						IO.DeleteTexture(iter->second.image.texture);
				}
				iter = url_hashMap.erase(iter);
			}
			else
				++iter;
		}
	}
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
	if (!gif_url_hashMap.empty())
	{
		auto iter = gif_url_hashMap.begin();
		while (iter != gif_url_hashMap.end()) {
			iter->second.life_cycle -= delta_time;
			if (iter->second.life_cycle < 0)
			{
				if (iter->second.image.texture)
				{
					if (iter->second.unload)
						iter->second.unload(iter->second.image.texture);
					else
						IO.DeleteTexture(iter->second.image.texture);
				}
				stbi_image_free(iter->second.data);
				iter = gif_url_hashMap.erase(iter);
			}
			else
				++iter;
		}
	}

#endif // HIMAGE_MANAGER_GIF_IMAGE_ENABLED
#endif // HIMAGE_MANAGER_URL_IMAGE_ENABLED

	auto iter = hashMap.begin();
	while (iter != hashMap.end()) {
		iter->second.life_cycle -= delta_time;
		if (iter->second.life_cycle < 0)
		{
			if (iter->second.unload)
				iter->second.unload(iter->second.image.texture);
			else
				IO.DeleteTexture(iter->second.image.texture);
			iter = hashMap.erase(iter);
		}
		else
			++iter;
	}
}

bool ResourceManagerItem(const char* filename, HImageInfo& info, float Size = 90, ImGuiButtonFlags flags = 0)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(filename);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 size = ImGui::CalcItemSize(ImVec2(-1, Size), style.FramePadding.x * 2.0f, style.FramePadding.y * 2.0f);

	float yoffset = (Size / 2) + style.FramePadding.y * 2;

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(size, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);

	// Render
	const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	ImGui::RenderNavHighlight(bb, id);
	ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

	window->DrawList->AddImageRounded(info.image.texture, bb.Min, ImVec2(bb.Min.x + Size, bb.Max.y), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), 15);
	window->DrawList->AddText(ImVec2(bb.Min.x + Size + style.FramePadding.x, bb.Max.y - yoffset), ImGui::GetColorU32(ImGuiCol_Text), filename);
	float s = ImGui::CalcTextSize(std::to_string(info.life_cycle).c_str(), NULL, true).x;

	window->DrawList->AddText(ImVec2(bb.Max.x - (s + 15), bb.Max.y - yoffset), ImColor(235, 140, 52), std::to_string(info.life_cycle).c_str());
	//ImGui::RenderTextClipped(ImVec2(90, 0) + (bb.Min + style.FramePadding), ImVec2(90, 0) + (bb.Max - style.FramePadding), filename, NULL, &label_size, style.ButtonTextAlign, &bb);

	return pressed;
}

inline std::string intToHex(long long num) {
	std::string hexString;
	bool negative = false;

	// 处理负数情况
	if (num < 0) {
		negative = true;
		num = -num;
	}

	// 将整数转换为十六进制字符串
	while (num > 0) {
		int remainder = num % 16;
		char hexDigit;

		if (remainder < 10) {
			hexDigit = remainder + '0';
		}
		else {
			hexDigit = remainder - 10 + 'A';
		}

		hexString = hexDigit + hexString;
		num /= 16;
	}

	// 添加负号
	if (negative) {
		hexString = "-" + hexString;
	}

	return hexString;
}

void HImageManager::ShowResourceManager(bool* p_open)
{
	static float itemsize = 90;
	if (ImGui::Begin("HImGuiImageManager-ResourceManager", 0, ImGuiWindowFlags_MenuBar))
	{
		if (ImGui::BeginMenuBar())
		{
			ImGui::SetNextItemWidth(125);
			ImGui::DragFloat("ItemSize", &itemsize, 0.1, 30);
			ImGui::EndMenuBar();
		}
		if (ImGui::BeginChild("HIMAGE_IMAGE_LIST", ImVec2(-1, 150), ImGuiChildFlags_ResizeY))
		{
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
			if (!url_hashMap.empty())
			{
				ImGui::Text("url images :");
				auto iter = url_hashMap.begin();
				while (iter != url_hashMap.end()) {
					ResourceManagerItem(iter->first.c_str(), iter->second, itemsize);
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("Image Info :\nheight :%d\nwidth : %d\nchannel : %d", iter->second.image.height, iter->second.image.width, iter->second.image.channel);
						ImGui::EndTooltip();
					}
					++iter;
				}
			}
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED
			if (!gif_url_hashMap.empty())
			{
				ImGui::Text("url gif images :");
				auto iter = gif_url_hashMap.begin();
				while (iter != gif_url_hashMap.end()) {
					ResourceManagerItem(iter->first.c_str(), iter->second, itemsize);
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::SliderInt("current_frame : %d", &iter->second.current_frame, 0, iter->second.frames);
						ImGui::Text("delay_buffer : %f", iter->second.delay_buffer);
						ImGui::Text("Max frames : %d", iter->second.frames);

						ImGui::Text("Image Info :\nheight :%d\nwidth : %d\nchannel : %d", iter->second.image.height, iter->second.image.width, iter->second.image.channel);
						ImGui::EndTooltip();
					}
					++iter;
				}
			}
#endif // HIMAGE_MANAGER_GIF_IMAGE_ENABLED
#endif // HIMAGE_MANAGER_URL_IMAGE_ENABLED
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED

			if (!gif_hashMap.empty())
			{
				ImGui::Text("gif images :");

				auto giter = gif_hashMap.begin();
				while (giter != gif_hashMap.end()) {
					ResourceManagerItem(giter->first.c_str(), giter->second, itemsize);
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::SliderInt("current_frame : %d", &giter->second.current_frame, 0, giter->second.frames);
						ImGui::Text("delay_buffer : %f", giter->second.delay_buffer);
						ImGui::Text("Max frames : %d", giter->second.frames);

						ImGui::Text("Image Info :\nheight :%d\nwidth : %d\nchannel : %d", giter->second.image.height, giter->second.image.width, giter->second.image.channel);
						ImGui::EndTooltip();
					}
					++giter;
				}
			}
#endif
			if (!hashMap.empty())
			{
				ImGui::Text("HImages :");
				auto iter = hashMap.begin();
				while (iter != hashMap.end()) {
					ResourceManagerItem(iter->first.c_str(), iter->second, itemsize);
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("Image Info :\nheight :%d\nwidth : %d\nchannel : %d", iter->second.image.height, iter->second.image.width, iter->second.image.channel);
						ImGui::EndTooltip();
					}
					++iter;
				}
			}

			if (!StaticImages.empty())
			{
				ImGui::Text("static images :");

				for (HTextureID texture : StaticImages)
				{
					HImageInfo info;
					info.image.texture = texture;
					info.life_cycle = -1;
					ResourceManagerItem(intToHex((long long)texture).c_str(), info, itemsize);
				}
			}
		}
		ImGui::EndChild();
#if HIMAGE_MANAGER_GIF_IMAGE_ENABLED || HIMAGE_MANAGER_URL_IMAGE_ENABLED
		ImGui::SeparatorText("Processing picture threads");
		//Asyn_url_waitingloader_lists
		for (size_t i = 0; i < Asynchronouslist.size(); i++)
		{
			ImGui::BulletText(Asynchronouslist[i].c_str());
		}
		ImGui::SeparatorText("Asyn url image waiting loader list");
#endif // 0
#if HIMAGE_MANAGER_URL_IMAGE_ENABLED
		auto iter = Asyn_url_waitingloader_lists.begin();
		while (iter != Asyn_url_waitingloader_lists.end()) {
			ImGui::BulletText(iter->first.c_str());
			++iter;
		}
#endif // HIMAGE_MANAGER_URL_IMAGE_ENABLED
	}
	ImGui::End();
}

const char* HImageManager::ImageToBitCode_DevelopmentTool(const char* filename, bool print = false)
{
	FILE* f = stbi__fopen(filename, "rb");
	if (!f)
		return  "Unable to open file";
	fseek(f, 0, SEEK_END);
	int bufSize = ftell(f);
	unsigned char* buf = (unsigned char*)malloc(sizeof(unsigned char) * bufSize);
	std::stringstream buffer;
	buffer << "size_t /*variable name*/_Size = " << bufSize;
	buffer << ";\nHBitImage /*variable name*/ = {";
	if (print)
		printf("\n\n\n---------------------------------------------------------------------------------------------------------------------------------------------------\n", buffer.str().c_str());
	if (buf)
	{
		fseek(f, 0, SEEK_SET);
		fread(buf, 1, bufSize, f);
		if (print)
		{
			for (size_t i = 0; i < bufSize; i++)
			{
				std::cout << "0x" << std::hex << (long long)buf[i];
				buffer << "0x" << std::hex << (long long)buf[i];
				if (i + 1 != bufSize)
				{
					std::cout << ",";
					buffer << ",";
				}
			}
			std::cout << "};";
		}
		else
		{
			for (size_t i = 0; i < bufSize; i++)
			{
				buffer << "0x" << std::hex << (long long)buf[i];
				if (i + 1 != bufSize)
					buffer << ",";
			}
		}
		buffer << "};";
		free(buf);
		buf = NULL;
	}
	else
	{
		printf("HImGuiImageManager ->ImageToBitCode_DevelopmentTool -> Error -> Out of memory");
	}
	fclose(f);

	std::cout << buffer.str();
	std::cout << "\n\n\n";
	return buffer.str().c_str();
}

double HImageManagerIO::HGetFunctionRuningSpeed(void(*function)())
{
	auto start = std::chrono::high_resolution_clock::now();
	function();
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;
	return duration.count();
}