#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <reflection/serialization.h>
#include <reflection/autoimgui.h>

using namespace reflection;

class Shape : public ISerialization, public IAutoImGui {
public:
    virtual ~Shape() {};
};

class Circle : public Shape {
public:
    float radius{};

FIELD_DECLARATION_BEGIN(Circle, ISerialization)
    FIELD_DECLARATION("radius", radius)
FIELD_DECLARATION_END()

FIELD_DECLARATION_BEGIN(Circle, IAutoImGui)
    FIELD_DECLARATION("radius", radius, { { AutoImGuiArg::SliderFloatMin, 0.0f }, {AutoImGuiArg::SliderFloatMax, 100.0f} })
FIELD_DECLARATION_END()
};

class Rectangle : public Shape {
public:
    float width{};
    float height{};

FIELD_DECLARATION_BEGIN(Rectangle, ISerialization)
    FIELD_DECLARATION("width", width)
    FIELD_DECLARATION("height", height)
FIELD_DECLARATION_END()

FIELD_DECLARATION_BEGIN(Rectangle, IAutoImGui)
    FIELD_DECLARATION("width", width, { { AutoImGuiArg::SliderFloatMin, 0.0f }, {AutoImGuiArg::SliderFloatMax, 200.0f} })
    FIELD_DECLARATION("height", height, { { AutoImGuiArg::SliderFloatMin, 0.0f }, {AutoImGuiArg::SliderFloatMax, 200.0f} })
FIELD_DECLARATION_END()
};

SUBCLASS_DECLARATION_BEGIN(Shape)
    SUBCLASS_DECLARATION(Circle)
    SUBCLASS_DECLARATION(Rectangle)
SUBCLASS_DECLARATION_END()

class Shapes : public ISerialization, public IAutoImGui {
public:
    std::vector<std::unique_ptr<Shape>> data;

FIELD_DECLARATION_BEGIN(Shapes, ISerialization)
    FIELD_DECLARATION("data", data)
FIELD_DECLARATION_END()

FIELD_DECLARATION_BEGIN(Shapes, IAutoImGui)
    FIELD_DECLARATION("data", data)
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
    auto shapes = std::make_unique<Shapes>();

    rapidjson::Document document_origin;
    document_origin.Parse(src);
    shapes->Deserialize(document_origin);

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    shapes->Serialize(writer);
    
    rapidjson::Document document_new;
    document_new.Parse(sb.GetString());
    R_ASSERT(document_origin == document_new);

    auto DrawGui = [&shapes]() {
        if (ImGui::Button("Serialize")) {
            system("cls");
            rapidjson::StringBuffer sb;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
            shapes->Serialize(writer);
            puts(sb.GetString());
        }
        ImGui::Separator();
        shapes->DrawAutoImGui();
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
 