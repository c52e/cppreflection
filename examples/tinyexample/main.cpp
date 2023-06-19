// clang-format off
// Order cannot change
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <reflection/autoimgui.h>
#include <reflection/serialization.h>

#include <iostream>
#include <vector>

#define DECLARE(name, ...) FIELD_DECLARATION(#name, name, __VA_ARGS__)

using reflection::IAutoImGui;
using reflection::ISerialization;

using reflection::Deserialize;
using reflection::DrawAutoImGui;
using reflection::Serialize;

class Shape : public ISerialization, public IAutoImGui {
public:
    virtual ~Shape(){};

    FIELD_DECLARATION_BEGIN(ISerialization)
    FIELD_DECLARATION_END()
    FIELD_DECLARATION_BEGIN(IAutoImGui)
    FIELD_DECLARATION_END()
};

HAS_SUBCLASS(Shape)

class Circle : public Shape {
public:
    float radius{};

    FIELD_DECLARATION_BEGIN(ISerialization)
    DECLARE(radius)
    FIELD_DECLARATION_END()

    FIELD_DECLARATION_BEGIN(IAutoImGui)
    DECLARE(radius, d.Min = 0.0f, d.Max = 100.0f)
    FIELD_DECLARATION_END()
};

class Rectangle : public Shape {
public:
    float width{};
    float height{};

    FIELD_DECLARATION_BEGIN(ISerialization)
    DECLARE(width)
    DECLARE(height)
    FIELD_DECLARATION_END()

    FIELD_DECLARATION_BEGIN(IAutoImGui)
    DECLARE(width, d.Min = 0.0f, d.Max = 200.0f)
    DECLARE(height, d.Min = 0.0f, d.Max = 200.0f)
    FIELD_DECLARATION_END()
};

SUBCLASS_DECLARATION_BEGIN(Shape)
SUBCLASS_DECLARATION(Circle)
SUBCLASS_DECLARATION(Rectangle)
SUBCLASS_DECLARATION_END()

class Shapes : public ISerialization, public IAutoImGui {
public:
    std::vector<std::unique_ptr<Shape>> data;

    FIELD_DECLARATION_BEGIN(ISerialization)
    DECLARE(data)
    FIELD_DECLARATION_END()

    FIELD_DECLARATION_BEGIN(IAutoImGui)
    DECLARE(data)
    FIELD_DECLARATION_END()
};

const char* src = R"(
{
    "data": [
        {
            "type": "class Circle",
            "data": {
                "radius": 50.0
            }
        },
        {
            "type": "class Rectangle",
            "data": {
                "height": 80.0,
                "width": 120.0
            }
        }
    ]
}
)";

int main() {
    Shapes shapes;

    rapidjson::Document document_origin;
    document_origin.Parse(src);
    Deserialize(shapes, document_origin);

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    Serialize(shapes, writer);

    rapidjson::Document document_new;
    document_new.Parse(sb.GetString());
    R_ASSERT(document_origin == document_new);

    auto DrawGui = [&shapes]() {
        if (ImGui::Button("Serialize")) {
            system("cls");
            rapidjson::StringBuffer sb;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
            Serialize(shapes, writer);
            puts(sb.GetString());
        }
        ImGui::Separator();
        DrawAutoImGui(shapes);
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
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
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
