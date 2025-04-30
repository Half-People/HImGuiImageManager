HImGuiImageManager 📦

Stable VersionHImGuiImageManager is a powerful C++ library for Dear ImGui, designed to streamline image loading, display, and management. It supports loading images from files, memory, or URLs, with built-in GIF animation support, image buttons, and custom drawing capabilities. Perfect for creating dynamic and visually rich ImGui interfaces.

🚀 Key Features

🌟 Image Loading: Load images from files, memory buffers, or URLs.  
🎞️ GIF Support: Display animated GIFs (enable with HIMAGE_MANAGER_GIF_IMAGE_ENABLED).  
🌐 URL Images: Fetch and cache images from the web (enable with HIMAGE_MANAGER_URL_IMAGE_ENABLED).  
🖼️ Image Display: Render images in ImGui windows with customizable size and rounding.  
✏️ Custom Drawing: Add images to ImDrawList for advanced rendering.  
🖱️ Image Buttons: Create interactive buttons with hover and active states.  
🗑️ Cache Management: Automatically clean up old URL cache files.


📦 Installation and Configuration
Steps

Clone the Repository:  git clone https://github.com/Half-People/HImGuiImageManager.git


Add to Your Project: Copy HImGuiImageManager.h (and .cpp if provided) into your codebase.  
Link Dependencies: Ensure ImGui and your graphics API (e.g., OpenGL, DirectX) are linked.  
Compile: Build the library with your project.

Configuring Macros
Edit HImGuiImageManager.h to enable or disable specific features. Below are the macro definitions:

HIMAGE_MANAGER_GIF_IMAGE_ENABLED  

Default: 1 (GIF support enabled)  
Description: Enables loading and displaying animated GIFs. Set to 0 to disable and reduce compilation overhead.  
Dependencies: None; GIF decoding is built-in.


HIMAGE_MANAGER_URL_IMAGE_ENABLED  

Default: 0 (URL image support disabled)  
Description: Enables loading images from URLs with caching. Set to 1 to enable.  
Dependencies: Requires cpp-httplib.  
Download: Get the latest version from cpp-httplib GitHub.  
Setup: Include httplib.h in your project and ensure proper linking.




HIMAGE_MANAGER_URL_OPENSSL_SUPPORT  

Default: 0 (OpenSSL support disabled)  
Description: Enables HTTPS support for URL images. Set to 1 if needed.  
Dependencies: Requires OpenSSL version 3.0 or later.  
Download: Obtain the source from OpenSSL GitHub.  
Build: Follow the OpenSSL installation guide.  
Tutorial: If you encounter issues, watch this YouTube tutorial for detailed OpenSSL setup instructions.






Note: Modify the macros in HImGuiImageManager.h based on your needs, and ensure you download and configure any required dependencies.


🎨 Getting Started
HImGuiImageManager operates within the HImageManager namespace. You need to set up texture creation and deletion callbacks for your graphics API, then use the library’s functions to load and display images.
Setting Up Texture Callbacks
Here are examples for different platforms:
OpenGL
HTextureID load_texture(const void* data, int width, int height, int channels) {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return (HTextureID)texture_id;
}

void unload_texture(HTextureID texture) {
    GLuint texture_id = (GLuint)texture;
    glDeleteTextures(1, &texture_id);
}

DirectX 11
HTextureID load_texture(const void* data, int width, int height, int channels) {
    ID3D11Device* device = /* Your DirectX device */;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem = data;
    init_data.SysMemPitch = width * channels;

    ID3D11Texture2D* texture;
    device->CreateTexture2D(&desc, &init_data, &texture);
    ID3D11ShaderResourceView* srv;
    device->CreateShaderResourceView(texture, nullptr, &srv);
    texture->Release();
    return (HTextureID)srv;
}

void unload_texture(HTextureID texture) {
    ID3D11ShaderResourceView* srv = (ID3D11ShaderResourceView*)texture;
    if (srv) srv->Release();
}

Vulkan
// Simplified example; actual Vulkan texture loading requires more setup
HTextureID load_texture(const void* data, int width, int height, int channels) {
    // Create Vulkan texture and return texture view
    return nullptr; // Refer to Vulkan documentation for implementation
}

void unload_texture(HTextureID texture) {
    // Release Vulkan texture resources
}

Configure callbacks:
void setup_image_manager() {
    HImageManager::GetIO().CreateTexture = load_texture;
    HImageManager::GetIO().DeleteTexture = unload_texture;
}

Main Loop Example
Here’s a simple ImGui main loop demonstrating HImGuiImageManager usage:
#include <imgui.h>
#include "HImGuiImageManager.h"

int main() {
    // Initialize ImGui and graphics API (e.g., OpenGL)
    // Set up ImGui context and window

    setup_image_manager(); // Configure texture callbacks

    while (!window_should_close()) { // Example window close condition
        ImGui::NewFrame();

        // Display images
        if (ImGui::Begin("Image Showcase")) {
            HImageManager::Image("assets/example.png", ImVec2(200, 200), 5.0f);
            HImageManager::Image_gif("assets/animation.gif", ImVec2(150, 150), 1000.0f);
            HImageManager::Image_url("https://example.com/image.jpg", "cache/", "img1", ImVec2(200, 200), true);
            ImGui::End();
        }

        // Update image manager
        HImageManager::updata(ImGui::GetIO().DeltaTime);

        ImGui::Render();
        // Render ImGui DrawData
    }

    // Clean up ImGui and graphics resources
    return 0;
}


📚 API Reference
Below is a detailed list of all functions in HImGuiImageManager.h, categorized.
🌟 Image Loading



Function
Parameters
Description



StaticImageLoader
const char* filename, CreateTextureCallback load = 0
Loads a static image and returns a texture ID.


DeleteStaticImage
HTextureID texture, DeleteTextureCallback unload = 0
Deletes a static image texture.


GetImage
const char* filename, HImage*& image_out, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0
Loads an image into an HImage object from a file.


GetImage
HBitImage& bit_image, size_t& bit_image_size, HImage*& image_out, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0
Loads an image from a memory bitmap.


GetImage_gif
const char* filename, HImage*& image_out, float speed = 1000, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0
Loads a GIF image (requires GIF support).


GetImage_url
const char* url, const char* path, const char* id, HImage*& image_out, bool CacheFile = false, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0
Loads an image from a URL (requires URL support).


🖼️ Image Display



Function
Parameters
Description



Image
const char* filename, const ImVec2& size = ImVec2(150, 150), float rounding = 0, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0
Displays a static image in an ImGui window.


Image_gif
const char* filename, const ImVec2& size = ImVec2(150, 150), float speed = 1000, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0
Displays an animated GIF.


Image_url
const char* url, const char* path, const char* id, const ImVec2& size = ImVec2(150, 150), bool CacheFile = false, float rounding = 0, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0
Displays an image loaded from a URL.


✏️ Custom Drawing



Function
Parameters
Description



AddImage
ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0
Adds an image to a draw list.


AddImageRounded
ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float rounding, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0
Adds a rounded image to a draw list.


AddImage_gif
ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float speed = 1000, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0
Adds a GIF to a draw list.


🖱️ Image Buttons



Function
Parameters
Description



ImageButton_plus
const char* label, const char* Bace_ButtonImageFileName, const char* Hovered_ButtonImageFileName, const char* Active_ButtonImageFileName, const ImVec2& size = ImVec2(150, 150), float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0
Creates an image button with base, hover, and active states.


🛠️ Utilities



Function
Parameters
Description



ClearOldUrlFiles
int Hour, int minute, int second
Removes old URL cache files (requires C++17).


updata
float delta_time
Updates the image manager for animations or cache management.



🛠️ Advanced Examples
Example 1: Displaying an Image
if (ImGui::Begin("Image Window")) {
    HImageManager::Image("assets/example.png", ImVec2(200, 200), 5.0f);
    ImGui::End();
}

Example 2: Displaying a GIF
if (ImGui::Begin("GIF Demo")) {
    HImageManager::Image_gif("assets/animation.gif", ImVec2(150, 150), 500.0f);
    ImGui::End();
}

Example 3: URL Image
if (ImGui::Begin("Web Image")) {
    HImageManager::Image_url("https://example.com/image.jpg", "cache/", "img1", ImVec2(200, 200), true);
    ImGui::End();
}

Example 4: Custom Drawing
ImDrawList* draw_list = ImGui::GetWindowDrawList();
HImageManager::DrawList::AddImageRounded(draw_list, "assets/icon.png", ImVec2(50, 50), ImVec2(150, 150), 10.0f);

Example 5: Image Button
if (HImageManager::ImageButton_plus("MyButton", "assets/btn_base.png", "assets/btn_hover.png", "assets/btn_active.png", ImVec2(100, 50))) {
    ImGui::Text("Button clicked!");
}


⚠️ Important Notes

Graphics API: Ensure texture callbacks match your graphics backend.  
Dependencies: Requires ImGui and a compatible graphics API.  
Thread Safety: Avoid loading images from multiple threads without synchronization.  
Macro Configuration: Enable GIF or URL features as needed.


📄 License
HImGuiImageManager is released under the MIT License. Feel free to use, modify, and distribute.

📧 Contact & Support

GitHub: Submit issues to HImGuiImageManager Issues.  
Email: Refer to the repository’s contact details.

Build stunning ImGui interfaces with HImGuiImageManager! 🎉
