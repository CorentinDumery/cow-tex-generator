#include <igl/opengl/glfw/Viewer.h>
#include <igl/opengl/glfw/imgui/ImGuiHelpers.h>
#include <igl/opengl/glfw/imgui/ImGuiMenu.h>
#include <igl/png/readPNG.h>
#include <igl/readOFF.h>
#include <igl/file_dialog_save.h>
#include <imgui.h>
#include <fstream>

int main(int argc, char *argv[])
{
   
    igl::opengl::glfw::Viewer viewer;
    viewer.append_mesh();
    int cow_id = viewer.data_list[0].id;
    int ground_id = viewer.data_list[1].id;

    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> R, G, B, A;
    
    igl::readOFF("../data/cow.off", V, F);

    V.col(1) = V.col(1).array() - V.col(1).minCoeff();

    viewer.data(cow_id).set_mesh(V, F);

    // Find existing cow textures in /data
    bool exists = true;
    std::vector<std::string> textures;
    int id = 0;
    while (exists){
        std::string name = "../data/cow" + std::to_string(id) + ".png";
        std::ifstream f(name);
        if (!f.good()){
            exists = false;
            break;
        }
        textures.push_back(name);
        id ++;
    }

    std::cout << "Found " << id << " textures." << std::endl;
    
    igl::opengl::glfw::imgui::ImGuiMenu menu;
    menu.callback_draw_viewer_window = []() {};
    viewer.plugins.push_back(&menu);

    //helper function for menu
    auto make_checkbox = [&](const char *label, unsigned int &option) {
        return ImGui::Checkbox(
            label,
            [&]() { return viewer.core().is_set(option); },
            [&](bool value) { return viewer.core().set(option, value); });
    };

    float light_f;
    uint continuous_rotation = 0;
    float uv_factor = 1;
    int current_texture = 0;
    bool inverse_bw = false;

    menu.callback_draw_custom_window = [&]() {
        bool show = true;
        ImGui::SetNextWindowSize(ImVec2(300, 400));
        if (ImGui::Begin("Cow"))
        {
            ImGui::SetClipboardText("Cow");
            ImGui::SetNextWindowPos(ImVec2(0.f * menu.menu_scaling(), 5),
                                    ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(150, 600), ImGuiCond_FirstUseEver);
            

            if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)){
                float w = ImGui::GetContentRegionAvailWidth();
                float p = ImGui::GetStyle().FramePadding.x;

                if (ImGui::Button("Black <-> White", ImVec2(-1,0))){
                    R = 255 - R.array();
                    G = 255 - G.array();
                    B = 255 - B.array();
                    viewer.data(cow_id).set_texture(R, G, B, A);
                }

                if (ImGui::DragFloat("UV factor", &uv_factor, 0.01, 0.01, 3)){
                    viewer.data(cow_id).set_uv(V.array() * uv_factor);
                }

                if (ImGui::SliderInt("Texture id", &current_texture, 0, textures.size()-1)){
                    if (!igl::png::readPNG(textures[current_texture], R, G, B, A))
                    {
                        std::cout << "Error: couldn't read texture" << std::endl;
                    }
                    viewer.data(cow_id).set_texture(R, G, B);
                }

                make_checkbox("Show texture", viewer.data(cow_id).show_texture);
                make_checkbox("Show lines", viewer.data(cow_id).show_lines);
                make_checkbox("Show faces", viewer.data(cow_id).show_faces);
                make_checkbox("Rotate", continuous_rotation);
            }

            if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::SliderFloat("Shininess", &viewer.data(cow_id).shininess, 0.1f, 5.0f, "%.3f");
                ImGui::SliderFloat("Specularity", &viewer.core().lighting_factor, 0.0f, 5.0f, "%.3f");
            }

            ImGui::End();
        }
    };

    viewer.callback_post_draw = [&](igl::opengl::glfw::Viewer &v) {
        float t = 0.1;
        if (continuous_rotation)
        {
            Eigen::Matrix3f rot_mat;
            rot_mat << 0.9998000, 0.0, -0.0199987,
                0, 1.0, 0.0,
                0.0199987, 0.0, 0.9998000;
            Eigen::Quaternionf rot = Eigen::Quaternionf(rot_mat);
            viewer.core().trackball_angle *= rot;
        }
        return true;
    };

    viewer.data(cow_id).show_texture = true;

    if (!igl::png::readPNG("../data/cow0.png", R, G, B, A))
    {
        std::cout << "Error: couldn't read texture" << std::endl;
    }

    viewer.data(cow_id).set_texture(R, G, B);
    viewer.data(cow_id).set_uv(V.array());
    viewer.data(cow_id).set_colors(Eigen::RowVector3d(1, 1, 1));
    viewer.data(cow_id).show_texture = true;
    viewer.data(cow_id).show_lines = 0u;
    viewer.data(cow_id).set_face_based(0u);
    viewer.data(cow_id).shininess = 2;

    Eigen::MatrixXd ground_V(4,3);
    Eigen::MatrixXd ground_UV(4,2);
    Eigen::MatrixXi ground_F(2,3);

    double far = 100;
    ground_V << far, 0, far, 
               -far, 0, far, 
               -far, 0,-far, 
                far, 0,-far;

    ground_UV << 1,  1, 
               -1,  1, 
               -1, -1, 
                1, -1;

    ground_UV = ground_UV.array() * 100;
    
    ground_F << 0, 2, 1, 2, 0, 3;
    viewer.data(ground_id).set_mesh(ground_V, ground_F);
    viewer.data(ground_id).show_texture = true;

    Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> Rg, Gg, Bg, Ag;
    if (!igl::png::readPNG("../data/grass.png", Rg, Gg, Bg, Ag))
    {
        std::cout << "Error: couldn't read texture" << std::endl;
    }
    
    viewer.data(ground_id).set_texture(Rg, Gg, Bg);
    viewer.data(ground_id).set_uv(ground_UV);
    viewer.data(ground_id).show_texture = true;
    viewer.data(ground_id).show_lines = false;

    viewer.core().background_color = Eigen::RowVector4f(129.0/255, 222.0/255, 252.0/255, 1.0);
    viewer.core().orthographic = false;
    viewer.core().is_animating = true;
    viewer.core().align_camera_center(V, F);
    viewer.core().camera_zoom = 70;
    viewer.core().lighting_factor = 0.75;

    // Initial camera angle
    Eigen::Matrix3f rot_mat(3,3);
    rot_mat << 0.999997,           0,  0.00245437,
               0.000269328, 0.993961, -0.109734,
              -0.00243955,  0.109734,  0.993958;
    Eigen::Quaternionf rot = Eigen::Quaternionf(rot_mat);
    viewer.core().trackball_angle = rot;


    viewer.launch(true, false, "Cow Tex Viz");
}