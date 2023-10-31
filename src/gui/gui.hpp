#ifndef GUI_HPP
#define GUI_HPP

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <iostream>
#include <vector>

#include "logger/logger.hpp"

namespace gen {
    typedef uint32_t gui_element_id_t;
    enum gui_element_type { GUI_WINDOW };

    struct gui_element {
        gui_element(gui_element_id_t id, gui_element_type type, std::string label, glm::vec2 position, glm::vec2 size, bool is_visible) : 
            id(id), type(type), label(label), position(position), size(size), is_visible(is_visible) { }

        gui_element_id_t id;
        gui_element_type type;

        std::string label;
        glm::vec2 position;
        glm::vec2 size;
        bool is_visible;

        gui_element* parent;
        std::vector<gui_element> children;

        glm::vec2 topLeft() const {
            return position;
        }

        glm::vec2 topRight() const {
            return glm::vec2(position.x + size.x, position.y);
        }

        glm::vec2 bottomLeft() const {
            return glm::vec2(position.x, position.y + size.y);
        }

        glm::vec2 bottomRight() const {
            return glm::vec2(position.x + size.x, position.y + size.y);
        }
    };

    class imgui {
    public:
        imgui();

        void update();
    private:
        void _create_window_vertex_buffer(gui_element type);

        const std::string _LOGGER_TAG = "GEN_IMGUI";
        logger _logger;

        gui_element_id_t _next_id = 0;
        std::vector<gui_element> _elements;

    };
}

#endif

/*



*/