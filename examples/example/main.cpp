#include <iostream>
#include <vector>
#include <list>
#include <unordered_map>
#include <set>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define FIELD_NOT_FOUND_HANDLE(msg) throw std::runtime_error(msg);
#include <reflection/serialization_ext_glm.h>
#include <reflection/autoimgui_ext_glm.h>

using reflection::ISerialization;
using reflection::IAutoImGui;

class Shape : public ISerialization, public IAutoImGui {
public:
    virtual ~Shape() {};
};

class Circle : public Shape {
public:
    float r{};

FIELD_DECLARATION_BEGIN(Circle, ISerialization)
    FIELD_DECLARATION("r", r)
FIELD_DECLARATION_END()
FIELD_DECLARATION_BEGIN(Circle, IAutoImGui)
    FIELD_DECLARATION("r", r, .Min = 0.0f, .Max = 100.0f)
FIELD_DECLARATION_END()
};

class Rectangle : public Shape {
public:
    float w{};
    float h{};

FIELD_DECLARATION_BEGIN(Rectangle, ISerialization)
    FIELD_DECLARATION("w", w)
    FIELD_DECLARATION("h", h)
FIELD_DECLARATION_END()
FIELD_DECLARATION_BEGIN(Rectangle, IAutoImGui)
    FIELD_DECLARATION("w", w)
    FIELD_DECLARATION("h", h)
FIELD_DECLARATION_END()
};

SUBCLASS_DECLARATION_BEGIN(Shape)
    SUBCLASS_DECLARATION(Circle)
    SUBCLASS_DECLARATION(Rectangle)
SUBCLASS_DECLARATION_END()

enum class Enum {
    E1, E2
};

template <class _Ty>
struct my_delete {
    void operator()(_Ty* _Ptr) const noexcept /* strengthened */ {
        static_assert(0 < sizeof(_Ty), "can't delete an incomplete type");
        delete _Ptr;
    }
};

class Test : public ISerialization, public IAutoImGui {
public:
    Enum e{};
    int i{};
    float f{};
    bool b{};
    std::string s;
    struct Inner{
        double d{};
    };
    Inner inner{};
    std::list<float> li;
    std::map<std::string, int> map;
    std::unordered_map<std::string, std::unique_ptr<int>> umap;
    std::unique_ptr<float, my_delete<float>> uf{};
    std::unique_ptr<Shape> shape;
    std::vector<std::unique_ptr<Shape>> vec;
    std::vector<float> vecf;
    std::array<float[3], 2> mat1x2x3[1];
    glm::ivec3 glmivec3;
    glm::mat2x3 glmmat2x3;
    Rectangle rectangle;
    std::unique_ptr<Test> pnext;

FIELD_DECLARATION_BEGIN(Test, ISerialization)
    FIELD_DECLARATION("e", e)
    FIELD_DECLARATION("i", i)
    FIELD_DECLARATION("b", b)
    FIELD_DECLARATION("s", s)
    FIELD_DECLARATION("f", f)
    FIELD_DECLARATION("d", inner.d)
    FIELD_DECLARATION("li", li)
    FIELD_DECLARATION("map", map)
    FIELD_DECLARATION("umap", umap)
    FIELD_DECLARATION("uf", uf)
    FIELD_DECLARATION("shape", shape)
    FIELD_DECLARATION("vec", vec)
    FIELD_DECLARATION("vecf", vecf)
    FIELD_DECLARATION("mat1x2x3", mat1x2x3)
    FIELD_DECLARATION("glmivec3", glmivec3)
    FIELD_DECLARATION("glmmat2x3", glmmat2x3)
    FIELD_DECLARATION("rectangle", rectangle)
    FIELD_DECLARATION("pnext", pnext)
FIELD_DECLARATION_END()

FIELD_DECLARATION_BEGIN(Test, IAutoImGui)
    FIELD_DECLARATION("e", e)
    FIELD_DECLARATION("i", i, .Min = -100, .Max = 100)
    FIELD_DECLARATION("b", b)
    FIELD_DECLARATION("f", f, .Min = -10.0f, .Max = 10.0f)
    FIELD_DECLARATION("uf", uf)
    FIELD_DECLARATION("li", li)
    FIELD_DECLARATION("shape", shape)
    FIELD_DECLARATION("pnext", pnext)
    FIELD_DECLARATION("vec", vec)
    FIELD_DECLARATION("vecf", vecf, { .Min = 0.0f, .Max = 1000.0f })
    FIELD_DECLARATION("map", map)
    FIELD_DECLARATION("mat1x2x3", mat1x2x3)
    FIELD_DECLARATION("glmivec3", glmivec3)
    FIELD_DECLARATION("glmmat2x3", glmmat2x3)
    FIELD_DECLARATION("rectangle", rectangle)
    FIELD_DECLARATION("umap", umap)
FIELD_DECLARATION_END()
};

constexpr size_t kMaxMemoryAllocTimes = 0x80000;
size_t countnew = 0;
void* pnewset[kMaxMemoryAllocTimes];
size_t countdelete = 0;
void* pdeleteset[kMaxMemoryAllocTimes];
struct MemoryRecord{
    ~MemoryRecord() {
        std::sort(pnewset, pnewset + countnew);
        std::sort(pdeleteset, pdeleteset + countdelete);
        std::cout << countnew << "\t" << countdelete << "\n";
        bool leak = countnew != countdelete;
        // std::cout << "data:\n";
        for (int i = 0; i < countnew; ++i) {
            // std::cout << pnewset[i] << "\t" << pdeleteset[i] << "\n";
            leak |= pnewset[i] != pdeleteset[i];
        }
        std::cout << (leak ? "leak" : "no leak") << "\n";
    }
}_;
void* operator new(size_t size) {
    if (countnew >= kMaxMemoryAllocTimes) {
        std::cout << "Cannot record more new operation\n";
        exit(-1);
    }
    auto ptr = malloc(size);
    if (ptr == nullptr)
        throw std::bad_alloc{};
    pnewset[countnew++] = ptr;
    return ptr;
}
void operator delete(void* ptr) {
    if (ptr) {
        if (countdelete >= kMaxMemoryAllocTimes) {
            std::cout << "Cannot record more delete operation\n";
            exit(-1);
        }
        pdeleteset[countdelete++] = ptr;
        free(ptr);
    }
}

const char* src = R"(
{
    "b": false,
    "d": 1.23,
    "e": "E1",
    "f": 2.3399999141693115,
    "i": 889,
    "li": [
        0.125,
        0.25
    ],
    "map": {
        "key": 9,
        "new key": 999
    },
    "s": "string",
    "shape": null,
    "uf": 999.5,
    "umap": {
        "ukey": 9
    },
    "vec": [
        {
            "type": "class Circle",
            "data": {
                "r": 1.0
            }
        },
        {
            "type": "class Rectangle",
            "data": {
                "w": 2.0,
                "h": 3
            }
        },
        null
    ],
    "vecf": [
        0.5
    ],
    "mat1x2x3": [
        [
            [1,2,3],
            [4,5,6]
        ]
    ],
    "glmivec3": [1,2,3],
    "glmmat2x3": [
        [0.25,0.5,0.75],
        [0.25,0.5,0.75]
    ],
    "rectangle": {
        "w": 1.0,
        "h": 4
    },
    "pnext": null
}
)";

int main() {
    auto t = std::make_unique<Test>();

    rapidjson::Document document_origin;
    document_origin.Parse(src);
    t->Deserialize(document_origin);
    t->Deserialize(document_origin); // Just for testing memory leak

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    t->Serialize(writer);
    
    rapidjson::Document document_new;
    document_new.Parse(sb.GetString());
    R_ASSERT(document_origin == document_new);

    auto DrawGui = [&t]() {
        if (ImGui::Button("Serialize")) {
            rapidjson::StringBuffer sb;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
            t->Serialize(writer);
            puts(sb.GetString());
        }
        ImGui::Separator();
        t->DrawAutoImGui();
    };

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    auto window = glfwCreateWindow(800, 600, "test", NULL, NULL);
    if (window == NULL) {
        throw std::runtime_error("Failed to create GLFW window");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    while (!glfwWindowShouldClose(window)) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClear(GL_COLOR_BUFFER_BIT);
        DrawGui();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}
 