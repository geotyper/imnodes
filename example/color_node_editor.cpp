#include "node_editor.h"
#include "graph.h"

#include <imnodes.h>
#include <imgui.h>

#include <SDL_keycode.h>
#include <algorithm> // for std::swap
#include <cassert>
#include <chrono>
#include <cmath>
#include <stack>
#include <utility>
#include <vector>

namespace example
{
namespace
{
// What about functions which do multiple operations?
using OperationFn = float (*)(const float);

enum class NodeType
{
    add,
    multiply,
    output,
    sine,
    time,
    value
};

struct Node
{
    NodeType type;
    float value;

    explicit Node(const NodeType t) : type(t), value(0.f) {}

    Node(const NodeType t, const float v) : type(t), value(v) {}
};

template<class T>
T clamp(T x, T a, T b)
{
    return std::min(b, std::max(x, a));
}

class ColorNodeEditor
{
public:
    ColorNodeEditor() : graph_(), nodes_(), links_() {}

    void show()
    {
        // The node editor window
        ImGui::Begin("color node editor");
        ImGui::TextUnformatted(
            "Edit the color of the output color window using nodes.");
        ImGui::Columns(2);
        ImGui::TextUnformatted("A -- add node");
        ImGui::TextUnformatted("X -- delete selected node or link");
        ImGui::NextColumn();
        ImGui::Checkbox(
            "emulate three button mouse",
            &imnodes::GetIO().emulate_three_button_mouse.enabled);
        ImGui::Columns(1);

        imnodes::BeginNodeEditor();

        for (const UiNode& node : nodes_)
        {
            switch (node.type)
            {
            case UiNodeType::add:
            {
                const float node_width = 100.f;
                imnodes::BeginNode(node.add.op);

                imnodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("add");
                imnodes::EndNodeTitleBar();
                {
                    imnodes::BeginInputAttribute(node.add.lhs);
                    const float label_width = ImGui::CalcTextSize("left").x;
                    ImGui::TextUnformatted("left");
                    if (graph_.num_adjacencies(node.add.lhs) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel",
                            &graph_.node(node.add.lhs).value,
                            0.01f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                {
                    imnodes::BeginInputAttribute(node.add.rhs);
                    const float label_width = ImGui::CalcTextSize("right").x;
                    ImGui::TextUnformatted("right");
                    if (graph_.num_adjacencies(node.add.rhs) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel",
                            &graph_.node(node.add.lhs).value,
                            0.01f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                ImGui::Spacing();

                {
                    imnodes::BeginOutputAttribute(node.add.op);
                    const float label_width = ImGui::CalcTextSize("result").x;
                    ImGui::Indent(node_width - label_width);
                    ImGui::TextUnformatted("result");
                    imnodes::EndOutputAttribute();
                }

                imnodes::EndNode();
            }
            break;
            case UiNodeType::multiply:
            {
                const float node_width = 100.0f;
                imnodes::BeginNode(node.multiply.op);

                imnodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("multiply");
                imnodes::EndNodeTitleBar();

                {
                    imnodes::BeginInputAttribute(node.multiply.lhs);
                    const float label_width = ImGui::CalcTextSize("left").x;
                    ImGui::TextUnformatted("left");
                    if (graph_.num_adjacencies(node.multiply.lhs) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel",
                            &graph_.node(node.multiply.lhs).value,
                            0.01f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                {
                    imnodes::BeginInputAttribute(node.multiply.rhs);
                    const float label_width = ImGui::CalcTextSize("right").x;
                    ImGui::TextUnformatted("right");
                    if (graph_.num_adjacencies(node.multiply.rhs) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel",
                            &graph_.node(node.multiply.rhs).value,
                            0.01f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                ImGui::Spacing();

                {
                    imnodes::BeginOutputAttribute(node.multiply.op);
                    const float label_width = ImGui::CalcTextSize("result").x;
                    ImGui::Indent(node_width - label_width);
                    ImGui::TextUnformatted("result");
                    imnodes::EndOutputAttribute();
                }

                imnodes::EndNode();
            }
            break;
            case UiNodeType::output:
            {
                const float node_width = 100.0f;
                imnodes::PushColorStyle(
                    imnodes::ColorStyle_TitleBar, IM_COL32(11, 109, 191, 255));
                imnodes::PushColorStyle(
                    imnodes::ColorStyle_TitleBarHovered,
                    IM_COL32(45, 126, 194, 255));
                imnodes::PushColorStyle(
                    imnodes::ColorStyle_TitleBarSelected,
                    IM_COL32(81, 148, 204, 255));
                imnodes::BeginNode(node.output.out);

                imnodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("output");
                imnodes::EndNodeTitleBar();

                ImGui::Dummy(ImVec2(node_width, 0.f));
                {
                    imnodes::BeginInputAttribute(node.output.r);
                    const float label_width = ImGui::CalcTextSize("r").x;
                    ImGui::TextUnformatted("r");
                    if (graph_.num_adjacencies(node.output.r) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel",
                            &graph_.node(node.output.r).value,
                            0.01f,
                            0.f,
                            1.0f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                ImGui::Spacing();

                {
                    imnodes::BeginInputAttribute(node.output.g);
                    const float label_width = ImGui::CalcTextSize("g").x;
                    ImGui::TextUnformatted("g");
                    if (graph_.num_adjacencies(node.output.g) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel",
                            &graph_.node(node.output.g).value,
                            0.01f,
                            0.f,
                            1.f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                ImGui::Spacing();

                {
                    imnodes::BeginInputAttribute(node.output.b);
                    const float label_width = ImGui::CalcTextSize("b").x;
                    ImGui::TextUnformatted("b");
                    if (graph_.num_adjacencies(node.output.b) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel",
                            &graph_.node(node.output.b).value,
                            0.01f,
                            0.f,
                            1.0f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }
                imnodes::EndNode();
                imnodes::PopColorStyle();
                imnodes::PopColorStyle();
                imnodes::PopColorStyle();
            }
            break;
            case UiNodeType::sine:
            {
                const float node_width = 100.0f;
                imnodes::BeginNode(node.sine.op);

                imnodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("sine");
                imnodes::EndNodeTitleBar();

                {
                    imnodes::BeginInputAttribute(node.sine.input);
                    const float label_width = ImGui::CalcTextSize("number").x;
                    ImGui::TextUnformatted("number");
                    if (graph_.num_adjacencies(node.sine.input) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel",
                            &graph_.node(node.sine.input).value,
                            0.01f,
                            0.f,
                            1.0f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                ImGui::Spacing();

                {
                    imnodes::BeginOutputAttribute(node.sine.op);
                    const float label_width = ImGui::CalcTextSize("output").x;
                    ImGui::Indent(node_width - label_width);
                    ImGui::TextUnformatted("output");
                    imnodes::EndInputAttribute();
                }

                imnodes::EndNode();
            }
            break;
            case UiNodeType::time:
            {
                imnodes::BeginNode(node.time.op);

                imnodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("time");
                imnodes::EndNodeTitleBar();

                imnodes::BeginOutputAttribute(node.time.op);
                ImGui::Text("output");
                imnodes::EndOutputAttribute();

                imnodes::EndNode();
            }
            break;
            }
        }

        for (const UiLink& link : links_)
        {
            imnodes::Link(link.id, link.start_attr, link.end_attr);
        }

        const bool open_popup =
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            imnodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A);

        for (const UiLink& link : links_)
        {
            imnodes::Link(link.id, link.start_attr, link.end_attr);
        }

        imnodes::EndNodeEditor();

        // Handle new links

        {
            UiLink link;
            if (imnodes::IsLinkCreated(&link.start_attr, &link.end_attr))
            {
                const NodeType start_type = graph_.node(link.start_attr).type;
                const NodeType end_type = graph_.node(link.end_attr).type;

                const bool valid_link = start_type != end_type;
                if (valid_link)
                {
                    if (start_type == NodeType::output)
                    {
                        std::swap(link.start_attr, link.end_attr);
                    }
                    link.id =
                        graph_.insert_edge(link.start_attr, link.end_attr);
                    links_.push_back(link);
                }
            }
        }

        // Handle deleted links

        {
            int link_id;
            if (imnodes::IsLinkDestroyed(&link_id))
            {
                graph_.erase_edge(link_id);
                auto iter = std::find(
                    links_.begin(),
                    links_.end(),
                    [link_id](const UiLink& link) -> bool {
                        return link.id == link_id;
                    });
                links_.erase(iter);
            }
        }

        {
            const int num_selected = imnodes::NumSelectedLinks();
            if (num_selected > 0 && ImGui::IsKeyReleased(SDL_SCANCODE_X))
            {
                //
            }
        }

        {
            const int num_selected = imnodes::NumSelectedNodes();
            if (num_selected > 0 && ImGui::IsKeyReleased(SDL_SCANCODE_X))
            {
                //
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
        if (!ImGui::IsAnyItemHovered() && open_popup)
        {
            ImGui::OpenPopup("add node");
        }

        if (ImGui::BeginPopup("add node"))
        {
            const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

            if (ImGui::MenuItem("add"))
            {
                const Node value(NodeType::value, 0.f);
                const Node op(NodeType::add);

                UiNode ui_node;
                ui_node.type = UiNodeType::add;
                ui_node.add.lhs = graph_.insert_node(value);
                ui_node.add.rhs = graph_.insert_node(value);
                ui_node.add.op = graph_.insert_node(op);

                graph_.insert_edge(ui_node.add.lhs, ui_node.multiply.op);
                graph_.insert_edge(ui_node.add.rhs, ui_node.multiply.op);

                nodes_.push_back(ui_node);
                imnodes::SetNodeScreenSpacePos(ui_node.add.op, click_pos);
            }

            if (ImGui::MenuItem("multiply"))
            {
                const Node value(NodeType::value, 0.f);
                const Node op(NodeType::multiply);

                UiNode ui_node;
                ui_node.type = UiNodeType::multiply;
                ui_node.multiply.lhs = graph_.insert_node(value);
                ui_node.multiply.rhs = graph_.insert_node(value);
                ui_node.multiply.op = graph_.insert_node(op);

                graph_.insert_edge(ui_node.multiply.lhs, ui_node.multiply.op);
                graph_.insert_edge(ui_node.multiply.rhs, ui_node.multiply.op);

                nodes_.push_back(ui_node);
                imnodes::SetNodeScreenSpacePos(ui_node.multiply.op, click_pos);
            }

            if (ImGui::MenuItem("output"))
            {
                const Node value(NodeType::value, 0.f);
                const Node out(NodeType::output);

                UiNode ui_node;
                ui_node.type = UiNodeType::output;
                ui_node.output.r = graph_.insert_node(value);
                ui_node.output.g = graph_.insert_node(value);
                ui_node.output.b = graph_.insert_node(value);
                ui_node.output.out = graph_.insert_node(out);

                graph_.insert_edge(ui_node.output.r, ui_node.output.out);
                graph_.insert_edge(ui_node.output.g, ui_node.output.out);
                graph_.insert_edge(ui_node.output.b, ui_node.output.out);

                nodes_.push_back(ui_node);
                imnodes::SetNodeScreenSpacePos(ui_node.output.out, click_pos);
            }

            if (ImGui::MenuItem("sine"))
            {
                const Node value(NodeType::value, 0.f);
                const Node op(NodeType::output);

                UiNode ui_node;
                ui_node.type = UiNodeType::sine;
                ui_node.sine.input = graph_.insert_node(value);
                ui_node.sine.op = graph_.insert_node(op);

                graph_.insert_edge(ui_node.sine.op, ui_node.sine.input);

                nodes_.push_back(ui_node);
                imnodes::SetNodeScreenSpacePos(ui_node.sine.op, click_pos);
            }

            if (ImGui::MenuItem("time"))
            {
                UiNode ui_node;
                ui_node.type = UiNodeType::time;
                ui_node.time.op = graph_.insert_node(Node(NodeType::time));

                nodes_.push_back(ui_node);
                imnodes::SetNodeScreenSpacePos(ui_node.time.op, click_pos);
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
        ImGui::End();

        // The color output window

        // TODO: actually compute the color
        const ImU32 color = IM_COL32(255, 20, 147, 255);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, color);
        ImGui::Begin("output color");
        ImGui::End();
        ImGui::PopStyleColor();
    }

private:
    enum class UiNodeType
    {
        add,
        multiply,
        output,
        sine,
        time,
    };

    struct UiNode
    {
        UiNodeType type;
        // TODO: the identifying node will have to be stored here I think?

        union
        {
            struct
            {
                int lhs, rhs, op;
            } add;

            struct
            {
                int lhs, rhs, op;
            } multiply;

            struct
            {
                int r, g, b, out;
            } output;

            struct
            {
                int input, op;
            } sine;

            struct
            {
                int op;
            } time;
        };
    };

    struct UiLink
    {
        int id;
        int start_attr, end_attr;
    };

    Graph<Node> graph_;
    std::vector<UiNode> nodes_;
    std::vector<UiLink> links_;
};

static ColorNodeEditor color_editor;
} // namespace

void NodeEditorInitialize()
{
    imnodes::IO& io = imnodes::GetIO();
    io.link_detach_with_modifier_click.modifier = &ImGui::GetIO().KeyCtrl;
}

void NodeEditorShow() { color_editor.show(); }

void NodeEditorShutdown() {}
} // namespace example
