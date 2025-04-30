# HImGuiImageManager 📦

![ImGui Logo](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v184/logo.png)

**穩定版本**  
**HImGuiImageManager** 是一個為 [Dear ImGui](https://github.com/ocornut/imgui) 設計的 C++ 圖像管理庫，旨在簡化圖像的載入、顯示和管理流程。它支援從檔案、記憶體或 URL 載入圖像，並提供 GIF 動畫支援、圖像按鈕和自訂繪製功能，非常適合用於構建動態且視覺豐富的 ImGui 介面。

---

## 🚀 主要功能

- 🌟 **圖像載入**：支援從檔案、記憶體或 URL 載入圖像。  
- 🎞️ **GIF 支援**：顯示動畫 GIF（需啟用 `HIMAGE_MANAGER_GIF_IMAGE_ENABLED`）。  
- 🌐 **URL 圖像**：從網路載入圖像並支援快取（需啟用 `HIMAGE_MANAGER_URL_IMAGE_ENABLED`）。  
- 🖼️ **圖像顯示**：在 ImGui 視窗中顯示圖像，支援自訂大小和圓角。  
- ✏️ **自訂繪製**：將圖像加入 `ImDrawList` 以進行進階渲染。  
- 🖱️ **圖像按鈕**：創建具有懸停和點擊狀態的互動按鈕。  
- 🗑️ **快取管理**：自動清理舊的 URL 快取檔案。

---

## 📦 安裝與配置

### 步驟

1. **複製儲存庫**：  
   ```bash
   git clone https://github.com/Half-People/HImGuiImageManager.git
   ```
2. **加入專案**：將 `HImGuiImageManager.h`（和 `.cpp` 若有）複製到您的專案中。  
3. **連結相依性**：確保已連結 ImGui 庫和您的圖形 API（如 OpenGL、DirectX）。  
4. **編譯**：將庫檔案編譯進您的專案。

### 配置宏定義

您需要編輯 `HImGuiImageManager.h` 以啟用或停用特定功能。以下是相關宏定義的說明：

- **`HIMAGE_MANAGER_GIF_IMAGE_ENABLED`**  
  - **預設值**：`1`（啟用 GIF 支援）  
  - **說明**：啟用後，支援載入和顯示動畫 GIF。若不需要此功能，將其設為 `0` 以減少編譯開銷。  
  - **相依性**：無需額外下載，內建支援 GIF 解碼。

- **`HIMAGE_MANAGER_URL_IMAGE_ENABLED`**  
  - **預設值**：`0`（停用 URL 圖像支援）  
  - **說明**：啟用後，允許從 URL 載入圖像並支援快取。若要啟用，設為 `1`。  
  - **相依性**：需要 [cpp-httplib](https://github.com/yhirose/cpp-httplib) 庫。  
    - **下載**：從 [cpp-httplib GitHub](https://github.com/yhirose/cpp-httplib) 取得最新版本。  
    - **配置**：將 `httplib.h` 加入您的專案並確保正確連結。

- **`HIMAGE_MANAGER_URL_OPENSSL_SUPPORT`**  
  - **預設值**：`0`（停用 OpenSSL 支援）  
  - **說明**：若需要 HTTPS 支援（例如載入 HTTPS URL 的圖像），將其設為 `1`。  
  - **相依性**：需要 [OpenSSL](https://github.com/openssl/openssl) 3.0 或更高版本。  
    - **下載**：從 [OpenSSL GitHub](https://github.com/openssl/openssl) 取得源碼。  
    - **建置**：參考 [OpenSSL 安裝指南](https://github.com/openssl/openssl/blob/master/INSTALL.md)。  
    - **教學資源**：若安裝過程有困難，建議觀看 [此 YouTube 教學影片](https://youtu.be/PMHEoBkxYaQ?si=fYpChXxw_uEitGMT)，提供詳細的 OpenSSL 配置說明。

> **注意**：請根據您的需求修改 `HImGuiImageManager.h` 中的宏定義，並確保下載並正確配置所需的相依性。

---

## 🎨 使用方法

HImGuiImageManager 使用 `HImageManager` 命名空間。您需要設置圖形 API 的紋理創建和刪除回調，然後使用庫的功能來載入和顯示圖像。

### 設置紋理回調

以下是在不同平台上的回調設置範例：

#### OpenGL
```cpp
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
```

#### DirectX 11
```cpp
HTextureID load_texture(const void* data, int width, int height, int channels) {
    ID3D11Device* device = /* 您的 DirectX 裝置 */;
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
```

#### Vulkan
```cpp
// 簡化範例，實際 Vulkan 紋理載入需要更多配置
HTextureID load_texture(const void* data, int width, int height, int channels) {
    // 使用 Vulkan API 創建紋理並返回紋理視圖
    return nullptr; // 請參考 Vulkan 文檔實現
}

void unload_texture(HTextureID texture) {
    // 釋放 Vulkan 紋理資源
}
```

設置回調：
```cpp
void setup_image_manager() {
    HImageManager::GetIO().CreateTexture = load_texture;
    HImageManager::GetIO().DeleteTexture = unload_texture;
}
```

### 主迴圈範例

以下是一個簡單的 ImGui 主迴圈，展示如何使用 HImGuiImageManager：

```cpp
#include <imgui.h>
#include "HImGuiImageManager.h"

int main() {
    // 初始化 ImGui 和圖形 API（例如 OpenGL）
    // 設置 ImGui 上下文和窗口

    setup_image_manager(); // 設置紋理回調

    while (!window_should_close()) { // 假設的窗口關閉條件
        ImGui::NewFrame();

        // 顯示圖像
        if (ImGui::Begin("圖像展示")) {
            HImageManager::Image("assets/example.png", ImVec2(200, 200), 5.0f);
            HImageManager::Image_gif("assets/animation.gif", ImVec2(150, 150), 1000.0f);
            HImageManager::Image_url("https://example.com/image.jpg", "cache/", "img1", ImVec2(200, 200), true);
            ImGui::End();
        }

        // 更新圖像管理器
        HImageManager::updata(ImGui::GetIO().DeltaTime);

        ImGui::Render();
        // 渲染 ImGui DrawData
    }

    // 清理 ImGui 和圖形資源
    return 0;
}
```

---

## 📚 API 參考

以下是 `HImGuiImageManager.h` 中所有函數的詳細說明，分類列出。

### 🌟 圖像載入

| 函數 | 參數 | 說明 |
|------|------|------|
| `StaticImageLoader` | `const char* filename, CreateTextureCallback load = 0` | 從檔案載入靜態圖像，返回紋理 ID。 |
| `DeleteStaticImage` | `HTextureID texture, DeleteTextureCallback unload = 0` | 刪除靜態圖像的紋理。 |
| `GetImage` | `const char* filename, HImage*& image_out, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0` | 從檔案載入圖像到 `HImage` 物件。 |
| `GetImage` | `HBitImage& bit_image, size_t& bit_image_size, HImage*& image_out, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0` | 從記憶體位圖載入圖像。 |
| `GetImage_gif` | `const char* filename, HImage*& image_out, float speed = 1000, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0` | 載入 GIF 圖像（需啟用 GIF 支援）。 |
| `GetImage_url` | `const char* url, const char* path, const char* id, HImage*& image_out, bool CacheFile = false, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0` | 從 URL 載入圖像（需啟用 URL 支援）。 |

### 🖼️ 圖像顯示

| 函數 | 參數 | 說明 |
|------|------|------|
| `Image` | `const char* filename, const ImVec2& size = ImVec2(150, 150), float rounding = 0, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0` | 在 ImGui 視窗中顯示靜態圖像。 |
| `Image_gif` | `const char* filename, const ImVec2& size = ImVec2(150, 150), float speed = 1000, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0` | 顯示動畫 GIF。 |
| `Image_url` | `const char* url, const char* path, const char* id, const ImVec2& size = ImVec2(150, 150), bool CacheFile = false, float rounding = 0, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0` | 顯示從 URL 載入的圖像。 |

### ✏️ 自訂繪製

| 函數 | 參數 | 說明 |
|------|------|------|
| `AddImage` | `ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0` | 將圖像加入繪製列表。 |
| `AddImageRounded` | `ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float rounding, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0` | 將圓角圖像加入繪製列表。 |
| `AddImage_gif` | `ImDrawList* draw_list, const char* filename, const ImVec2& p_min, const ImVec2& p_max, float speed = 1000, float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0` | 將 GIF 加入繪製列表。 |

### 🖱️ 圖像按鈕

| 函數 | 參數 | 說明 |
|------|------|------|
| `ImageButton_plus` | `const char* label, const char* Bace_ButtonImageFileName, const char* Hovered_ButtonImageFileName, const char* Active_ButtonImageFileName, const ImVec2& size = ImVec2(150, 150), float life_cycle = 1.5, CreateTextureCallback load = 0, DeleteTextureCallback unload = 0` | 創建具有基礎、懸停和點擊狀態的圖像按鈕。 |

### 🛠️ 實用函數

| 函數 | 參數 | 說明 |
|------|------|------|
| `ClearOldUrlFiles` | `int Hour, int minute, int second` | 清除舊的 URL 快取檔案（需 C++17）。 |
| `updata` | `float delta_time` | 更新圖像管理器，用於動畫或快取管理。 |

---

## 🛠️ 進階範例

### 範例 1：顯示圖像
```cpp
if (ImGui::Begin("圖像視窗")) {
    HImageManager::Image("assets/example.png", ImVec2(200, 200), 5.0f);
    ImGui::End();
}
```

### 範例 2：顯示 GIF
```cpp
if (ImGui::Begin("GIF 展示")) {
    HImageManager::Image_gif("assets/animation.gif", ImVec2(150, 150), 500.0f);
    ImGui::End();
}
```

### 範例 3：URL 圖像
```cpp
if (ImGui::Begin("網路圖像")) {
    HImageManager::Image_url("https://example.com/image.jpg", "cache/", "img1", ImVec2(200, 200), true);
    ImGui::End();
}
```

### 範例 4：自訂繪製
```cpp
ImDrawList* draw_list = ImGui::GetWindowDrawList();
HImageManager::DrawList::AddImageRounded(draw_list, "assets/icon.png", ImVec2(50, 50), ImVec2(150, 150), 10.0f);
```

### 範例 5：圖像按鈕
```cpp
if (HImageManager::ImageButton_plus("MyButton", "assets/btn_base.png", "assets/btn_hover.png", "assets/btn_active.png", ImVec2(100, 50))) {
    ImGui::Text("按鈕被點擊！");
}
```

---

## ⚠️ 注意事項

- **圖形 API**：確保紋理回調與您的圖形後端匹配。  
- **相依性**：需要 ImGui 和相容的圖形 API。  
- **執行緒安全**：避免在多執行緒中同時載入圖像。  
- **宏配置**：根據需要啟用 GIF 或 URL 功能。  

---

## 📄 授權

HImGuiImageManager 採用 [MIT 授權](LICENSE)。您可以自由使用、修改和分發。

---

## 📧 聯繫與支援

- **GitHub**：提交問題至 [HImGuiImageManager Issues](https://github.com/Half-People/HImGuiImageManager/issues)。  
- **電子郵件**：請參考儲存庫中的聯繫資訊。  

使用 HImGuiImageManager 打造令人驚艷的 ImGui 介面吧！🎉
